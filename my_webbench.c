/*
 * Study From WebBench-1.5 Under GPLv2
 *
 * <weiqiangdragonite@gmail.com>
 *
 * Style: 8 Tab
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

/* Marco */
#define PROGRAM_VERSION	"1.5"

/* Allow: GET, HEAD, OPTIONS, TRACE */
#define METHOD_GET	0
#define METHOD_HEAD	1
#define METHOD_OPTIONS	2
#define METHOD_TRACE	3

/* Global variables */
int force = 0;		/* ?? */
int force_reload = 0;	/* ?? */
int http_ver = 1;	/* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */
int bench_time = 30;	/* run benchmark for 30 sec (default) */
int clients = 1;	/* run n http clients, default n = 1 */
int method = METHOD_GET;	/* HTTP method */
char *proxy_host = NULL;
int proxy_port = 80;


/* long options structure */
static const struct option long_options[] = {
	{"force", no_argument, &force, 1},
	{"reload", no_argument, &force_reload, 1},
	{"time", required_argument, NULL, 't'},
	{"help", no_argument, NULL, 'h'},
	{"http09", no_argument, NULL, '9'},
	{"http10", no_argument, NULL, '1'},
	{"http11", no_argument, NULL, '2'},
	{"get", no_argument, &method, METHOD_GET},
	{"head", no_argument, &method, METHOD_HEAD},
	{"options", no_argument, &method, METHOD_OPTIONS},
	{"trace", no_argument, &method, METHOD_TRACE},
	{"version", no_argument, NULL, 'v'},
	{"proxy", required_argument, NULL, 'p'},
	{"clients", required_argument, NULL, 'c'},
	{0, 0, 0, 0}
};


/* Function prototypes */
void usage(void);


/*
 * Usage:
 *	webbench --help
 *
 * Return:
 *	0 - success
 *	1 - bad paramaters
 *	2 - benchmark failed (server is not on-line)
 *	3 - internal error, fork failed
 */
int
main(int argc, char *argv[])
{
	int opt;
	int options_index = 0;	/* not used */
	char *optstring = ":h912vfrt:p:c:";
	char *tmp = NULL;

	/* check for command paramaters */

	if (argc == 1) {
		usage();
		return 2;
	}

	/* process arguments - glibc: getopt_long() */
	while (1) {
		opt = getopt_long(argc, argv, optstring,
			long_options, &options_index);
		if (opt == -1)
			break;

		switch (opt) {
		/* For long option, if flag is NULL, then return val, otherwise
		return 0 and flag point to val. */
		case 0:
			break;
		case 'f':
			force = 1;
			break;
		case 'r':
			force_reload = 1;
			break;
		case '9':
			http_ver = 0;
			break;
		case '1':
			http_ver = 1;
			break;
		case '2':
			http_ver = 2;
			break;
		case 't':
			bench_time = atoi(optarg);
			break;
		case 'p':
			/* proxy server parsing - "server:port" */
			/* strrchr() - find character from the end */
			/* still cannot check come bad parameters, such
			xxx:xxx:xxx:ddd */
			tmp = strrchr(optarg, ':');
			proxy_host = optarg;
			if (tmp == NULL)
				break;
			if (tmp == optarg) {
				fprintf(stderr, "Error in option --proxy %s "
					"(Missing hostname)\n", optarg);
				return 1;
			}
			if (tmp == (optarg + strlen(optarg) - 1)) {
				fprintf(stderr, "Error in option --proxy %s "
					"(Missing prot number)\n", optarg);
				return 1;
			}
			/* Be careful --> *tmp = '\0' is to end the host string */
			*tmp = '\0';
			proxy_port = atoi(tmp + 1);
			break;
		case 'c':
			clients = atoi(optarg);
			break;
		case 'v':
			printf("WebBench "PROGRAM_VERSION"\n");
			return 0;
		case ':':
			fprintf(stderr, "Missing argument (-%c)\n", optopt);
			usage();
			return 1;
		case '?':
			fprintf(stderr, "Unrecognized option (-%c)\n", optopt);
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	if (optind == argc) {
		fprintf(stderr, "webbench: Missing URL!\n");
		usage();
		return 1;
	}

	if (clients == 0)
		clients = 1;
	if (bench_time == 0)
		bench_time = 30;

	/* Print Copyright */
	/* Why use stderr? because un-buffer? */
	fprintf(stderr, "WebBench - Simple Web Benchmark "PROGRAM_VERSION"\n"
		"Copyright (c)  Radim Kolar 1997-2004, GPL Open Source Software.\n");

	return 0;
}

/*
 * Print usage.
 */
void
usage(void)
{
	printf(
		"webbench [option]... URL\n"
		"  -f|--force               Don't wait for reply from server.\n"
		"  -r|--reload              Send reload request - Pragma: no-cache.\n"
		"  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
		"  -p|--proxy <server:port> Use proxy server for request.\n"
		"  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
		"  -9|--http09              Use HTTP/0.9 style requests.\n"
		"  -1|--http10              Use HTTP/1.0 protocol.\n"
		"  -2|--http11              Use HTTP/1.1 protocol.\n"
		"  --get                    Use GET request method.\n"
		"  --head                   Use HEAD request method.\n"
		"  --options                Use OPTIONS request method.\n"
		"  --trace                  Use TRACE request method.\n"
		"  -h|--help                This information.\n"
		"  -v|--version             Display program version.\n");
}
