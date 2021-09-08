/*
 *
 *	Test program for RAW Ether socket.
 *
 */

#include	<stdio.h>
#include	<stdint.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<sys/unistd.h>
#include	<sys/ioctl.h>
#include	<netinet/in.h>
#include	<linux/ip.h>
#include	<linux/if.h>
#include	<linux/if_ether.h>
#include	<net/ethernet.h>
#include	<netinet/if_ether.h>
#include	<linux/if_packet.h>
#include	<poll.h>
#include	<netinet/ether.h>
#include	<arpa/inet.h>

#define	VLAN_VID_MASK	0xfff
#define	ETH_P_DUMMY	0x2020

void	print_packet(u_char*, int);
int	init_socket(char*, struct sockaddr_ll*);
int	sndrcv_packet(int, int, char*, char*, struct sockaddr_ll*);
int	get_hwaddr(int fd, char*, struct ether_addr*);

int	debug = 1;

int main(int argc, char **argv)
{
	int			fd_rcv, fd_snd;
	int			i;
	char			*ifname_snd;
	char			*ifname_rcv;
	int			if_index;
	struct ifreq		req;
	struct sockaddr_ll	sock_snd;
	struct sockaddr_ll	sock_rcv;
	int			rlen = 0;
	struct msghdr		msg;
	struct iovec		iov;
	union {
		struct cmsghdr	cmsg;
		char		buf[256];
	} cmsg_buf;
	u_char			data[2048];

	if (argc != 3) {
		exit(1);
	}
	if (strcmp(argv[1],argv[2]) == 0) {
		exit(1);
	}
	ifname_snd = argv[1];
	ifname_rcv = argv[2];

	memset(&sock_snd,0,sizeof(sock_snd));
	sock_snd.sll_family = PF_PACKET;
	sock_snd.sll_protocol = htons(ETH_P_ALL);
	fd_snd = init_socket(ifname_snd, &sock_snd);
	if (fd_snd < 0) {
		exit(1);
	}
	memset(&sock_rcv,0,sizeof(sock_rcv));
	sock_rcv.sll_family = PF_PACKET;
	sock_rcv.sll_protocol = htons(ETH_P_ALL);
	fd_rcv = init_socket(ifname_rcv, &sock_rcv);
	if (fd_rcv < 0) {
		close(fd_snd);
		exit(1);
	}

	if (sndrcv_packet(fd_snd, fd_rcv, ifname_snd, ifname_rcv, &sock_rcv) < 0) {
		close(fd_rcv);
		close(fd_snd);
		exit(1);
	}

	close(fd_rcv);
	close(fd_snd);
	exit(0);
}

int
init_socket(
	char*			ifname,
	struct sockaddr_ll*	sock
)
{
	int		fd;
	struct ifreq	req;
	int		if_index;

	fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd < 0) {
		fprintf(stderr,"socket=%d\n",errno);
		perror("socket");
		exit(1);
	}
	memset(&req,0,sizeof(req));
	strncpy(req.ifr_name,ifname,IFNAMSIZ);
	if (ioctl(fd,SIOCGIFINDEX,&req) < 0) {
		fprintf(stderr,"ioctl=%d\n",errno);
		perror("SIOCGIFINDEX");
		close(fd);
		return -1;
	}
	sock->sll_ifindex = req.ifr_ifindex;
	if (bind(fd,(struct sockaddr*)sock,sizeof(struct sockaddr_ll)) < 0) {
		fprintf(stderr,"bind=%d\n",errno);
		perror("bind");
		close(fd);
		return -1;
	}
	return fd;
}

int
get_hwaddr(int fd, char* ifname, struct ether_addr* addr) {
	struct ifreq   ifrq;

	if (ifname == NULL || addr == NULL) {
		return -1;
	}
	memset(&ifrq,0,sizeof(ifrq));
	strncpy(ifrq.ifr_name,ifname,IFNAMSIZ);
	if (ioctl(fd,SIOCGIFHWADDR,&ifrq) < 0) {
		perror("ioctl(SIOCGIFHWADDR)");
	return -1;
	}
	memcpy(addr,&ifrq.ifr_hwaddr.sa_data,ETH_ALEN);
	//printf("SIOGGIFHWADDR(%s) = %s\n", ifname, ether_ntoa(addr));
	return 0;
}

