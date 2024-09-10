#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/jiffies.h>
#include <linux/jhash.h>

#define MAX_CONNECTIONS 5
#define TIME_WINDOW (5 * HZ)
#define HASH_BUCKETS 1024
#define CLEANUP_INTERVAL (60 * HZ)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raz Kaner");
MODULE_DESCRIPTION("TCP connection rate limiter");

// Define a hash table with 1024 buckets
static DEFINE_HASHTABLE(connection_tracking_table, 10);

struct connection_entry {
    u32 source_ip;                  // Source IP address
    u32 dest_ip;                    // Destination IP address
    unsigned long first_connection_time; // Time of first connection in current window
    unsigned long last_connection_time;  // Time of last connection
    int connection_count;           // Number of connections in current window
    struct hlist_node node;         // For hash table linkage
};

static struct timer_list cleanup_timer;

//file operations
static struct file* connection_file;
static char* read_buf;
static const char* connection_filename = "/tmp/connections.log";

// Function to open or create the connection file
static int open_connection_file(void) {
    struct file *filp;
    int err = 0;

    filp = filp_open(connection_filename, O_RDWR | O_CREAT, 0644);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        pr_err("Failed to open or create connection file: %d\n", err);
        return err;
    }

    connection_file = filp;
    return 0;
}

// Function to write connection data to file
static void write_new_connection_to_file(struct connection_entry *entry) {
    char line[100];
    int len;
    ssize_t written;

    if (!connection_file) {
        pr_err("Connection file not open\n");
        return;
    }

    len = snprintf(line, sizeof(line), "%pI4,%pI4,%lu\n", 
                   &entry->source_ip, &entry->dest_ip, entry->last_connection_time);
    
    written = kernel_write(connection_file, line, len, &connection_file->f_pos);
    if (written < 0) {
        pr_err("Failed to write to connection file: %zd\n", written);
    }
}

// Function to update existing connection in file
static void update_connection_in_file(struct connection_entry *entry) {
    char *buf, *pos, *end;
    char line[100];
    int len;
    ssize_t file_size, read, written;
    loff_t offset = 0;

    if (!connection_file) {
        pr_err("Connection file not open\n");
        return;
    }

    // Get file size
    file_size = i_size_read(file_inode(connection_file));
    if (file_size <= 0) {
        pr_err("Failed to get file size\n");
        return;
    }

    // Allocate buffer to read file content
    buf = kmalloc(file_size + 1, GFP_KERNEL);
    if (!buf) {
        pr_err("Failed to allocate memory for file content\n");
        return;
    }

    // Read file content
    read = kernel_read(connection_file, buf, file_size, &offset);
    if (read < 0) {
        pr_err("Failed to read file content: %zd\n", read);
        kfree(buf);
        return;
    }
    buf[read] = '\0';

    // Find the line to update
    pos = buf;
    while ((end = strchr(pos, '\n')) != NULL) {
        *end = '\0';
        if (strstr(pos, &entry->source_ip) && strstr(pos, &entry->dest_ip)) {
            // Update the line
            len = snprintf(line, sizeof(line), "%pI4,%pI4,%lu\n", 
                           &entry->source_ip, &entry->dest_ip, entry->last_connection_time);
            
            // Write updated content back to file
            offset = pos - buf;
            written = kernel_write(connection_file, line, len, &offset);
            if (written < 0) {
                pr_err("Failed to update connection in file: %zd\n", written);
            }
            break;
        }
        pos = end + 1;
    }

    kfree(buf);
}

// Cleanup function
static void cleanup_connections(struct timer_list *t) {
    struct connection_entry *entry;
    struct hlist_node *tmp;
    int i;
    unsigned long current_time = jiffies;
    char *line_start, *line_end;
    u32 src_ip, dest_ip;
    unsigned long conn_time;
    ssize_t bytes_read;

    if (!connection_file) {
        pr_err("Connection file not open during cleanup\n");
        goto reschedule;
    }

    // Reset file position to beginning
    connection_file->f_pos = 0;

    // Read the file content
    bytes_read = kernel_read(connection_file, read_buf, PAGE_SIZE - 1, &connection_file->f_pos);
    if (bytes_read < 0) {
        pr_err("Failed to read connection file: %zd\n", bytes_read);
        goto reschedule;
    }
    read_buf[bytes_read] = '\0';  // Null-terminate the buffer

    line_start = read_buf;
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        *line_end = '\0';  // Temporarily replace newline with null terminator
        
        if (sscanf(line_start, "%pI4,%pI4,%lu", &src_ip, &dest_ip, &conn_time) == 3) {
            if (time_after(current_time, conn_time + TIME_WINDOW)) {
                // Connection is older than the window, remove it from hash table
                hash_for_each_possible_safe(connection_tracking_table, entry, tmp, node, src_ip) {
                    if (entry->source_ip == src_ip && entry->dest_ip == dest_ip) {
                        hash_del(&entry->node);
                        kfree(entry);
                        break;
                    }
                }
            }
        }
        
        line_start = line_end + 1;  // Move to next line
    }

