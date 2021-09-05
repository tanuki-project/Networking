/******************************************************************************/
/*                                                                            */
/*    scanlldp : Recieve and Display LLDP Protocol Messages.                  */
/*                                                                            */
/*      Command Format                                                        */
/*                                                                            */
/*                 scanlldp device                                            */
/*                                                                            */
/*                     device: Spaecify a network interface name.             */
/*                             ex. eth0 eth1 ...                              */
/*                                                                            */
/*      File   :          scanlldp.c                                          */
/*      Date   :          2013.08.27                                          */
/*      Author :          T.Sayama                                            */
/*                                                                            */
/******************************************************************************/

#include        <stdio.h>
#include        <stdlib.h>
#include        <stdarg.h>
#include        <sys/types.h>
#include        <unistd.h>
#include        <string.h>
#include        <ctype.h>
#include        <errno.h>
#include        <time.h>
#include        <sys/time.h>
#include        <sys/socket.h>
#include        <sys/ioctl.h>
#include        <netinet/in.h>
#include	<netinet/in_systm.h>
#include	<netinet/ip.h>
#include	<arpa/inet.h>
#include	<netinet/ether.h>
#include        <net/if.h>
#include	<netinet/if_ether.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include        <linux/if_packet.h>
#include        <linux/if_ether.h>
#include	<linux/sockios.h>

#define	MAXDLBUFSIZE	8192
#define	DLBUFSIZE	8192
#define TCP_DATA_PRINT_LENGTH  100
#define ETHERTYPE_LLDP  0x88cc

#define	LLDP_TLV_TYPE_END		0
#define	LLDP_TLV_TYPE_CLASS_ID		1
#define	LLDP_TLV_TYPE_PORT_ID		2
#define	LLDP_TLV_TYPE_TTL		3
#define	LLDP_TLV_TYPE_PORT_DESC		4
#define	LLDP_TLV_TYPE_SYS_NAME		5
#define	LLDP_TLV_TYPE_SYS_DESC		6
#define	LLDP_TLV_TYPE_SYS_CAP		7
#define	LLDP_TLV_TYPE_ADDRESS		8
#define	LLDP_TLV_TYPE_ORGSPEC		127

#define	LLDP_ADDR_TYPE_INET4		1
#define	LLDP_ADDR_TYPE_INET6		2
#define	LLDP_ADDR_TYPE_MAC		6

#define	LLDP_ADDR_SUBTYPE_UNKNOWN	1
#define	LLDP_ADDR_SUBTYPE_INDEX		2
#define	LLDP_ADDR_SUBTYPE_PORTNUM	3

struct ether_addr	lldp_mac = {
	0x01,0x80,0xc2,0x00,0x00,0x0e};

int	init_socket(char *devname);

char    		buff[4096];	/* buff */
struct msghdr		msg;		/* msg */
struct iovec		iov;		/* iov */
struct sockaddr_ll	sock;		/* socket */
struct cmsghdr		cmsg;		/* cmsg */

void print_packet(caddr_t, int);

int
main(
int	argc,
char	*argv[]

)
{
	int	fd;
	int	flags = 0;
	char	devname[IFNAMSIZ];
	char	path[256];
	int	len;

	if (argc != 2) {
		printf("Usage: scanlldp device\n" );
		exit(1);
	}

	memset(devname,0,sizeof(devname));
	strncpy(devname,argv[1],IFNAMSIZ);
	fd = init_socket(devname);
	if (fd < 0) {
		fprintf(stderr, "failed to open device (%s): %s\n", path, strerror(errno));
		exit(1);
	}

	len = 0;
	while (len >= 0) {
		len = recvmsg(fd,&msg,MSG_TRUNC);
		print_packet(buff,len);
	}
	close(fd);
	exit(0);
}

