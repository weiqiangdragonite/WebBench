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
#include <errno.h>
#include <signal.h>

#include <netdb.h>

/* Marco */
#define PROGRAM_VERSION	"1.5"

/* Allow: GET, HEAD, OPTIONS, TRACE */
#define METHOD_GET	0
#define METHOD_HEAD	1
#define METHOD_OPTIONS	2
#define METHOD_TRACE	3


/* Global variables */
int force = 0;		/* set 0 to wait for server reply */
int force_reload = 0;	/* reload request - no cache */
int http_ver = 1;	/* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */
int bench_time = 30;	/* run benchmark for 30 sec (default) */
int clients = 1;	/* run n http clients, default n = 1 */
int method = METHOD_GET;	/* HTTP method */
char *proxy_host = NULL;	/* proxy host address */
int proxy_port = 80;		/* proxy port, also default http 80 port */

char host[NI_MAXHOST];
#define REQUEST_SIZE	2048
char request[REQUEST_SIZE];	/* HTTP request header */

int mypipe[2];			/* internal */
int speed = 0;			/* child process get reply from server */
int failed = 0;			/* child process failed counter */
int bytes = 0;			/* recv bytes form socket */
volatile int timer_expired = 0;	/* bench time over set to 1 */

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
int bench(void);
int inet_connect(const char *host, int port, int type);
static void alarm_handler(int signal);
void bench_core(const char *host, const int port, const char *req);


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


	/* print bench info */
	printf("\nBenchmarking: ");
	switch (method) {
	case METHOD_OPTIONS:
		printf("OPTIONS");
		break;
	case METHOD_HEAD:
		printf("HEAD");
		break;
	case METHOD_TRACE:
		printf("TRACE");
		break;
	case METHOD_GET:
	default:
		printf("GET");
		break;
	}
	printf(" %s", argv[optind]);

	switch (http_ver) {
	case 0:
		printf(" HTTP/0.9");
		break;
	case 2:
		printf(" HTTP/1.1");
		break;
	case 1:
	default:
		printf(" HTTP/1.0");
		break;
	}
	printf("\n");

	if (clients == 1)
		printf("1 client");
	else
		printf("%d clients", clients);
	printf(", running %d sec", bench_time);
	if (force)
		printf(", early socket close");
	if (proxy_host != NULL)
		printf(", via proxy server %s:%d", proxy_host, proxy_port);
	if (force_reload)
		printf(", forcing reload");
	printf(".\n");

	return bench();
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
 * Build the reuqest header, such:
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

	/* Need HTTP knowledge to know why */
	/* Proxy and force reload need HTTP/1.0 or HTTP/1.1 */
	if (force_reload && proxy_host != NULL && http_ver < 1)
		http_ver = 1;

	/* HEAD need HTTP/1.0 or HTTP/1.1 */
	if (method == METHOD_HEAD && http_ver < 1)
		http_ver = 1;

	/* Options need HTTP/1.1 */
	if (method == METHOD_OPTIONS && http_ver < 2)
		http_ver = 2;

	/* TRACE need HTTP/1.1 */
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
			memset(tmp, 0, sizeof(tmp));
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
		/* Need proxy knowledge */
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

	/* no-cache */
	if (force_reload && proxy_host != NULL)
		strcat(request, "Pragma: no-cache\r\n");

	/* HTTP/1.1 default is keep-alive */
	if (http_ver > 1)
		strcat(request, "Connection: close\r\n");

	/* The last blank line */
	if (http_ver > 0)
		strcat(request, "\r\n");
}

/*
 *
 */
