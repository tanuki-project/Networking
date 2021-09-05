/*
 *
 * 
 *
 */

#ifndef _VSWITCH_H
#define	_VSWITCH_H

#include        <linux/init.h>
#include        <linux/module.h>
#include        <linux/types.h>
#include        <linux/kernel.h>
#include        <linux/fs.h>
#include        <linux/cdev.h>
#include        <linux/device.h>
#include        <linux/poll.h>
#include        <linux/slab.h>
#include        <linux/in.h>
#include        <linux/in6.h>
#include        <linux/netdevice.h>
#include        <linux/skbuff.h>
#include        <linux/etherdevice.h>
#include        <linux/rtnetlink.h>
#include        <linux/timer.h>
#include        <linux/sched.h>
#include        <linux/delay.h>
#include        <linux/if_vlan.h>
#include        <linux/ip.h>
#include        <linux/ioctl.h>
#include        <linux/kthread.h>
#include        <linux/version.h>
#include        <asm/uaccess.h>

#define	VSWITCH_DEVNAME		"vswitch"
#define	VSWITCH_VERSION		"1.0"

#ifdef DEBUG
#define dbgprintk(fmt, ...)     if (debug_level) printk(fmt, ##__VA_ARGS__)
#else
#define dbgprintk(fmt, ...)
#endif  /* DEBUG */

#define	VSWITCH_TIMER_INTERVAL	10000
#define FDB_DEFAULT_TIMER	180
#define FDB_TIMER_INTERVAL	10

int		vswitch_init(void);
void		vswitch_exit(void);
int		vswitch_open(struct inode *, struct file *);
int		vswitch_close(struct inode *, struct file *);
long		vswitch_ioctl(struct file *, u_int, u_long);
ssize_t		vswitch_read(struct file *, char *, size_t, loff_t *);

typedef struct	vsw_rwlock		vsw_rwlock_t;

struct	vsw_rwlock {
	rwlock_t	lock;
	atomic_t	preemption;
};

typedef	struct	pport		pport_t;

struct pport {
	struct list_head	list;
	char			portname[IFNAMSIZ];
	int			link_state;
	struct net_device*	devp;
};

/* FDB entry */
typedef struct	vswitch_fdb_entry	vswitch_fdb_entry_t;

struct  vswitch_fdb_entry {
	struct list_head	list;
	pport_t			*port;
	void			*data;
	int			countdown;
	u_char			ethaddr[ETH_ALEN];
};

pport_t *	find_port_by_name(char *);
pport_t *	find_port_by_dev(struct net_device *);

int	vswitch_rx_handler_register(struct net_device*, pport_t*);
void	vswitch_rx_handler_unregister(struct net_device*);
rx_handler_result_t	vswitch_rx(struct sk_buff **);
int	vswitch_forward(pport_t*, struct sk_buff*);

int	vswitch_set_fdb(u_char*, pport_t*);
vswitch_fdb_entry_t*	vswitch_search_fdb(u_char *);
void	vswitch_remove_fdb_list(void);
void	vswitch_countdown_fdb(void);
void	vswitch_disable_fdb(pport_t*);

static inline void
vsw_rwlock_init(vsw_rwlock_t *lock)
{
	rwlock_init(&lock->lock);
	atomic_set(&lock->preemption, 0);
	return;
}

static inline void
vsw_read_lock(vsw_rwlock_t *lock, u_long *eflags)
{
	while (atomic_read(&lock->preemption) > 0) {
		;
	}
	if (eflags) {
		local_irq_save(*eflags);
	}
	read_lock(&lock->lock);
	return;
}

static inline void
vsw_read_unlock(vsw_rwlock_t *lock, u_long *eflags)
{
	read_unlock(&lock->lock);
	if (eflags) {
		local_irq_restore(*eflags);
	}
	return;
}

static inline void
vsw_write_lock(vsw_rwlock_t *lock, u_long *eflags)
{
	while (atomic_read(&lock->preemption) > 0) {
		;
	}
	if (eflags) {
		local_irq_save(*eflags);
	}
	write_lock(&lock->lock);
	return;
}

static inline void
vsw_write_unlock(vsw_rwlock_t *lock, u_long *eflags)
{
	write_unlock(&lock->lock);
	if (eflags) {
		local_irq_restore(*eflags);
	}
	return;
}

static inline void
vsw_write_release(vsw_rwlock_t *lock, u_long *eflags)
{
	atomic_inc(&lock->preemption);
	write_unlock(&lock->lock);
	if (eflags) {
		local_irq_restore(*eflags);
	}
	return;
}

static inline void
vsw_write_reacquire(vsw_rwlock_t *lock, u_long *eflags)
{
	if (eflags) {
		local_irq_save(*eflags);
	}
	write_lock(&lock->lock);
	atomic_dec(&lock->preemption);
	return;
}

#endif