int
init_socket(char *devname) {
	int                     fd;
	char                    ifname[32];
	struct ifreq            req;

	if (ifname == NULL) {
		return -1;
	}

        strcpy(ifname, devname);
        msg.msg_name = &sock;
        msg.msg_namelen = sizeof(struct sockaddr_ll);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = &cmsg;
        msg.msg_controllen = sizeof(cmsg);
        iov.iov_len = 4096;
        iov.iov_base = buff;

        sock.sll_family = PF_PACKET;
        sock.sll_protocol = htons(ETH_P_ALL);
        /*interface->sock.sll_protocol = htons(ETH_P_SHA);*/
	fd = socket(PF_PACKET, SOCK_RAW, htons(ETHERTYPE_LLDP));
	if (fd < 0) {
		return -1;
	}
	memset(&req,0,sizeof(req));
	strcpy(req.ifr_name,ifname);
	if (ioctl(fd,SIOCGIFINDEX,&req) < 0) {
		close(fd);
		return -1;
	}
	sock.sll_ifindex = req.ifr_ifindex;
	if (bind(fd,(struct sockaddr*)&sock,sizeof(struct sockaddr_ll)) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

void
print_packet(caddr_t buf, int len)
{
	struct ether_header	*ether;
	int			 etherlen;
	u_char			*p, *ap;
	char			tlv_head[2];
	int			tlv_type;
	int			tlv_length;
	int			lldplen;
	int			i, l;
	struct timeval		tsu;
	struct tm		*tmBuf;
	int			dump = 0;
	u_short			ttl;
	char			buff[DLBUFSIZE];
	char			abuf[INET6_ADDRSTRLEN+4];
	u_char			type;
	struct in_addr		ip;
	struct in6_addr		ip6;
	struct ether_addr	mac;
	time_t			lt;

	gettimeofday(&tsu,NULL);
	lt = tsu.tv_sec;
	tmBuf = localtime(&lt);

	etherlen = sizeof(struct ether_header);
	ether = (struct ether_header *)buf;

	if (htons(ether->ether_type) != ETHERTYPE_LLDP) {
		return;
	}	

	if (len < etherlen)
		return;

	printf("Arrived at : %04d/%02d/%02d %02d:%02d:%02d.%03d\n",
	    1900 + tmBuf->tm_year, tmBuf->tm_mon, tmBuf->tm_mday, tmBuf->tm_hour,
	    tmBuf->tm_min, tmBuf->tm_sec, tsu.tv_usec/1000);

	printf("\n----Ether Header----\n");
	printf("src addr    : %s\n",ether_ntoa((struct ether_addr*)ether->ether_shost));
	printf("dest addr   : %s\n",ether_ntoa((struct ether_addr*)ether->ether_dhost));
	printf("ether type  : 0x%x\n",htons(ether->ether_type));

	printf("\n-----LLDPDU------\n");
	p = (u_char*)buf;
	p += etherlen;
	lldplen = len - etherlen;

	while (lldplen > 0) {
		if (lldplen < sizeof(tlv_head)) {
			return;
		}
		tlv_head[0] = *p;
		p++;
		tlv_head[1] = *p;
		p++;
		tlv_type = tlv_head[0] & 0xfe;
		tlv_type = tlv_type >> 1;
		tlv_length = ((tlv_head[0] & 0x01) << 8) + tlv_head[1];
		printf("-------TLV-------\n");
		printf("Type    : 0x%x",tlv_type);
		buff[0] = 0;
		dump = 0;
		switch(tlv_type) {
		case LLDP_TLV_TYPE_END:
			printf("(END)\n");
			printf("Length  : 0x%x\n",tlv_length);
			break;
		case LLDP_TLV_TYPE_CLASS_ID:
			printf("(Class ID)\n");
			printf("Length  : 0x%x\n",tlv_length);
			dump = 1;
			break;
		case LLDP_TLV_TYPE_PORT_ID:
			printf("(Port ID)\n");
			printf("Length  : 0x%x\n",tlv_length);
			dump = 1;
			break;
		case LLDP_TLV_TYPE_TTL:
			printf("(TTL)\n");
			printf("Length  : 0x%x\n",tlv_length);
			memcpy(&ttl,p,sizeof(ttl));
			printf("Value   : %d\n",ttl);
			break;
		case LLDP_TLV_TYPE_PORT_DESC:
			printf("(Port Description)\n");
			printf("Length  : 0x%x\n",tlv_length);
			snprintf(buff,tlv_length+1,"%s",p);
			printf("Value   : %s\n",buff);
			break;
		case LLDP_TLV_TYPE_SYS_NAME:
			printf("(System Name)\n");
			printf("Length  : 0x%x\n",tlv_length);
			snprintf(buff,tlv_length+1,"%s",p);
			printf("Value   : %s\n",buff);
			break;
		case LLDP_TLV_TYPE_SYS_DESC:
			printf("(System Description)\n");
			printf("Length  : 0x%x\n",tlv_length);
			snprintf(buff,tlv_length+1,"%s",p);
			printf("Value   : %s\n",buff);
			break;
		case LLDP_TLV_TYPE_SYS_CAP:
			printf("(System Capability)\n");
			printf("Length  : 0x%x\n",tlv_length);
			dump = 1;
			break;
		case LLDP_TLV_TYPE_ADDRESS:
			printf("(Management Address)\n");
			printf("Length  : 0x%x\n",tlv_length);
			ap = p;
			ap++;
			type = *ap;
			ap++;
			printf("Address Type = %d\n",type);
			switch (type) {
			case LLDP_ADDR_TYPE_INET4:
				memcpy(&ip,ap,sizeof(ip));
				ap += sizeof(ip);
				if (inet_ntoa(ip)) {
					strcpy(abuf,(char*)inet_ntoa(ip));
					printf("Value   : Address = %s\n",abuf);
				}
				break;
			case LLDP_ADDR_TYPE_INET6:
				memcpy(&ip6,ap,sizeof(ip6));
				inet_ntop(AF_INET6,&ip6,abuf,sizeof(abuf));
				printf("Value   : Address = %s\n", abuf);
				ap += sizeof(ip6);
				break;
			case LLDP_ADDR_TYPE_MAC:
				memcpy(&mac,ap,sizeof(mac));
				ap += sizeof(mac);
				if (ether_ntoa(&mac)) {
					strcpy(abuf,(char*)ether_ntoa(&mac));
					printf("Value   : Address = %s\n",abuf);
				}
				//printf("Value   : Address = %s\n",ether_ntoa(mac));
				break;
			default:
				break;
			}

			type = *ap;
			ap++;
			switch (type) {
			case LLDP_ADDR_SUBTYPE_UNKNOWN:
				printf("        : Unknown Subtype (%02x%02x%02x%02x)\n",
				    ap[0],ap[1],ap[2],ap[3]);
				break;
			case LLDP_ADDR_SUBTYPE_INDEX:
				printf("        : ifIndex (%02x%02x%02x%02x)\n",
				    ap[0],ap[1],ap[2],ap[3]);
				break;
			case LLDP_ADDR_SUBTYPE_PORTNUM:
				printf("        : System port num (%02x%02x%02x%02x)\n",
				    ap[0],ap[1],ap[2],ap[3]);
				break;
			default:
				break;
			}
			ap += 4;
			type = *ap;
			ap++;
			ap += type;
			dump = 1;
			break;
		case LLDP_TLV_TYPE_ORGSPEC:
			printf("(Organization Specific)\n");
			printf("Length  : 0x%x\n",tlv_length);
			dump = 1;
			break;
		default:
			printf("(Unknown)\n");
			printf("Length  : 0x%x\n",tlv_length);
			dump = 1;
			break;
		}
		if (dump && tlv_length > 0) {
			printf("Value   : ");
			for (l = 0; l < tlv_length; l += 16) {
				if (l > 0) {
					printf("        : ");
				}
				for (i = 0; i < 16; i++) {
					if (i > 0 && (i % 4) == 0) {
						printf(" ");
					}
					if (l+i < tlv_length) {
						printf("%02x",p[l+i]);
					} else {
						printf("  ");
					}
				}
				printf("  ");
				for (i = 0; i < 16; i++) {
					if (l+i < tlv_length) {
						if(isprint(p[l+i])) {
							printf("%c",p[l+i]);
						} else {
							printf(".");
						}
					}
				}
				printf("\n");
			}
		}

		p += tlv_length;
		lldplen -= sizeof(tlv_head);
		lldplen -= tlv_length;
	}

	printf("\n");
	return;
}

