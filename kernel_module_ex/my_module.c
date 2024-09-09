#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raz");
MODULE_DESCRIPTION("A simple example kernel module with netfilter hooks.");
MODULE_VERSION("0.01");

static struct timer_list my_timer;
static unsigned int current_hook = 0;

static void print_hook(struct timer_list *t)
{
    const char *hook_names[] = {"PREROUTING", "LOCAL_IN", "FORWARD", "LOCAL_OUT", "POSTROUTING"};
    printk(KERN_INFO "Kernel module in %s hook\n", hook_names[current_hook]);
    current_hook = (current_hook + 1) % 5;
    
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(10000));
}

static unsigned int my_hook(void *priv, struct sk_buff *skb,
                            const struct nf_hook_state *state)
{
    return NF_ACCEPT;
}

/*
This struct is defined in the Linux kernel headers
struct nf_hook_ops {
    nf_hookfn *hook;
    struct net_device *dev;
    void *priv;
    u_int8_t pf;
    unsigned int hooknum;
    int priority;
};

.hook This sets the function pointer to our hook function.
.hooknum This specifies which netfilter hook point this entry is for.
.pf This specifies the protocol family (IPv4 in this case).
.priority This sets the priority of our hook relative to others.

*/


static struct nf_hook_ops nfho[] = {
    {
        .hook = my_hook,
        .hooknum = NF_INET_PRE_ROUTING,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_FIRST,
    },
    {
        .hook = my_hook,
        .hooknum = NF_INET_LOCAL_IN,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_FIRST,
    },
    {
        .hook = my_hook,
        .hooknum = NF_INET_FORWARD,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_FIRST,
    },
    {
        .hook = my_hook,
        .hooknum = NF_INET_LOCAL_OUT,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_FIRST,
    },
    {
        .hook = my_hook,
        .hooknum = NF_INET_POST_ROUTING,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_FIRST,
    },
};

static int __init my_module_init(void)
{
    int ret;

    ret = nf_register_net_hooks(&init_net, nfho, ARRAY_SIZE(nfho));

    timer_setup(&my_timer, print_hook, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(10000));

    return 0;
}

static void __exit my_module_exit(void)
{
    nf_unregister_net_hooks(&init_net, nfho, ARRAY_SIZE(nfho));
    del_timer_sync(&my_timer);
}

module_init(my_module_init);
module_exit(my_module_exit);