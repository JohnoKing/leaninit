/*
 * Copyright (c) 2018 Johnothan King. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * halt - Shutdown, reboot or halt system
 */

#include "inc.h"

// Long options for halt
static struct option halt_long_options[] = {
	{ "no-wall",     no_argument, 0, 'l' },
	{ "help",        no_argument, 0, '?' },
};

// Shows usage for halt(8)
static int usage(int ret)
{
	printf("Usage: %s [-l?]\n", __progname);
	printf("  -l, --no-wall          Turn off wall messages\n");
	printf("  -?, --help             Show this usage information\n");
	return ret;
}

// Main function for halt
int main(int argc, char *argv[])
{
	// halt(8) can only be run by root
	if(getuid() != 0) {
		printf("%s\n", strerror(EPERM));
		return 1;
	}

	bool wall = true; // For syslog(3)
	int signal;       // For signals to init

	// Halt
	if(strncmp(__progname, "halt", 4) == 0 || strncmp(__progname, "lhalt", 5) == 0)
		signal = SIGUSR1;

	// Poweroff
	else if(strncmp(__progname, "poweroff", 8) == 0 || strncmp(__progname, "lpoweroff", 9) == 0)
		signal = SIGUSR2;

	// Reboot
	else if(strncmp(__progname, "reboot", 6) == 0 || strncmp(__progname, "lreboot", 7) == 0)
		signal = SIGINT;

#ifdef Linux
	// Hibernate
	else if(strncmp(__progname, "zzz", 3) == 0 || strncmp(__progname, "lzzz", 4) == 0)
		return reboot(RB_SW_SUSPEND);
#endif

	// Not valid
	else {
		printf("Halt cannot be run as %s\n", __progname);
		return 1;
	}

	// When given arguments
	if(argc != 1) {

		// Parse the given options
		int args;
		while((args = getopt_long(argc, argv, "l?", halt_long_options, NULL)) != -1) {
			switch(args) {

				// Display usage with a return status of 0
				case '?':
					return usage(0);

				// Turn off wall messages
				case 'l':
					wall = false;
					break;

				// Show usage, but with a return status of 1
				default:
					return usage(1);
			}
		}
	}

	// Syslog
	if(wall == true) {
		openlog(__progname, LOG_CONS, LOG_AUTH);
		syslog(LOG_CRIT, "The system is going down NOW!");
		closelog();
	}

	// Send the correct sygnal to init
	//return kill(1, signal);
}
