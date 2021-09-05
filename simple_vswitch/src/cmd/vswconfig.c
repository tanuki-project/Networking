/*
 * 
 *
 *
 * 
 */

#include	"vswconfig.h"
#include	"vswitch_ioctl.h"

#define OPTIONS			""
#define DESC_ADDPORT		"add port to vswitch."
#define DESC_DELPORT		"delete port from vswitch."
#define DESC_SHOW		"show configuration."

void	usage		(void);
int	docommand	(char *, char **, int);

int	addport		(int, char**);
int	delport		(int, char**);
int	show 		(int, char**);
int	vsw_ioctl	(int, void*);

static struct builtin {
	char	*name;
	int	(*func)(int, char **);
	char	*desc;
} commandtab[] = {
	{"add",		addport,     DESC_ADDPORT},
	{"delete",	delport,     DESC_DELPORT},
	{"show",        show,        DESC_SHOW},
	{NULL, NULL, NULL}
};

int
main(int argc, char *argv[])
{
	extern int	optind;
	extern int	opterr;
	extern char	*optarg;
	unsigned int	c;
	int		rc = 0;

	opterr = 0;
	while ((c = getopt(argc, argv, OPTIONS)) != EOF) {
		switch (c) {
		case '?':
		default:
			usage();
			exit(1);
		}
	}
	if (optind >= argc) {
		usage();
		exit(1);
	}

	rc = docommand(argv[optind], &argv[optind], argc - optind);
	if (rc != VSWCONFIG_OK) {
		return 1;
	}
	return 0;
}

void
usage(void)
{
	struct builtin	*tp = NULL;

	fprintf(stderr, "usage: vswconfig {command} [args]\n");
	fprintf(stderr, "        %-16s%s\n", "<command>", "<description>");
	for (tp = commandtab; tp->name; tp++) {
		if (tp->desc == NULL) {
			continue;
		}
		fprintf(stderr, "        %-16s%s\n", tp->name, tp->desc);
	}
	return;
}

int
addport(int argc, char *argv[])
{
	vsw_ioctl_cmd_t		cmd;
	int			rc = 0;

	if (argc != 2) {
		fprintf(stderr, "usage: vswconfig addport portname\n");
		return VSWCONFIG_NG;
	}

	memset(&cmd, 0, sizeof(cmd));
	strncpy(cmd.portname, argv[1], sizeof(cmd.portname) - 1);
	rc = vsw_ioctl(VSWIOC_ADD_PORT, (void *)&cmd);
	if (rc != VSWCONFIG_OK) {
		perror("error: failed to add port");
		return VSWCONFIG_NG;
	}
	printf("info: command succeeded.\n");
	return VSWCONFIG_OK;
}

int
delport(int argc, char *argv[])
{
	vsw_ioctl_cmd_t		cmd;
	int			rc = 0;

	if (argc != 2) {
		fprintf(stderr, "usage: vswconfig delport portname\n");
		return VSWCONFIG_NG;
	}

	memset(&cmd, 0, sizeof(cmd));
	strncpy(cmd.portname, argv[1], sizeof(cmd.portname) - 1);
	rc = vsw_ioctl(VSWIOC_DELETE_PORT, (void *)&cmd);
	if (rc != VSWCONFIG_OK) {
		perror("error: failed to delete port");
		return VSWCONFIG_NG;
	}
	printf("info: command succeeded.\n");
	return VSWCONFIG_OK;
}

int
show(int argc, char *argv[])
{
	vsw_ioctl_get_ports_t	cmd;
	int			rc = 0;
	int			i;

	if (argc != 1) {
		fprintf(stderr, "usage: vswconfig show\n");
		return VSWCONFIG_NG;
	}
	rc = vsw_ioctl(VSWIOC_GET_PORTS, (void *)&cmd);
	if (rc != VSWCONFIG_OK) {
		perror("error: failed to delete port");
		return VSWCONFIG_NG;
	}
	if (cmd.portnum == 0) {
		printf("no ports\n");
	} else {
		for (i = 0; i < cmd.portnum; i++) {
			printf("%s\n", cmd.portname[i]);
		}
	}
	return VSWCONFIG_OK;
}

int
docommand(char *command, char *args[], int narg)
{
	struct builtin	*tp = NULL;

	for (tp = commandtab; tp->name; tp++) {
		if (strcmp(tp->name, command) == 0) {
			break;
		}
	}
	if (tp->name == NULL) {
		usage();
		return VSWCONFIG_NG;
	}
	return (*tp->func)(narg, args);
}

int
vsw_ioctl(int request, void *iobuf)
{
	int	rc = VSWCONFIG_OK;
	int	fd = -1;

	fd = open(VSWITCH_DEV, O_RDWR);
	if (fd < 0) {
		return VSWCONFIG_NG;
	}
	rc = ioctl(fd, request, iobuf);
	if (rc < 0) {
		rc = VSWCONFIG_NG;
	} else {
		rc = VSWCONFIG_OK;
	}
	close(fd);
	return rc;
}