int
sndrcv_packet(
	int			fd_snd,
	int			fd_rcv,
	char*			ifname_snd,
	char*			ifname_rcv,
	struct sockaddr_ll*	sock_rcv
)
{
	int			len = 0;
	int			pktlen = 0;
	struct msghdr		msg;
	struct iovec		iov;
	union {
		struct cmsghdr	cmsg;
		char		buf[256];
	} cmsg_buf;
	u_char			data[2048];
	u_char			packet[256];
	struct ethhdr*		ether;
	unsigned char*		p;
	
	msg.msg_name = sock_rcv;
	msg.msg_namelen = sizeof(struct sockaddr_ll);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &cmsg_buf;
	msg.msg_controllen = sizeof(cmsg_buf);
	msg.msg_flags = 0;
	iov.iov_len = sizeof(data);
	iov.iov_base = data;


	while (len != -1) {
		p = packet;
		ether = (struct ethhdr*)packet;
		get_hwaddr(fd_rcv, ifname_rcv, (struct ether_addr*)&ether->h_dest);
		get_hwaddr(fd_snd, ifname_snd, (struct ether_addr*)&ether->h_source);
		ether->h_proto = htons(ETH_P_DUMMY);
		p += sizeof(struct ethhdr);
		memset(p,0x01,60);
		pktlen =  50 + sizeof(struct ethhdr);
		len = write(fd_snd, packet, pktlen);
		if (len < 0) {
			perror("write");
			return -1;
		}
		printf("write(%d, %d) = %d\n", fd_snd, pktlen, len);
		if (debug) {
			int	i;
			p = (char*)ether;
			for (i = 0; i < 16; i++) {
				printf("%02x",p[i]);
				if (i == 5 || i == 11 || i == 13) {
					printf(" ");
				}
			}
			printf("\n");
		}

		len = recvmsg(fd_rcv,&msg,MSG_TRUNC);
		printf("recvmsg(%d) =  %d\n", fd_rcv, len);
		if (len < 0) {
			perror("recvmsg");
			continue;
		}
		print_packet(data,sizeof(data));
		sleep(1);
	}
	perror("recvmsg");
	return 0;
}

void
print_packet(u_char *buff, int len) {
	unsigned short		proto;
	unsigned short*		vlan_tci = NULL;
	struct ethhdr*		ether;
	unsigned char*		p;
	struct iphdr*		ip;
	struct ether_arp*	arp;
	struct in_addr		ip_addr;
	struct ether_addr	ether_addr;
	char			ip1[16];
	char			ip2[16];
	char			mac1[32];
	char			mac2[32];

	if (len < sizeof(struct ethhdr)) {
		return;
	}
	ether = (struct ethhdr*)buff;
	p = buff + sizeof(struct ethhdr);

	proto = ntohs(ether->h_proto);
	if (proto == ETH_P_8021Q) {
		vlan_tci = (unsigned short*)p;
		proto = htons(*vlan_tci) & VLAN_VID_MASK;
		p += sizeof(unsigned short);
	}
	memcpy(&ether_addr,(char*)ether->h_source,sizeof(ether_addr));
	strncpy(mac1,(char*)ether_ntoa(&ether_addr),sizeof(mac1) - 1),
	memcpy(&ether_addr,(char*)ether->h_dest,sizeof(ether_addr));
	strncpy(mac2,(char*)ether_ntoa(&ether_addr),sizeof(mac2) - 1);
	if (vlan_tci && *vlan_tci) {
		printf("%s -> %s type = %04x/%04x\n", mac1, mac2,
			proto, ntohs(*vlan_tci) & VLAN_VID_MASK);
	} else {
		printf("%s -> %s type = %04x\n", mac1, mac2, proto);
	}
	if (debug) {
		int	i;
		p = (char*)ether;
		for (i = 0; i < 16; i++) {
			printf("%02x",p[i]);
			if (i == 5 || i == 11 || i == 13) {
				printf(" ");
			} else if (vlan_tci && *vlan_tci && i == 15) {
				printf(" ");
			}
		}
		printf("\n");
	}
}