reschedule:
    // Reschedule the timer
    mod_timer(&cleanup_timer, jiffies + CLEANUP_INTERVAL);
}

static unsigned int tcp_connection_limiter(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct iphdr *ip_header;
    struct tcphdr *tcp_header;
    struct connection_entry *entry;
    u32 source_ip, dest_ip;
    unsigned long current_time = jiffies;
    bool is_new_entry = false;

    ip_header = ip_hdr(skb);
    if (ip_header->protocol != IPPROTO_TCP)
        return NF_ACCEPT;  // Not TCP, accept packet

    tcp_header = tcp_hdr(skb);
    if (!tcp_header->syn)
        return NF_ACCEPT;  // Not a SYN packet, accept

    source_ip = ip_header->saddr;
    dest_ip = ip_header->daddr;

    // Only proceed if source and destination IPs are different
    if (source_ip == dest_ip) {
        return NF_ACCEPT;
    }

    rcu_read_lock();
    hash_for_each_possible_rcu(connection_tracking_table, entry, node, source_ip) {
        if (entry->source_ip == source_ip && entry->dest_ip == dest_ip) {
            // Check if we're in a new time window
            if (time_after(current_time, entry->last_connection_time + TIME_WINDOW)) {
                // New window
                entry->last_connection_time = current_time;
                entry->connection_count = 1;
                update_connection_in_file(entry);
            } else if (entry->connection_count >= MAX_CONNECTIONS) {
                rcu_read_unlock();
                printk(KERN_INFO "Rate limit exceeded for IP %pI4\n", &source_ip);
                return NF_DROP;
            } else {
                entry->connection_count++;
            }
            
            rcu_read_unlock();
            return NF_ACCEPT;
        }
    }
    rcu_read_unlock();

    // New entry
    entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
    if (!entry)
        return NF_ACCEPT;  // If allocation fails, accept packet (fail-open)

    entry->source_ip = source_ip;
    entry->dest_ip = dest_ip;
    entry->last_connection_time = current_time;
    entry->connection_count = 1;

    hash_add_rcu(connection_tracking_table, &entry->node, source_ip);

    // Write new entry to file
    write_new_connection_to_file(entry);

    return NF_ACCEPT;
}

// Netfilter hook operations structure
static struct nf_hook_ops nf_hook_ops = {
    .hook = tcp_connection_limiter,     
    .hooknum = NF_INET_LOCAL_IN,  
    .pf = NFPROTO_IPV4,                 
    .priority = NF_IP_PRI_FIRST,
};

static int __init rate_limiter_init(void) {
    int ret;

    ret = nf_register_net_hook(&init_net, &nf_hook_ops);
    if (ret < 0) {
        pr_err("Failed to register netfilter hook: %d\n", ret);
        return ret;
    }

    // Initialize and start the cleanup timer
    timer_setup(&cleanup_timer, cleanup_connections, 0);
    mod_timer(&cleanup_timer, jiffies + CLEANUP_INTERVAL);

    // Open or create the connection file
    ret = open_connection_file();
    if (ret < 0) {
        pr_err("Failed to open or create connection file: %d\n", ret);
        nf_unregister_net_hook(&init_net, &nf_hook_ops);
        return ret;
    }

    // Allocate read buffer
    read_buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!read_buf) {
        pr_err("Failed to allocate read buffer\n");
        filp_close(connection_file, NULL);
        nf_unregister_net_hook(&init_net, &nf_hook_ops);
        return -ENOMEM;
    }

    pr_info("TCP connection rate limiter initialized successfully\n");
    return 0;
}

static void __exit rate_limiter_exit(void) {
    // Delete the timer
    del_timer_sync(&cleanup_timer);

    // Close the connection file
    if (connection_file) {
        filp_close(connection_file, NULL);
    }

    // Free the read buffer
    kfree(read_buf);

    // Unregister the netfilter hook
    nf_unregister_net_hook(&init_net, &nf_hook_ops);

    // Free all entries in the hash table
    struct connection_entry *entry;
    struct hlist_node *tmp;
    int i;
    hash_for_each_safe(connection_tracking_table, i, tmp, entry, node) {
        hash_del(&entry->node);
        kfree(entry);
    }

    pr_info("TCP connection rate limiter unloaded\n");
}

module_init(rate_limiter_init);
module_exit(rate_limiter_exit);