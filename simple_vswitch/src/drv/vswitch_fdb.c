/*
 *
 *      Forwarding database for simple_vswitch
 *
 *              File:           vswitch_fdb.c
 *              Date:           2021.09.09
 *              Auther:         T.Sayama
 *
 */

#include        "vswitch.h"
#include        "vswitch_ioctl.h"

static int			debug_level = 1;
#define	MAC_FMT			"%02x:%02x:%02x:%02x:%02x:%02x"

extern struct list_head		vswitch_fdb_list;
extern int			vswitch_fdb_num;

/* set or update fdb entry. */

int
vswitch_set_fdb(
	u_char		*addr,
	pport_t		*port
)
{
	vswitch_fdb_entry_t	*entry = NULL;
	vswitch_fdb_entry_t	*new_fdb  = NULL;
	char			mac_addr_str[32]  = { 0 };

	if (port == NULL || addr == NULL) {
		return -EINVAL;
	}
	sprintf(mac_addr_str, MAC_FMT, addr[0], addr[1], addr[2], addr[3],addr[4], addr[5]);
	dbgprintk(KERN_INFO "vswitch_set_fdb: %s %s", mac_addr_str, port->portname);
	entry = vswitch_search_fdb(addr);
	if (entry) {
		entry->port = port;
		entry->countdown = FDB_DEFAULT_TIMER;
		dbgprintk(KERN_INFO "vswitch_set_fdb: update");
	} else {
		if (vswitch_fdb_num >= VSWITCH_FDB_MAX) {
			return 0;
		}
		dbgprintk(KERN_INFO "vswitch_set_fdb: alloc");
		new_fdb = kmalloc(sizeof(vswitch_fdb_entry_t), GFP_ATOMIC);
		if (new_fdb == NULL) {
			return -1;
		}
		memset(new_fdb, 0, sizeof(vswitch_fdb_entry_t));
		memcpy(new_fdb->ethaddr, addr, ETH_ALEN);
		new_fdb->port = port;
		new_fdb->countdown  = FDB_DEFAULT_TIMER;
		list_add_tail(&new_fdb->list, &vswitch_fdb_list);
	}
	return 0;
}

/* search fdb entry. */

vswitch_fdb_entry_t*
vswitch_search_fdb(
	u_char	*addr
)
{
	u_char                  laddr[ETH_ALEN] = { 0 };
	vswitch_fdb_entry_t	*entry    = NULL;
	struct list_head	*position = NULL;
	char			mac_addr_str[32]  = { 0 };

	if (addr == NULL) {
		return NULL;
	}
	sprintf(mac_addr_str, MAC_FMT, addr[0], addr[1], addr[2], addr[3],addr[4], addr[5]);
	dbgprintk(KERN_INFO "vswitch_search_fdb %s", mac_addr_str);
	memcpy(&laddr, addr, ETH_ALEN);
	list_for_each (position, &vswitch_fdb_list) {
		entry = list_entry(position, vswitch_fdb_entry_t, list);
		if (memcmp(entry->ethaddr, laddr, ETH_ALEN) == 0) {
			dbgprintk(KERN_INFO "vswitch_search_fdb %s %s", mac_addr_str, entry->port->portname);
			return entry;
		}
	}
	return NULL;

}

/* decrement countdown timer and destruct expired entry. */

void
vswitch_countdown_fdb(void)
{
	vswitch_fdb_entry_t	*entry    = NULL;
	vswitch_fdb_entry_t	*free     = NULL;
	struct list_head	*position = NULL;
	char			mac_addr_str[32]  = { 0 };
	u_char			*addr;

	list_for_each (position, &vswitch_fdb_list) {
		if (free) {
			list_del(&free->list);
			kfree((void *)free);
			vswitch_fdb_num--;
			free = NULL;
			dbgprintk(KERN_INFO "vswitch_fdb_countdown: expired: %s", mac_addr_str);
		}
		entry = list_entry(position, vswitch_fdb_entry_t, list);
		if (entry->countdown > 0) {
			entry->countdown -= FDB_TIMER_INTERVAL;
		}
		if (entry->countdown <= 0) {
			free     = entry;
			addr = entry->ethaddr;
			sprintf(mac_addr_str, MAC_FMT, addr[0], addr[1], addr[2], addr[3],addr[4], addr[5]);
		}
	}
	if (free) {
		list_del(&free->list);
		kfree((void *)free);
		vswitch_fdb_num--;
		free = NULL;
		dbgprintk(KERN_INFO "vswitch_fdb_countdown: expired: %s", mac_addr_str);
	}
	return;
}

/* remove fdb list */

void    vswitch_remove_fdb_list(void)
{
	vswitch_fdb_entry_t	*entry    = NULL;
	struct list_head	*position = NULL;
	char			mac_addr_str[32]  = { 0 };
	u_char			*addr;

	while (!list_empty(&vswitch_fdb_list)) {
		position = vswitch_fdb_list.next;
		entry = list_entry(position, vswitch_fdb_entry_t, list);
		if (entry) {
			addr = entry->ethaddr;
			sprintf(mac_addr_str, MAC_FMT, addr[0], addr[1], addr[2], addr[3],addr[4], addr[5]);
			dbgprintk(KERN_INFO "vswitch_remove_fdb_list: %s", mac_addr_str);
			list_del(&entry->list);
			kfree(entry);
			vswitch_fdb_num--;
		}
	}
	return;
}

/* disable fdb crrespond with specified port */

void    vswitch_disable_fdb(pport_t *port)
{
	vswitch_fdb_entry_t	*entry    = NULL;
	struct list_head	*position = NULL;
	char			mac_addr_str[32]  = { 0 };
	u_char			*addr;

	dbgprintk(KERN_INFO "vswitch_disable_fdb");
	list_for_each (position, &vswitch_fdb_list) {
		entry = list_entry(position, vswitch_fdb_entry_t, list);
		if (entry && entry->port == port) {
			addr = entry->ethaddr;
			sprintf(mac_addr_str, MAC_FMT, addr[0], addr[1], addr[2], addr[3],addr[4], addr[5]);
			dbgprintk(KERN_INFO "vswitch_disable_fdb: %s", mac_addr_str);
			entry->countdown = 0;
		}
	}
	return;
}

