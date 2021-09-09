/*
 *
 *	L2 switching routine for simple_vswitch
 *
 *		File:		vswitch_packet.c
 *		Date:		2021.09.09
 *		Auther:		T.Sayama
 *
 */

#include	"vswitch.h"

extern vsw_rwlock_t		vswitch_rwlock;
extern struct list_head		vswitch_port_list;
extern int			vswitch_port_num;
extern struct list_head		vswitch_fdb_list;
extern int			vswitch_fdb_num;

static int	debug_level = 1;

const u_char	c_bcastaddr[] = {0xff,0xff,0xff,0xff,0xff,0xff};
const u_char	c_bpdu_addr[] = {0x01,0x80,0xc2,0x00,0x00,0x00};

/* register receive handler */

int
vswitch_rx_handler_register(struct net_device *devp, pport_t *port)
{
	int	rtn;
	if (devp == NULL || port == NULL) {
		return -EINVAL;
	}
	rtnl_lock();
	rtn = netdev_rx_handler_register(devp, vswitch_rx, port);
	if (rtn == 0) {
		dev_set_promiscuity(devp, 1);
	}
	dbgprintk(KERN_INFO "netdev_rx_handler_register: %s", devp->name);
	rtnl_unlock();
	return rtn;
}

/* unregister receive handler */

void
vswitch_rx_handler_unregister(struct net_device *devp)
{
	rtnl_lock();
	dev_set_promiscuity(devp, -1);
	netdev_rx_handler_unregister(devp);
	dbgprintk(KERN_INFO "netdev_rx_handler_unregister: %s", devp->name);
	rtnl_unlock();
}

/* recieve handler for port of vswitch */

rx_handler_result_t
vswitch_rx(struct sk_buff **pskb)
{
	struct sk_buff		*skb = NULL;
	struct net_device	*devp = NULL;
	pport_t			*port;
	struct ethhdr		*ethp = NULL;
	u_long			eflags    = 0;
	int			rtn= 0;

	if ( pskb == NULL || *pskb == NULL ) {
		return RX_HANDLER_PASS;
	}
	skb = *pskb;
	if (skb->pkt_type == PACKET_LOOPBACK) {
		return RX_HANDLER_PASS;
	}
	skb = skb_share_check(skb, GFP_ATOMIC);
	if (skb == NULL) {
		return RX_HANDLER_CONSUMED;;
	}
	skb_push(skb, ETH_HLEN);
	ethp = (struct ethhdr *)skb_mac_header(skb);
	if (skb_linearize(skb) != 0) {
		dev_kfree_skb(skb);
		return RX_HANDLER_CONSUMED;;
	}

	devp = skb->dev;
	port = (pport_t*)rcu_dereference(devp->rx_handler_data);
	if (port == NULL) {
		return RX_HANDLER_CONSUMED;;
	}
	vsw_write_lock(&vswitch_rwlock, &eflags);
	atomic_inc(&port->refcnt);
	dbgprintk(KERN_INFO "vswitch_rx: %s:%x shared:%d cloned:%d",
	    port->portname, ntohs(ethp->h_proto), skb_shared(skb), skb->cloned);

	/* learn MAC address and port on FDB */
	vswitch_set_fdb(ethp->h_source, port);
	vsw_write_unlock(&vswitch_rwlock, &eflags);

	/* drop not forwardable packet		*/
	/* STP, BPDU, LACP, LLDP, ...		*/
	if (memcmp(ethp->h_dest, c_bpdu_addr, 5) == 0 &&
	    ethp->h_dest[5] >= 0x00 && ethp->h_dest[5] <= 0x0f) {
		/* destination is 01:80:C2:00:00:0X	*/
		dev_kfree_skb(skb);
		vsw_write_lock(&vswitch_rwlock, &eflags);
		atomic_dec(&port->refcnt);
		vsw_write_unlock(&vswitch_rwlock, &eflags);
		return NET_XMIT_DROP;
	}

	rtn = vswitch_forward(port, skb);
	consume_skb(skb);
	vsw_write_lock(&vswitch_rwlock, &eflags);
	atomic_dec(&port->refcnt);
	vsw_write_unlock(&vswitch_rwlock, &eflags);
        return rtn;
}

/* foward or drop packet */

int
vswitch_forward(
	pport_t		*port,
	struct sk_buff	*skb
)
{
	struct list_head	*position = NULL;
	pport_t			*entry    = NULL;
	int			rc        = 0;
	u_long			eflags    = 0;
	struct sk_buff		*newskb   = NULL;
	struct ethhdr		*ethp = NULL;
	vswitch_fdb_entry_t	*fdb = NULL;

	if (port == NULL || skb == NULL) {
		if (skb) {
			dev_kfree_skb(skb);
		}
		return NET_XMIT_DROP;
	}

	ethp = (struct ethhdr *)skb_mac_header(skb);
	if (!(ethp->h_dest[0] & 0x01)) {
		/* search fdb when destination is unicast */
		vsw_read_lock(&vswitch_rwlock, &eflags);
		fdb = vswitch_search_fdb(ethp->h_dest);
		vsw_read_unlock(&vswitch_rwlock, &eflags);
	}
	if (fdb && fdb->countdown > 0) {
		/* foward packet to learned port. */
		newskb = skb_clone(skb, GFP_ATOMIC);
		if (newskb) {
			newskb->dev = fdb->port->devp;
			rc = dev_queue_xmit(newskb);
			dbgprintk(KERN_INFO "vswitch_forward: %s", fdb->port->portname);
		}
		return NET_XMIT_SUCCESS;
	}

	/* forward packet to all ports */
	vsw_read_lock(&vswitch_rwlock, &eflags);
	list_for_each (position, &vswitch_port_list) {
		entry = list_entry(position, pport_t, list);
		if (entry == port) {
			continue;
		}
		newskb = skb_clone(skb, GFP_ATOMIC);
		if (newskb) {
			newskb->dev = entry->devp;
			vsw_read_unlock(&vswitch_rwlock, &eflags);
			rc = dev_queue_xmit(newskb);
			vsw_read_lock(&vswitch_rwlock, &eflags);
			dbgprintk(KERN_INFO "vswitch_forward: %s", entry->portname);
		}
	}
	vsw_read_unlock(&vswitch_rwlock, &eflags);
	return NET_XMIT_SUCCESS;
}

