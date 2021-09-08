/*
 *
 *	Detect following events of network by NetLink socket.
 *
 *		add/delete/modify device
 *		add/delete ip address
 *		add/delete routing table
 *		add/delete neighbor table
 *		add/delete ipv4 rule
 *		receive ipv6 prefix
 *
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<linux/version.h>
#if (defined(RHEL_MAJOR) && RHEL_MAJOR >= 8)
#include	<net/if.h>
#endif
#include	<arpa/inet.h>
#include	<poll.h>
#include	<time.h>
#include	<sys/time.h>
#include	<errno.h>
#include	<linux/netlink.h>
#include	<linux/rtnetlink.h>
#include	<linux/neighbour.h>
#include	<linux/fib_rules.h>
#include	<linux/if_link.h>
#include	<netinet/ether.h>
#include	<linux/if.h>

void	watch_event(int);
void	show_event(struct nlmsghdr*);
int	get_rtattr(struct rtattr**, int, void*, int);

int
main()
{
	int			socknl;
	int			alen ;
	struct sockaddr_nl	local = {0};

	socknl = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (socknl < 0) {
		exit(1);
	}
	local.nl_family = AF_NETLINK;
	local.nl_groups = RTMGRP_LINK | RTMGRP_NOTIFY | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_IPV4_MROUTE | RTMGRP_IPV4_RULE | RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_MROUTE | RTMGRP_IPV6_IFINFO | RTMGRP_IPV6_PREFIX | RTMGRP_NEIGH;
	if (bind(socknl, (struct sockaddr*)&local, sizeof(local)) < 0) {
		close(socknl);
		exit(1);
	}
	alen = sizeof(local);
	if (getsockname(socknl, (struct sockaddr*)&local, &alen) < 0 ||
            alen != sizeof(local) || local.nl_family != AF_NETLINK) {
		close(socknl);
		exit(1);
	}

	watch_event(socknl);
	close(socknl);
	return 0;
}

void	watch_event(int socknl)
{
	int		nfd;
	int		n;
	struct pollfd	pfd;
	char		buff[4096] = {0};
	struct nlmsghdr	*hdr;
	int		rt;
	int		len;

	pfd.fd = socknl;
	pfd.events = POLLIN;
	pfd.revents = 0;
	while (1) {
		n = poll(&pfd, 1, -1);
		len = recv(socknl, buff, sizeof(buff), 0);
		if (len < 0) {
			break;
		}
		for (hdr = (struct nlmsghdr*)buff; NLMSG_OK(hdr,len); hdr=NLMSG_NEXT(hdr,len)) {
			show_event(hdr);
		}  
	}
	return;
}

void	show_event(struct nlmsghdr *hdr) {
	struct timeval		tsu;
	struct tm		tmv;
	char			ifname[IFNAMSIZ] = {0};
	char			addr[64] = {0};
	char			local[64] = {0};
	struct rtattr		*rta = NULL;
	struct ifinfomsg	*ifi = NULL;
	struct ifaddrmsg	*ifa = NULL;
	struct rtattr*		rtas[IFA_MAX + 1] = {0};
	struct rtmsg		*rtm = NULL;
	struct rtattr*		rtrs[RTA_MAX + 1] = {0};
	struct ndmsg		*ndm = NULL;
	struct rtattr*		rtns[NDA_MAX + 1] = {0};
	struct fib_rule_hdr	*frh = NULL;
	struct rtattr*		rtfs[FRA_MAX + 1] = {0};
	struct prefixmsg	*prefix = NULL;
	struct rtattr*		rtps[PREFIX_MAX + 1] = {0};
	void			*src = NULL;
	int			enable_local = 0;
	int			index;
	struct ether_addr	*etha = NULL;

	gettimeofday (&tsu, NULL);
	memcpy (&tmv, localtime ((time_t *) & tsu.tv_sec), sizeof (struct tm));
	printf("[%02d:%02d:%02d.%03d] ", tmv.tm_hour, tmv.tm_min, tmv.tm_sec, (int)tsu.tv_usec/1000);

	switch(  hdr->nlmsg_type ) {
	case RTM_NEWLINK:
	case RTM_DELLINK:
		/* New/Delete LINK Status */
		ifi = NLMSG_DATA(hdr);
		if_indextoname(ifi->ifi_index,ifname);
		printf("%s:  dev=%s", hdr->nlmsg_type==RTM_NEWLINK ? "new link":"del link", ifname);
		printf(" flags=%x", ifi->ifi_flags);
		printf(" change=%d", ifi->ifi_change);
		printf("\n");
		break;

	case RTM_NEWADDR:
	case RTM_DELADDR:
		/* New/Delete IP Address */
		ifa = NLMSG_DATA(hdr);
		if_indextoname(ifa->ifa_index,ifname);
		printf("%s:  dev=%s", hdr->nlmsg_type==RTM_NEWADDR ? "new addr":"del addr", ifname);
		get_rtattr(rtas, IFA_MAX, IFA_RTA(ifa), RTM_PAYLOAD(hdr));
		if (rtas[IFA_ADDRESS]) {
			src = RTA_DATA(rtas[IFA_ADDRESS]);
			inet_ntop(ifa->ifa_family, src, addr, sizeof(addr));	
		}
		if (enable_local && rtas[IFA_LOCAL]) {
			src = RTA_DATA(rtas[IFA_LOCAL]);
			inet_ntop(ifa->ifa_family, src, local, sizeof(local));	
			if (strcmp(local, addr) && local[0] != '\0') {
				printf(" local=%s", local);
			}
		}
		printf(" address=%s", addr);
		printf("\n");
		break;

	case RTM_NEWROUTE:
	case RTM_DELROUTE:
		/* New/Delete Routeing Table */
		rtm = NLMSG_DATA(hdr);
		rta = (struct rtattr *) RTM_RTA(rtm);
		get_rtattr(rtrs, RTA_MAX, rta,  RTM_PAYLOAD(hdr));
		if (rtrs[RTA_OIF]) {
			index = *(int*)RTA_DATA(rtrs[RTA_OIF]);
			if_indextoname(index,ifname);
		} else {
			strncpy(ifname, "unspec", sizeof(ifname - 1));
		}
		printf("%s: dev=%s", hdr->nlmsg_type==RTM_NEWROUTE ? "new route":"del route", ifname);
		if (rtrs[RTA_DST]) {
			inet_ntop(rtm->rtm_family, RTA_DATA(rtrs[RTA_DST]), addr, sizeof(addr));
			printf(" dst=%s", addr);
		}
		if (rtrs[RTA_GATEWAY]) {
			inet_ntop(rtm->rtm_family, RTA_DATA(rtrs[RTA_GATEWAY]), addr, sizeof(addr));
			printf(" gateway=%s", addr);
		}
		printf("\n");
		break;

	case RTM_NEWNEIGH:
	case RTM_DELNEIGH:
		/* New/Delete Neighbor Table */
		ndm =  NLMSG_DATA(hdr);
		if_indextoname(ndm->ndm_ifindex,ifname);
		printf("%s: dev=%s", hdr->nlmsg_type==RTM_NEWNEIGH ? "new neigh":"del neigh", ifname);
		rta = (struct rtattr *) RTM_RTA(ndm);
		get_rtattr(rtns, RTA_MAX, rta, RTM_PAYLOAD(hdr));
		if (rtns[NDA_DST]) {
			inet_ntop(ndm->ndm_family, RTA_DATA(rtns[NDA_DST]), addr, sizeof(addr));
			printf(" dst=%s", addr);
		}
		if (rtns[NDA_LLADDR]) {
			etha = (struct ether_addr*)RTA_DATA(rtns[NDA_LLADDR]);
			printf(" laddr=%s", ether_ntoa(etha));
		}
		if (ndm->ndm_state & NUD_INCOMPLETE) {
			printf(" incomplete");
		}
		if (ndm->ndm_state & NUD_REACHABLE) {
			printf(" reachable");
		}
		if (ndm->ndm_state & NUD_STALE) {
			printf(" stale");
		}
		if (ndm->ndm_state & NUD_DELAY) {
			printf(" delay");
		}
		if (ndm->ndm_state & NUD_PROBE) {
			printf(" probe");
		}
		if (ndm->ndm_state & NUD_FAILED) {
			printf(" failed");
		}
		printf("\n");
		break;

	case RTM_NEWRULE:
	case RTM_DELRULE:
		frh = NLMSG_DATA(hdr);
		rta = (struct rtattr *) RTM_RTA(frh);
		get_rtattr(rtfs, FRA_MAX, rta, RTM_PAYLOAD(hdr));
		index = -1;
		if (rtfs[FRA_TABLE]) {
			index = *(int*)RTA_DATA(rtfs[FRA_TABLE]);
		}
		printf("%s:  table=%d", hdr->nlmsg_type==RTM_NEWRULE ? "new rule":"del rule", index);
		if (rtfs[FRA_PRIORITY]) {
			index = *(int*)RTA_DATA(rtfs[FRA_PRIORITY]);
			printf(" priority=%d", index);
		}
		if (rtfs[FRA_SRC]) {
			inet_ntop(frh->family, RTA_DATA(rtfs[FRA_SRC]), addr, sizeof(addr));
			printf(" src=%s", addr);
		}
		if (rtfs[FRA_DST]) {
			inet_ntop(frh->family, RTA_DATA(rtfs[FRA_DST]), addr, sizeof(addr));
			printf(" dst=%s", addr);
		}
		printf("\n");
		break;

	case RTM_NEWPREFIX:
		prefix = NLMSG_DATA(hdr);
		if_indextoname(prefix->prefix_ifindex,ifname);
		rta = (struct rtattr *) RTM_RTA(prefix);
		get_rtattr(rtps, PREFIX_MAX, rta, RTM_PAYLOAD(hdr));
		printf("new pref:  dev=%s", ifname);
		if (rtps[PREFIX_ADDRESS]) {
			inet_ntop(prefix->prefix_family, RTA_DATA(rtps[PREFIX_ADDRESS]), addr, sizeof(addr));
			printf(" prefix=%s", addr);
		}
		printf("\n");
		break;

	case RTM_NEWQDISC:
	case RTM_DELQDISC:
	case RTM_NEWTCLASS:
	case RTM_DELTCLASS:
	case RTM_NEWTFILTER:
	case RTM_DELTFILTER:
	case RTM_NEWACTION:
	case RTM_DELACTION:
	case RTM_NEWNEIGHTBL:
	case RTM_NEWNDUSEROPT:
	case RTM_NEWADDRLABEL:
	case RTM_DELADDRLABEL:
	case RTM_NEWNETCONF:
#if (defined(RHEL_MAJOR) && RHEL_MAJOR >= 8)
	case RTM_DELNETCONF:
#endif
	case RTM_NEWMDB:
	case RTM_DELMDB:
	case RTM_NEWNSID:
	case RTM_DELNSID:
	case RTM_NEWSTATS:
#if (defined(RHEL_MAJOR) && RHEL_MAJOR >= 8)
	case RTM_NEWCACHEREPORT:
	case RTM_NEWCHAIN:
	case RTM_DELCHAIN:
#endif
	default:
		printf("unknown: type=%d: %s\n", hdr->nlmsg_type);
		break;
	}
	return;
}

int
get_rtattr(struct rtattr **rtas, int max, void *ent, int len)
{
	int		cnt = 0;
	struct rtattr	*rta = ent;

	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max) {
			cnt++;
			rtas[rta->rta_type] = rta;
		}
		rta = RTA_NEXT(rta, len);
	}
	return cnt;
}

