#ifndef _VSWCONF_H
#define _VSWCONF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define VSWITCH_DEV	"/dev/vswitch"

#define VSWCONFIG_OK	0
#define VSWCONFIG_NG	1

#endif	/* _VSWCONF_H */

