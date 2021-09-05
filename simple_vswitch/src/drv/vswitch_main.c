/*
 *
 *
 *
 */

#include	"vswitch.h"
#include	"vswitch_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtuasl Switch Module");
MODULE_VERSION(VSWITCH_VERSION);

static int	debug_level = 1;

int			g_vswitch_major;
struct module		*g_vswitch_module = NULL;

/* callback functions for file operation */
struct file_operations vswitch_fops = {
	.owner          = THIS_MODULE,
	.open           = vswitch_open,
	.release        = vswitch_close,
	.read           = vswitch_read,
	.unlocked_ioctl = vswitch_ioctl,
};

int vswitch_netdev_handler( struct notifier_block*, u_long, void*);

struct notifier_block vswitch_netdev_notifier = {
	vswitch_netdev_handler,		/* notifier_call function */
	NULL,				/* next */
	0				/* priority */
};

vsw_rwlock_t		vswitch_rwlock;
struct list_head	vswitch_port_list;
int			vswitch_port_num;
struct list_head	vswitch_fdb_list;
int			vswitch_fdb_num;
struct timer_list 	vswitch_timer_list;

void remove_allports(void);
void vswitch_timer_callback(struct timer_list*);

/* initialize vswitch module */

int
vswitch_init(void)
{
	g_vswitch_major = register_chrdev(0, VSWITCH_DEVNAME, &vswitch_fops);
	if (g_vswitch_major < 0) {
		return -1;
	}
	INIT_LIST_HEAD(&vswitch_port_list);
	INIT_LIST_HEAD(&vswitch_fdb_list);
	vswitch_port_num = 0;
	vswitch_fdb_num = 0;
	vsw_rwlock_init(&vswitch_rwlock);
	timer_setup(&vswitch_timer_list, vswitch_timer_callback, 0);
	mod_timer(&vswitch_timer_list, jiffies + msecs_to_jiffies(VSWITCH_TIMER_INTERVAL));
	register_netdevice_notifier(&vswitch_netdev_notifier);
	dbgprintk(KERN_INFO "vswitch driver installed.\n");
	return 0;
}

/* finalize vswitch module */

void
vswitch_exit(void)
{
	u_long		eflags = 0;

	del_timer(&vswitch_timer_list);
	vsw_read_lock(&vswitch_rwlock, &eflags);
	if (vswitch_port_num > 0) {
		vsw_read_unlock(&vswitch_rwlock, &eflags);
		remove_allports();
		vsw_read_lock(&vswitch_rwlock, &eflags);
	}
	if (vswitch_fdb_num > 0) {
		vsw_read_unlock(&vswitch_rwlock, &eflags);
		vswitch_remove_fdb_list();
		vsw_read_lock(&vswitch_rwlock, &eflags);
	}
	vsw_read_unlock(&vswitch_rwlock, &eflags);
	unregister_netdevice_notifier(&vswitch_netdev_notifier);
	unregister_chrdev(g_vswitch_major, VSWITCH_DEVNAME);
	dbgprintk(KERN_INFO "vswitch driver uninstalled.\n");
	return;
}

module_init(vswitch_init);
module_exit(vswitch_exit);


/* callback function for open(2). */

int
vswitch_open(
	struct inode	*inode,
	struct file	*file
)
{
	dbgprintk(KERN_INFO "vswitch driver: open\n");
	if (g_vswitch_module) {
		try_module_get(g_vswitch_module);
	}
	return 0;
}

/* callback function for close(2). */

int
vswitch_close(
	struct inode	*inode,
	struct file	*file
)
{
	dbgprintk(KERN_INFO "vswitch driver: close\n");
	if (g_vswitch_module) {
		module_put(g_vswitch_module);
	}
	return 0;
}

/* callback function for ioctl(2). */

long
vswitch_ioctl(
	struct file	*file,
	u_int		cmd,
	u_long		arg
)
{
	long		rtn     = 0;
	vsw_ioctl_cmd_t		ioc_cmd;
	vsw_ioctl_get_ports_t	ioc_ports;
	void			*bufp   = NULL;
	size_t			bufsize = 0;

	dbgprintk(KERN_INFO "vswitch driver: ioctl\n");
	switch (cmd) {
	case VSWIOC_ADD_PORT:
	case VSWIOC_DELETE_PORT:
		memset(&ioc_cmd, 0, sizeof(ioc_cmd));
		bufp    = &ioc_cmd;
		bufsize = sizeof(ioc_cmd);
		break;
	case VSWIOC_GET_PORTS:
		memset(&ioc_ports, 0, sizeof(ioc_ports));
		bufp    = &ioc_ports;
		bufsize = sizeof(ioc_ports);
		break;
	default:
		bufp    = NULL;
		bufsize = 0;
		break;
	}

	if (bufp) {
		rtn = copy_from_user(bufp, (void *)arg, bufsize);
		if (rtn != 0) {
			return -ENOMEM;
		}
	}

	switch (cmd) {
	case VSWIOC_ADD_PORT:
		dbgprintk(KERN_INFO "VSWIOC_ADD_PORT: %s\n", ioc_cmd.portname);
		rtn = ioc_add_port(ioc_cmd.portname);
		break;
	case VSWIOC_DELETE_PORT:
		dbgprintk(KERN_INFO "VSWIOC_DELETE_PORT: %s\n", ioc_cmd.portname);
		rtn = ioc_delete_port(ioc_cmd.portname);
		break;
	case VSWIOC_GET_PORTS:
		dbgprintk(KERN_INFO "VSWIOC_GET_PORTS\n");
		rtn = ioc_get_ports(bufp);
		rtn = copy_to_user((void *)arg, bufp, bufsize);
		break;
	default:
		break;
	}
	return rtn;
}

