/*
 *
 *	Test program for packet capture.
 *
 */

#include	<stdio.h>
#include	<stdint.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<sys/ioctl.h>
#include	<netinet/in.h>
#include	<netinet/tcp.h>
#include	<netinet/udp.h>
#include	<linux/ip.h>
#include	<linux/if.h>
#include	<linux/if_ether.h>
#include	<net/ethernet.h>
#include	<netinet/if_ether.h>
#include	<linux/if_packet.h>
#include	<poll.h>
#include	<netinet/ether.h>
#include	<arpa/inet.h>
#include	<unistd.h>

#define	VLAN_VID_MASK	0xfff
#define	ETH_P_8021Q	0x8100

void	print_packet(u_char*, int);
int	init_socket(char*, struct sockaddr_ll*);
int	receive_packet(int, struct sockaddr_ll*);

int	debug = 0;

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

	if (argc != 2) {
		exit(1);
	}
	ifname_rcv = argv[1];
	memset(&sock_rcv,0,sizeof(sock_rcv));
	sock_rcv.sll_family = PF_PACKET;
	sock_rcv.sll_protocol = htons(ETH_P_ALL);
	fd_rcv = init_socket(ifname_rcv, &sock_rcv);
	if (fd_rcv < 0) {
		close(fd_snd);
		exit(1);
	}
	receive_packet(fd_rcv, &sock_rcv);
	close(fd_rcv);
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
receive_packet(
	int			fd_rcv,
	struct sockaddr_ll*	sock_rcv
)
{
	int			len = 0;
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
		len = recvmsg(fd_rcv,&msg,MSG_TRUNC);
		if (len < 0) {
			continue;
			perror("recvmsg");
		}
		print_packet(data,sizeof(data));
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
	struct tcphdr*		tcp;
	struct udphdr*		udp;
	int			iphl;
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
	switch (proto) {
	case ETH_P_IP:
		ip = (struct iphdr*)p;
		iphl = ip->ihl * 4;
		/* get source IP address */
		memcpy((char*)&ip_addr,(char*)&ip->saddr,sizeof(ip_addr));
		strncpy(ip1,(char*)inet_ntoa(ip_addr),sizeof(ip1));

		/* get destination IP address */
		memcpy((char*)&ip_addr,(char*)&ip->daddr,sizeof(ip_addr));
		strncpy(ip2,(char*)inet_ntoa(ip_addr),sizeof(ip1));

		switch (ip->protocol) {
		case IPPROTO_TCP:
			p += iphl;
			tcp = (struct tcphdr*)p;
			printf("%s:%d -> %s:%d TCP \n", ip1, ntohs(tcp->th_sport), ip2, ntohs(tcp->th_dport));
			break;
		case IPPROTO_UDP:
			p += iphl;
			udp = (struct udphdr*)p;
			printf("%s:%d -> %s:%d UDP \n", ip1, ntohs(udp->uh_sport), ip2, ntohs(udp->uh_dport));
			break;
		default:
			printf("%s -> %s type = %04x\n", ip1, ip2, ip->protocol);
			break;
		}
		break;
	case ETH_P_ARP:
		arp = (struct ether_arp*)p;

		/* get sender IP address */
		memcpy(&ip_addr,arp->arp_spa,sizeof(ip_addr));
		strncpy(ip1,(char*)inet_ntoa(ip_addr),sizeof(ip1));

		/* get target IP address */
		memcpy(&ip_addr,(char*)arp->arp_tpa,sizeof(ip_addr));
		strncpy(ip2,(char*)inet_ntoa(ip_addr),sizeof(ip1));

		/* get sender hardware address */
		memcpy(&ether_addr,(char*)arp->arp_sha,sizeof(ether_addr));
		strncpy(mac1,(char*)ether_ntoa(&ether_addr),sizeof(mac1)),

		/* get target hardware address */
		memcpy(&ether_addr,(char*)arp->arp_tha,sizeof(ether_addr));
		strncpy(mac2,(char*)ether_ntoa(&ether_addr),sizeof(mac2));

		if (ntohs(arp->arp_op) == ARPOP_REQUEST) {
			printf("ARP REQ(%d) %s/%s -> %s/%s\n",
				ntohs(arp->arp_op), mac1,ip1,mac2,ip2);
		} else if (ntohs(arp->arp_op) == ARPOP_REPLY) {
			printf("ARP REP(%d) %s/%s -> %s/%s\n",
				ntohs(arp->arp_op), mac1,ip1,mac2,ip2);
		}
		break;
	default:
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
		break;
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

