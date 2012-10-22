#include <sys/cdefs.h>

#include <err.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <sysexits.h>

#include "extern.h"

volatile bool cw_keydown = false;
double cw_freq = -1;

__dead static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-f freq]\n", __progname);
	exit(EX_USAGE);
}

void
cw_stop(void)
{
	key_stop();
}

static void
handle_signal(int signal)
{
	/*
	 * This is bad handling practice. We should just set a sig_atomic_t
	 * and have something in the CFRunLoop check for it, but this method
	 * is both easier to write, and has less overhead while the program is
	 * running normally. Also whenever we reach this code, it's because we
	 * want to terminate anyway, so terminating less gracefully isn't the
	 * end of the world.
	 */
	puts("caught SIGINT, terminating...\n");
	cw_stop();
}

int
main(int argc, char *argv[])
{
	int ch;
	char *end;

	while ((ch = getopt(argc, argv, "f:")) != -1)
		switch (ch) {
		case 'f':
			cw_freq = strtod(optarg, &end);
			if (end == optarg) {
				warnx("no frequency specified");
				usage();
			} else if (*end == '\0') {
				/* default to Hz */
			} else if (strcasecmp(end, "hz") == 0) {
				/* no multiplier */
			} else if (
			    strcasecmp(end, "khz") == 0 ||
			    strcasecmp(end, "k") == 0) {
				cw_freq *= 1000.0;
			} else {
				warnx("invalid frequency unit");
				usage();
			}

			if (cw_freq < 20 || cw_freq > 20000) {
				/*
				 * The accepted range should
				 * probably be narrower.
				 */
				warnx("invalid frequency... "
				    "not in realistic range");
				usage();
			}
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

	if (cw_freq == -1)
		cw_freq = DEFAULT_FREQUENCY;
	printf("goal of %.1f Hz tone\n", cw_freq);

	if (cw_jack_init() == -1)
		errx(EX_UNAVAILABLE, "unable to initialize JACK connection");
	if (cw_jack_connect() == -1)
		warnx("unable to connect to JACK output ports... "
		    "you will need to do so yourself");

	printf("Please press the key you wish to use... ");
	fflush(stdout);
	if (key_configure() == -1)
		errx(EX_UNAVAILABLE, "unable to configure active key");
	printf("Thank you.\n");

	if (key_init() == -1)
		errx(EX_UNAVAILABLE, "unable to initialize key event listener");
	if (key_listen() == -1)
		errx(EX_UNAVAILABLE, "unable to listen for key events");

	if (signal(SIGINT, handle_signal) == SIG_ERR)
		warn("unable to setup signal handler");

	key_listen(); /* blocks */

	key_cleanup();
	cw_jack_cleanup();
	exit(EX_OK);
}
