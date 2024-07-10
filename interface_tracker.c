#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/inetdevice.h>
#include <linux/if_vlan.h>
#include <net/rtnetlink.h>
#include <linux/ethtool.h>

#define PROC_FILENAME "interface_tracker"
#define INTERFACE_HASH_BITS 10

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Interface Tracker Module");
MODULE_AUTHOR("ChatGPT");

static DEFINE_HASHTABLE(interface_map, INTERFACE_HASH_BITS);

struct interface_info {
    struct hlist_node hnode;
    int ifindex;
    char name[IFNAMSIZ];
    enum { PHYSICAL, VIRTUAL, LOGICAL } type;
    unsigned char mac[6];
    int vlan_id;
    int mtu;
    int speed;
    bool state_up;
    char attached_to[IFNAMSIZ];
};

static struct proc_dir_entry *proc_file;

static void update_interface_map(struct net_device *dev) {
    struct interface_info *info;
    struct net_device *master_dev = NULL;
    struct ethtool_link_ksettings ksettings;

    hash_for_each_possible(interface_map, info, hnode, dev->ifindex) {
        if (info->ifindex == dev->ifindex) {
            return;
        }
    }

    info = kmalloc(sizeof(*info), GFP_KERNEL);
    if (!info) {
        pr_err("Failed to allocate memory for interface_info\n");
        return;
    }

    info->ifindex = dev->ifindex;
    strncpy(info->name, dev->name, IFNAMSIZ);

    if (dev->flags & IFF_LOOPBACK) {
        info->type = LOGICAL;
    } else if (dev->priv_flags & IFF_LIVE_ADDR_CHANGE) {
        info->type = VIRTUAL;
    } else {
        info->type = PHYSICAL;
    }

    memcpy(info->mac, dev->dev_addr, 6);

    if (is_vlan_dev(dev)) {
        info->vlan_id = vlan_dev_vlan_id(dev);
    } else {
        info->vlan_id = 0;
    }

    info->mtu = dev->mtu;

    // Get link speed using ethtool
    if (dev->ethtool_ops && dev->ethtool_ops->get_link_ksettings) {
        if (dev->ethtool_ops->get_link_ksettings(dev, &ksettings) == 0) {
            info->speed = ksettings.base.speed;
        } else {
            info->speed = 0;
        }
    } else {
        info->speed = 0;
    }

    info->state_up = netif_running(dev);

    // Check if the device is attached to a master device (bridge, OVS, etc.)
    master_dev = netdev_master_upper_dev_get(dev);
    if (master_dev) {
        strncpy(info->attached_to, master_dev->name, IFNAMSIZ);
    } else {
        strncpy(info->attached_to, "None", IFNAMSIZ);
    }

    hash_add(interface_map, &info->hnode, dev->ifindex);
}

static int device_event_handler(struct notifier_block *nb, unsigned long event, void *ptr) {
    struct net_device *dev = netdev_notifier_info_to_dev(ptr);

    if (event == NETDEV_UP || event == NETDEV_CHANGE) {
        update_interface_map(dev);
    } else if (event == NETDEV_DOWN) {
        struct interface_info *info;

        hash_for_each_possible(interface_map, info, hnode, dev->ifindex) {
            if (info->ifindex == dev->ifindex) {
                hash_del(&info->hnode);
                kfree(info);
                break;
            }
        }
    }

    return NOTIFY_DONE;
}

static struct notifier_block device_nb = {
    .notifier_call = device_event_handler,
};

static int interface_tracker_show(struct seq_file *m, void *v) {
    struct interface_info *info;
    int bkt;

    seq_printf(m, "Interface Tracker:\n");
    hash_for_each(interface_map, bkt, info, hnode) {
        const char *type_str;
        switch (info->type) {
            case PHYSICAL: type_str = "Physical"; break;
            case VIRTUAL: type_str = "Virtual"; break;
            case LOGICAL: type_str = "Logical"; break;
            default: type_str = "Unknown"; break;
        }
        seq_printf(m, "%d %s %02x:%02x:%02x:%02x:%02x:%02x %s %d %d %d %s %s\n",
                   info->ifindex, info->name,
                   info->mac[0], info->mac[1], info->mac[2],
                   info->mac[3], info->mac[4], info->mac[5],
                   type_str, info->vlan_id, info->mtu, info->speed,
                   info->state_up ? "UP" : "DOWN", info->attached_to);
    }

    return 0;
}

static int interface_tracker_open(struct inode *inode, struct file *file) {
    return single_open(file, interface_tracker_show, NULL);
}

static const struct proc_ops interface_tracker_fops = {
    .proc_open = interface_tracker_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init interface_tracker_init(void) {
    int ret;

    proc_file = proc_create(PROC_FILENAME, 0, NULL, &interface_tracker_fops);
    if (!proc_file) {
        pr_err("Failed to create /proc/%s\n", PROC_FILENAME);
        return -ENOMEM;
    }

    ret = register_netdevice_notifier(&device_nb);
    if (ret) {
        pr_err("Failed to register netdevice notifier\n");
        proc_remove(proc_file);
        return ret;
    }

    pr_info("Interface Tracker: Initialization complete\n");
    return 0;
}

static void __exit interface_tracker_exit(void) {
    struct interface_info *info;
    struct hlist_node *tmp;
    int bkt;

    unregister_netdevice_notifier(&device_nb);
    proc_remove(proc_file);

    hash_for_each_safe(interface_map, bkt, tmp, info, hnode) {
        hash_del(&info->hnode);
        kfree(info);
    }

    pr_info("Interface Tracker: Module unloaded\n");
}

module_init(interface_tracker_init);
module_exit(interface_tracker_exit);
