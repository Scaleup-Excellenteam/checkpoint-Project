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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raz Kaner");
MODULE_DESCRIPTION("TCP connection rate limiter");

// Define a hash table with 1024 buckets
static DEFINE_HASHTABLE(connection_tracking_table, 10);

struct connection_entry {
    u32 source_ip;                  // Source IP address
    unsigned long first_connection_time; // Time of first connection in current window
    int connection_count;           // Number of connections in current window
    struct hlist_node node;         // For hash table linkage
};

static unsigned int tcp_connection_limiter(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct iphdr *ip_header;
    struct tcphdr *tcp_header;
    struct connection_entry *entry;
    u32 source_ip;
    unsigned long current_time = jiffies;

    ip_header = ip_hdr(skb);
    if (ip_header->protocol != IPPROTO_TCP)
        return NF_ACCEPT;  // Not TCP, accept packet

    tcp_header = tcp_hdr(skb);
    if (!tcp_header->syn)
        return NF_ACCEPT;  // Not a SYN packet, accept

    source_ip = ip_header->saddr;

    // Look for existing entry in hash table
    rcu_read_lock();
    hash_for_each_possible_rcu(connection_tracking_table, entry, node, source_ip) {
        if (entry->source_ip == source_ip) {
            // Check if we're in a new time window
            if (time_after(current_time, entry->first_connection_time + TIME_WINDOW)) {
                // New window, reset counters
                entry->first_connection_time = current_time;
                entry->connection_count = 1;
            } else if (entry->connection_count >= MAX_CONNECTIONS) {
                // Exceeded limit, drop packet
                rcu_read_unlock();
                printk(KERN_INFO "Rate limit exceeded for IP %pI4\n", &source_ip);
                return NF_DROP;
            } else {
                // Increment counter
                entry->connection_count++;
            }
            rcu_read_unlock();
            return NF_ACCEPT;
        }
    }
    rcu_read_unlock();

    // No existing entry, create a new one
    entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
    if (!entry)
        return NF_ACCEPT;  // If allocation fails, accept packet

    // Initialize new entry
    entry->source_ip = source_ip;
    entry->first_connection_time = current_time;
    entry->connection_count = 1;

    // Add new entry to hash table
    hash_add_rcu(connection_tracking_table, &entry->node, source_ip);

    return NF_ACCEPT;
}

// Netfilter hook operations structure
static struct nf_hook_ops nf_hook_ops = {
    .hook = tcp_connection_limiter,     
    .hooknum = NF_INET_LOCAL_IN,  
    .pf = NFPROTO_IPV4,                 
    .priority = NF_IP_PRI_FIRST,
};

static int __init rate_limiter_init(void)
{
    return nf_register_net_hook(&init_net, &nf_hook_ops);
}

static void __exit rate_limiter_exit(void)
{
    struct connection_entry *entry;
    struct hlist_node *tmp;
    int i;

    // Unregister the netfilter hook
    nf_unregister_net_hook(&init_net, &nf_hook_ops);

    // Clean up the hash table
    hash_for_each_safe(connection_tracking_table, i, tmp, entry, node) {
        hash_del(&entry->node);
        kfree(entry);
    }
}

module_init(rate_limiter_init);
module_exit(rate_limiter_exit);