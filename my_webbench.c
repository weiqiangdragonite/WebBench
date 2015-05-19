/*
 * Study From WebBench-1.5 Under GPLv2
 *
 * weiqiangdragonite@gmail.com
 *
 * Format: 8 Tab(Linux Kernel Coding Style)
 */

#include <stdio.h>


/* Function prototypes */
void usage(void);


/*
 * Usage:
 *	webbench --help
 *
 * Return:
 *	0 - success
 *	1 - bad paramaters
 *	
 */
int
main(int argc, char *argv[])
{
	/* check for command paramaters */

	if (argc == 1) {
		usage();
		return 2;
	}
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
		"  -?|-h|--help             This information.\n"
		"  -V|--version             Display program version.\n");
}