int
bench(void)
{
	int i, j, k, r;
	FILE *f;
	pid_t pid = 0;

	/* check avaibility of target server */
	i = inet_connect(proxy_host == NULL ? host : proxy_host,
		proxy_port, SOCK_STREAM);
	if (i < 0) {
		fprintf(stderr, "\nConnect to server failed. Aborting benchmark.\n");
		return 2;
	}
	close(i);

	/* create pipe */
	if (pipe(mypipe) == -1) {
		perror("PIPE failed: ");
		return 3;
	};

	/* fork childs */
	for (i = 0; i < clients; ++i) {
		pid = fork();
		if (pid <= (pid_t) 0) {
			/* child process or error */
			/* make childs faster */
			sleep(1);
			break;
		}
	}

	/* error */
	if (pid < (pid_t) 0) {
		fprintf(stderr, "Problems forking client No.%d\n", i);
		perror("Fork failed: ");
		return 3;
	}

	/* child */
	if (pid == (pid_t) 0) {
		if (proxy_host == NULL)
			bench_core(host, proxy_port, request);
		else
			bench_core(proxy_host, proxy_port, request);

		/* write results to pipe */
		/* pipe[0] for read; pipe[1] for write */
		f = fdopen(mypipe[1], "w");
		if (f == NULL) {
			perror("Open pipe for write failed: ");
			return 3;
		}

		/* write to pipe */
		fprintf(f, "%d %d %d\n", speed, failed, bytes);
		fclose(f);
		return 0;

	/* parent */
	} else {
		f = fdopen(mypipe[0], "r");
		if (f == NULL) {
			perror("Open pipe for read failed: ");
			return 3;
		}

		/* set open stream to unbuffered */
		setvbuf(f, NULL, _IONBF, 0);
		speed = 0;
		failed = 0;
		bytes = 0;

		while (1) {
			r = fscanf(f, "%d %d %d", &i, &j, &k);
			/* r < 2 ??? */
			if (r < 3) {
				fprintf(stderr, "Some of our children died.\n");
				break;
			}

			speed += i;
			failed += j;
			bytes += k;

			/* wait for all clients finished */
			if (--clients == 0)
				break;
		}
		fclose(f);

		printf("\nSpeed = %d pages/min, %d bytes/sec.\n"
			"Requests: %d succeed, %d failed.\n",
			(int) ((speed + failed) / (bench_time / 60.0)),
			(int)(bytes / (float) bench_time), speed, failed);

	}

	return 0;
}

/*
 *
 */
static void
alarm_handler(int signal)
{
	timer_expired = 1;
}

/*
 *
 */
void
bench_core(const char *host, const int port, const char *req)
{
	int rlen;
	char buf[1500];
	int s, i;
	struct sigaction sa;

	/* setup alarm signal handler */
	sa.sa_handler = alarm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		fprintf(stderr, "Setup alarm signal handler failed.\n");
		exit(3);
	}

	alarm(bench_time);

	rlen = strlen(req);

next_try:
	while(1) {
		/* bench time is over */
		if (timer_expired) {
			/* why --failed ? */
			if (failed > 0)
				--failed;
			return;
		}

		s = inet_connect(host, port, SOCK_STREAM);
		if (s < 0) {
			++failed;
			continue;
		}

		if (rlen != write(s, req, rlen)) {
			++failed;
			close(s);
			continue;
		}

		/* HTTP/0.9 */
		if (http_ver == 0) {
			/* shutdown — shut down socket send and receive operations */
			/* SHUT_WR - Disables further send operations. */
			if (shutdown(s, SHUT_WR) == -1) {
				++failed;
				close(s);
				continue;
			}
		}

		if (force == 0) {
			/* read all avaliable data from socket */
			while (1) {
				if (timer_expired)
					break;

				i = read(s, buf, 1500);
				if (i < 0) {
					++failed;
					close(s);
					goto next_try;
				} else {
					if (i == 0)
						break;
					else
						bytes += i;
				}
			}
		}

		if (close(s) == -1) {
			++failed;
			continue;
		}
		++speed;
	}
}

/*
 * Copy from TLPI
 *
 * This function creates a socket with the given socket type, and connects it
 * to the address specified by host and service. This function is designed for
 * TCP or UDP clients that need to connect their socket to a server socket.
 */
int
inet_connect(const char *host, int port, int type)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int socket_fd;
	char service[NI_MAXSERV];

	memset(service, 0, NI_MAXSERV);
	snprintf(service, NI_MAXSERV, "%d", port);

	/* Set the hints argument */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	/* User define socket type (SOCK_STREAM or SOCK_DGRAM) */
	hints.ai_socktype = type;
	/* ai_protocol = 0;
	   For our purposes, this field is always specified as 0, meaning that
	   the caller will accept any protocol */
	/* Allows IPv4 or IPv6 */
	hints.ai_family = AF_UNSPEC;

	/* get a list of socket address */
	if (getaddrinfo(host, service, &hints, &result) != 0) {
		fprintf(stderr, "getaddrinfo() failed: %s\n", strerror(errno));
		return -1;
	}

	/* Walk through returned list until we find an address structure that
	   can be used to successfully connect a socket */
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		socket_fd = socket(rp->ai_family, rp->ai_socktype,
			rp->ai_protocol);

		/* If error, try next address */
		if (socket_fd == -1)
			continue;

		/* Try to connect socket */
		if (connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		/* Connect failed: close this socket and try next address */
		close(socket_fd);
	}

	/* We must free the struct addrinfo */
	freeaddrinfo(result);

	/* Return the socket_fd or -1 is error */
	return (rp == NULL) ? -1 : socket_fd;
}