/* callback function for read(2). */

ssize_t
vswitch_read(
	struct file	*file,
	char		*buf,
	size_t		count,
	loff_t		*ppos
)
{
	/* Now read is empty */
	return 0;
}

/* add port to vswitch */

long
ioc_add_port(
	char		*portname
)
{
	pport_t			*port  = NULL;
	struct net_device	*dev   = NULL;
	u_long			eflags = 0;

	vsw_read_lock(&vswitch_rwlock, &eflags);
	port = find_port_by_name(portname);
	if (port) {
		vsw_read_unlock(&vswitch_rwlock, &eflags);
		return -EEXIST;
	}
	vsw_read_unlock(&vswitch_rwlock, &eflags);
	dev = dev_get_by_name(&init_net, portname);
	if (dev == NULL) {
		return -ENODEV;
	}
	port = kmalloc(sizeof(pport_t), GFP_ATOMIC);
	if (port == NULL) {
		dev_put(dev);
		return -ENOMEM;
	}
	memset(port, 0, sizeof(pport_t));
	strncpy(port->portname, portname, sizeof(port->portname) - 1);
	port->devp = dev;
	vswitch_rx_handler_register(port->devp, port);
	vsw_write_lock(&vswitch_rwlock, &eflags);
	list_add_tail(&port->list, &vswitch_port_list);
	vswitch_port_num++;
	vsw_write_unlock(&vswitch_rwlock, &eflags);
	dbgprintk(KERN_INFO "add port: %s\n", port->portname);
	return 0;
}

/* delete port from vswitch */

long
ioc_delete_port(
	char		*portname
)
{
	pport_t			*port  = NULL;
	u_long			eflags = 0;

	vsw_read_lock(&vswitch_rwlock, &eflags);
	port = find_port_by_name(portname);
	if (port == NULL) {
		vsw_read_unlock(&vswitch_rwlock, &eflags);
		return -ENODEV;
	}
	vsw_read_unlock(&vswitch_rwlock, &eflags);
	vswitch_rx_handler_unregister(port->devp);
	vsw_write_lock(&vswitch_rwlock, &eflags);
	vswitch_disable_fdb(port);
	dev_put(port->devp);
	list_del(&port->list);
	vswitch_port_num--;
	vsw_write_unlock(&vswitch_rwlock, &eflags);
	dbgprintk(KERN_INFO "delete port: %s\n", port->portname);
	kfree(port);
	return 0;
}

/* remove all ports from vswitch */

void
remove_allports(void)
{
	struct list_head	*position = NULL;
	pport_t			*port     = NULL;
	u_long			eflags    = 0;

	vsw_read_lock(&vswitch_rwlock, &eflags);
	while (!list_empty(&vswitch_port_list)) {
		position = vswitch_port_list.next;
		port = list_entry(position, pport_t, list);
		vsw_read_unlock(&vswitch_rwlock, &eflags);
		vsw_write_lock(&vswitch_rwlock, &eflags);
		vswitch_disable_fdb(port);
		dev_put(port->devp);
		list_del(&port->list);
		vswitch_port_num--;
		vsw_write_unlock(&vswitch_rwlock, &eflags);
		vswitch_rx_handler_unregister(port->devp);
		dbgprintk(KERN_INFO "delete port: %s\n", port->portname);
		kfree(port);
		vsw_read_lock(&vswitch_rwlock, &eflags);
	}
	vsw_read_unlock(&vswitch_rwlock, &eflags);
	return;
}

/* get list of ports connected vswitch */

