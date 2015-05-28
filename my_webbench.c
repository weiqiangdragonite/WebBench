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

#include <netdb.h>

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

char host[NI_MAXHOST];
#define REQUEST_SIZE	2048
char request[REQUEST_SIZE];


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
void build_request(const char *url);


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

	build_request(argv[optind]);

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

/*
 * build the reuqest header, such:
 * GET /abc HTTP/1.0\r\n
 * Connection: close\r\n
 * \r\n
 */
void
build_request(const char *url)
{
	char tmp[10];
	int i;

	memset(request, 0, REQUEST_SIZE);
	memset(host, 0, NI_MAXHOST);

	/* I don't understand */
	if (force_reload && proxy_host != NULL && http_ver < 1)
		http_ver = 1;

	if (method == METHOD_HEAD && http_ver < 1)
		http_ver = 1;

	if (method == METHOD_OPTIONS && http_ver < 2)
		http_ver = 2;

	if (method == METHOD_TRACE && http_ver < 2)
		http_ver = 2;


	switch (method) {
	case METHOD_HEAD:
		strncpy(request, "GET", REQUEST_SIZE);
		break;
	case METHOD_OPTIONS:
		strncpy(request, "OPTIONS", REQUEST_SIZE);
		break;
	case METHOD_TRACE:
		strncpy(request, "TRACE", REQUEST_SIZE);
		break;
	case METHOD_GET:
	default:
		strncpy(request, "GET", REQUEST_SIZE);
		break;
	}

	strcat(request, " ");

	if (strstr(url, "://") == NULL) {
		fprintf(stderr, "\n%s: is not a valid URL.\n", url);
		exit(1);
	}
	if (strlen(url) > 1500) {
		fprintf(stderr, "URL is too long.\n");
		exit(1);
	}

	if (proxy_host == NULL) {
		/* compare two strings ignoring case */
		if (strncasecmp("http://", url, 7) != 0) {
			fprintf(stderr, "\nOnly HTTP protocol is directly "
				"supported, set --proxy for others.\n");
			exit(1);
		}
	}

	/* protocol/host delimiter(定界符)*/
	/* If url is "http://abc.com/", then i = 7 */
	i = strstr(url, "://") - url + 3;

	if (strchr(url+i, '/') == NULL) {
		fprintf(stderr, "\nInvalid URL syntax - hostname must ends with '/'");
		exit(1);
	}

	if (proxy_host == NULL) {
		/* get host and port from hostname */
		/* if url is "http://abc.com:10086/test", then
		host = "abc.com", port = 10086, request = GET /test */
		if (index(url+i, ':') != NULL &&
		(index(url+i, ':') < index(url+i, '/'))) {
			strncpy(host, url+i, strchr(url+i, ':') - url - i);
			memset(tmp, 0, 10);
			strncpy(tmp, index(url+i, ':') + 1,
				strchr(url+i, '/') - index(url+i, ':') - 1);
			proxy_port = atoi(tmp);
		} else {
			/* strcspn - get length of a prefix substring */
			strncpy(host, url + i, strcspn(url+i, "/"));
		}
		/* printf("host = %s, port = %d\n", host, proxy_port); */
		/* printf("strcspn(url+i, /) = %d\n", (int) strcspn(url+i, "/")); */
		strcat(request + strlen(request), url + i + strcspn(url+i, "/"));
	} else {
		/* why? I don't understand proxy */
		strcat(request, url);
	}
	printf("rqeuest = %s\n", request);

	if (http_ver == 1)
		strcat(request, " HTTP/1.0");
	else if (http_ver == 2)
		strcat(request, " HTTP/1.1");
	strcat(request, "\r\n");

	if (proxy_host == NULL && http_ver > 0) {
		strcat(request, "Host: ");
		strcat(request, host);
		strcat(request, "\r\n");
	}

	if (force_reload && proxy_host != NULL)
		strcat(request, "Pragma: no-cache\r\n");

	/* HTTP/1.1 default is keep-alive */
	if (http_ver > 1)
		strcat(request, "Connection: close\r\n");

	if (http_ver > 0)
		strcat(request, "\r\n");
}
