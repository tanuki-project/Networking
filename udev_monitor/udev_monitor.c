/*
 *
 *	test program for udev monitor
 *
 */

#include	<errno.h>
#include	<getopt.h>
#include	<sys/epoll.h>
#include	<poll.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<libudev.h>

#define	MONITOR_KERNEL	"kernel"
#define	MONITOR_UDEV	"kernel"

int
main()
{
	//_cleanup_(udev_unrefp) struct udev *udev = NULL;
	//_cleanup_(udev_monitor_unrefp) struct udev_monitor *udev_monitor;
	int	fd_udev1;
	int	fd_udev2;
	struct udev_monitor	*udev_monitor1 = NULL;
	struct udev_monitor	*udev_monitor2 = NULL;
	struct udev		*udev1 = NULL;
	struct udev		*udev2 = NULL;
	struct udev_device	*device;
	const char		*str;
	int			nfd;
	int			n, i;
	struct pollfd		pfd[2];

	udev1 = udev_new();
	udev2 = udev_new();
	udev_monitor1 = udev_monitor_new_from_netlink(udev1, "kernel");
	udev_monitor2 = udev_monitor_new_from_netlink(udev2, "udev");
	fd_udev1 = udev_monitor_get_fd(udev_monitor1);
	fd_udev2 = udev_monitor_get_fd(udev_monitor2);
	udev_monitor_enable_receiving(udev_monitor1);
	udev_monitor_enable_receiving(udev_monitor2);
	nfd = 2;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	pfd[0].fd = fd_udev1;
	pfd[1].events = POLLIN;
	pfd[1].revents = 0;
	pfd[1].fd = fd_udev2;
	while (1) {
		n = poll(&pfd[0], nfd, -1);
		if (n == 0) {
			continue;
		} else if (n < 0) {
			break;
		}
		for (i = 0; i < nfd; i++) {
			if (pfd[i].fd == fd_udev1) {
				device = udev_monitor_receive_device(udev_monitor1);
				if (device) {
					printf("KERNEL:\n");
				}
			} else if (pfd[i].fd == fd_udev2) {
				device = udev_monitor_receive_device(udev_monitor2);
				if (device) {
					printf("UDEV:\n");
				}
			}
			if (device) {
				str = udev_device_get_devtype(device);
				if (str)
					printf(" devtype:   '%s'\n", str);
				str = udev_device_get_action(device);
				if (str)
					printf(" action:    '%s'\n", str);
				str = udev_device_get_devpath(device);
				if (str)
       					printf(" devpath:   '%s'\n", str);
				str = udev_device_get_subsystem(device);
				if (str)
					printf(" subsystem: '%s'\n", str);
				str = udev_device_get_sysname(device);
				if (str)
					printf(" sysname:   '%s'\n", str);
				str = udev_device_get_sysnum(device);
				if (str)
					printf(" sysnum:    '%s'\n", str);
				str = udev_device_get_devnode(device);
				if (str)
					printf(" devnode:   '%s'\n", str);
				udev_device_unref(device);
				printf("\n");

			}
		}
	}
	udev_monitor_unref(udev_monitor1);
	udev_monitor_unref(udev_monitor2);
	udev_unref(udev1);
	udev_unref(udev2);
	return 0;
}

