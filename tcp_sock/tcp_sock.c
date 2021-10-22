/*
 *
 *	TCP Socket sample.
 *
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/uio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<sys/select.h>
#include	<sys/unistd.h>
#include	<fcntl.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<arpa/inet.h>
#include	<sys/utsname.h>
#include	<signal.h>
#include	<poll.h>
#include	<string.h>
#include	<strings.h>
#include	<arpa/inet.h>
#include	<errno.h>


#define BUFLEN	1024
char	rbuff[BUFLEN] = { 0 };
char	sbuff[BUFLEN] = { 0 };
char	nodename[SYS_NMLN+1];
char	svname[] = "tcptps";
char	clname[] = "tcptpc";
int	serv_port = 9999;
char	serv_addr_str[] = "127.0.0.1";
char	message[] = "Hello TCP!";

int tcp_sock_server();
int tcp_sock_client();

int
main( argc, argv )
        int     argc;
        char    *argv[];
{
	printf("%s\n", argv[0]);
	if (strstr(argv[0], svname) != NULL) {
		tcp_sock_server();
	} else {
		tcp_sock_client();
	}
	exit(0);
}

/* Server */

int
tcp_sock_server()
{
	int			sockfd, newsockfd, clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf( "failed to socket\n" );
		exit(1);
	}
	memset( (char *)&serv_addr, 0, sizeof(serv_addr) );
	serv_addr.sin_family            = AF_INET;
	serv_addr.sin_addr.s_addr       = htonl( INADDR_ANY );
	serv_addr.sin_port              = htons( serv_port );
	if ( bind( sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0 ) {
		printf( "failed to bind\n" );
		exit(1);
        }
	listen( sockfd, 5 );
	clilen = sizeof(cli_addr);
	newsockfd = accept( sockfd, (struct sockaddr *)&cli_addr, &clilen );
	if ( newsockfd < 0 ) {
		printf( "failed to accept %d\n", errno );
		exit(1);
	}
	if ( recv( newsockfd, rbuff, BUFLEN, 0 ) < 0 ) {
		printf( "failed to recv %d\n", errno );
		close( newsockfd );
		close(sockfd);
		return -1;
	}
	printf( "message raceved : %s\n", rbuff );
        close( newsockfd );
	close(sockfd);
	return 0;
}

/* Client */

int tcp_sock_client()
{
	int			sockfd;
	struct sockaddr_in	serv_addr;
	struct in_addr		addr;

	inet_aton(serv_addr_str, &addr);
	memset( (char *)&serv_addr, 0, sizeof(serv_addr) );
	serv_addr.sin_family            = AF_INET;
	serv_addr.sin_addr.s_addr       = addr.s_addr;
	serv_addr.sin_port              = htons( serv_port );
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf( "failed to socket\n" );
		exit(1);
	}
	if ( connect( sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0 ) {
		printf( "failed to connect errno = %d\n", errno );
	}

	strncpy(sbuff, message, sizeof(rbuff) - 1);
	if ( send( sockfd, sbuff, strlen(sbuff), 0 ) < 0 ) {
		printf( "failed to send %d\n", errno );
		close( sockfd );
		return -1;
	}
	printf( "message sent : %s\n", sbuff );
	close(sockfd);
	return 0;
}

