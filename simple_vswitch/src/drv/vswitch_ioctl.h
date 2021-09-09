/*
 *
 *      I/O control definition of simple_vswitch
 *
 *              File:           vswitch_ioctl.h
 *              Date:           2021.09.09
 *              Auther:         T.Sayama
 *
 */

#ifndef _VSWITCH_IOCTL_H
#define _VSWITCH_IOCTL_H

#include	<linux/ioctl.h>
#ifdef	__KERNEL__
#include	<linux/if.h>
#else
#include	<net/if.h>
#endif

/***
 *	Typedefs
 */
typedef union	vsw_ioc_value		vsw_ioc_value_t;
typedef struct	vsw_ioctl_cmd		vsw_ioctl_cmd_t;
typedef struct	vsw_ioctl_get_ports	vsw_ioctl_get_ports_t;

#define	VSWITCH_PORT_MAX	24
#define	VSWITCH_FDB_MAX		256

/***
 *	Structures
 */
union vsw_ioc_value {
	u_short		usdata;
	u_int		uidata;
	u_long		uldata;
	u_char		uc8data[8];
	bool		bdata;
};

struct vsw_ioctl_cmd {
	u_int		type;
	//u_int		index;
	vsw_ioc_value_t	value;
	char		portname[IFNAMSIZ];
};

struct vsw_ioctl_get_ports {
	u_int		type;
	//u_int		index;
	vsw_ioc_value_t	value;
	u_int		portnum;
	char		portname[VSWITCH_PORT_MAX][IFNAMSIZ];
};

#define IOC_MAGIC 'v'

#define VSWIOC_ADD_PORT		_IOR(IOC_MAGIC, 1,   vsw_ioctl_cmd_t)
#define VSWIOC_DELETE_PORT	_IOR(IOC_MAGIC, 2,   vsw_ioctl_cmd_t)
#define VSWIOC_GET_PORTS	_IOR(IOC_MAGIC, 4,   vsw_ioctl_cmd_t)

long	ioc_add_port(char *);
long	ioc_delete_port(char *);
long	ioc_get_ports(vsw_ioctl_get_ports_t*);

#endif