long
ioc_get_ports(
	vsw_ioctl_get_ports_t	*ports
)
{
	struct list_head	*position = NULL;
	pport_t			*entry    = NULL;
	u_long			eflags = 0;
	u_int			i = 0;

	if (ports == NULL) {
		return -EINVAL;
	}
	vsw_read_lock(&vswitch_rwlock, &eflags);
	ports->portnum = vswitch_port_num;
	list_for_each (position, &vswitch_port_list) {
		entry = list_entry(position, pport_t, list);
		strncpy(ports->portname[i], entry->portname, IFNAMSIZ-1);
		i++;
	}
	vsw_read_unlock(&vswitch_rwlock, &eflags);
	return 0;
}

/* find port by name. */

pport_t *
find_port_by_name(
	char		*name
)
{
	struct list_head	*position = NULL;
	pport_t			*entry    = NULL;

	if (name == NULL) {
		return NULL;
	}
	list_for_each (position, &vswitch_port_list) {
		entry = list_entry(position, pport_t, list);
		if (entry && strcmp(entry->portname, name) == 0) {
			return entry;
		}
	}
	return NULL;
}

/* find port by net_device. */

pport_t *
find_port_by_dev(
	struct net_device	*dev
)
{
	struct list_head	*position = NULL;
	pport_t			*entry    = NULL;

	if (dev == NULL) {
		return NULL;
	}
	list_for_each (position, &vswitch_port_list) {
		entry = list_entry(position, pport_t, list);
		if (entry->devp == dev) {
			return entry;
		}
	}
	return NULL;
}

/* timer callback function */

void
vswitch_timer_callback(struct timer_list* timer)
{
	dbgprintk(KERN_INFO "vswitch_timer_callback (%ld)\n", jiffies);
	vsw_read_lock(&vswitch_rwlock, NULL);
	vswitch_countdown_fdb();
	vsw_read_unlock(&vswitch_rwlock, NULL);
	mod_timer(&vswitch_timer_list, jiffies + msecs_to_jiffies(VSWITCH_TIMER_INTERVAL));
	return;
}

/* netdevice event handler */

int
vswitch_netdev_handler(
	struct notifier_block	*notify,
	u_long			event,
	void			*ptr
)
{
	struct net_device	*devp = NULL;
	struct list_head	*position = NULL;
	pport_t			*entry    = NULL;

	devp = (struct net_device*)ptr;
#if 0
	switch (event) {
	case NETDEV_UP:
		dbgprintk(KERN_INFO "NETDEV_UP: dev=%s\n",
			 devp ? devp->name : "none");
		break;
	case NETDEV_DOWN:
		dbgprintk(KERN_INFO "NETDEV_DOWN: dev=%s\n",
			 devp ? devp->name : "none");
		break;
	case NETDEV_REBOOT:
		dbgprintk(KERN_INFO "NETDEV_REBOOT: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_CHANGE:
		dbgprintk(KERN_INFO "NETDEV_CHANGE: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_REGISTER:
		dbgprintk(KERN_INFO "NETDEV_REGISTER: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_UNREGISTER:
		dbgprintk(KERN_INFO "NETDEV_UNREGISTER: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_CHANGEMTU:
		dbgprintk(KERN_INFO "NETDEV_CHANGEMTU: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_CHANGEADDR:
		dbgprintk(KERN_INFO "NETDEV_CHANGEADDR: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_GOING_DOWN:
		dbgprintk(KERN_INFO "NETDEV_GOING_DOWN: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_CHANGENAME:
		dbgprintk(KERN_INFO "NETDEV_CHANGENAME: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_FEAT_CHANGE:
		dbgprintk(KERN_INFO "NETDEV_FEAT_CHANGE: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_BONDING_FAILOVER:
		dbgprintk(KERN_INFO "NETDEV_BONDING_FAILOVER: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_PRE_UP:
		dbgprintk(KERN_INFO "NETDEV_PRE_UP: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_PRE_TYPE_CHANGE:
		dbgprintk(KERN_INFO "NETDEV_PRE_TYPE_CHANGE: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_POST_TYPE_CHANGE:
		dbgprintk(KERN_INFO "NETDEV_POST_TYPE_CHANGE: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_POST_INIT:
		dbgprintk(KERN_INFO "NETDEV_POST_INIT: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_RELEASE:
		dbgprintk(KERN_INFO "NETDEV_RELEASE: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_NOTIFY_PEERS:
		dbgprintk(KERN_INFO "NETDEV_NOTIFY_PEERS: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	case NETDEV_JOIN:
		dbgprintk(KERN_INFO "NETDEV_JOIN: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	default:
		dbgprintk(KERN_INFO "UNKNOWN_NOTIFY_EVENT: dev=%s\n",
			  devp ? devp->name : "none");
		break;
	}
#endif

	/* call handler for each ports */
	vsw_read_lock(&vswitch_rwlock, NULL);
	list_for_each (position, &vswitch_port_list) {
		entry = list_entry(position, pport_t, list);
		//
		//
		//
	}
	vsw_read_unlock(&vswitch_rwlock, NULL);
	return NOTIFY_DONE;
}

