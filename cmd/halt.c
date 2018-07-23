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

#include "init.h"

// Long options for halt
static struct option halt_options[] = {
	{ "no-wtmp",     no_argument, NULL, 'd' },
	{ "force",       no_argument, NULL, 'f' },
	{ "halt",        no_argument, NULL, 'h' },
	{ "no-wall",     no_argument, NULL, 'l' },
	{ "nosync",      no_argument, NULL, 'N' },
	{ "nosync",      no_argument, NULL, 'n' },
	{ "poweroff",    no_argument, NULL, 'p' },
	{ "reboot",      no_argument, NULL, 'r' },
	{ "wtmp-only",   no_argument, NULL, 'w' },
	{ "help",        no_argument, NULL, '?' },
};

// Notify the system of shutdown
void halt_notify(const char *message)
{
	// Output a message if wall is true
	if(wall == true)
		syslog(LOG_NOTICE, "%s", message);

	// Kill all processes with SIGTERM if the force flag is not passed
	if(force == false)
		kill(-1, SIGTERM);
}

// Shows usage for halt(8)
int halt_usage(int ret)
{
	printf("Usage: %s [-dfhnNpqrw?]\n", __progname);
	printf("  -d, --no-wtmp          Ignored for compatibility (LeanInit currently does not write a wtmp entry on shutdown)\n");
	printf("  -f, -q,  --force       Perform shutdown or reboot without sending all processes SIGTERM\n");
	printf("  -h, --halt             Halts the system\n");
	printf("  -l, --no-wall          Turn off wall messages\n");
	printf("  -n, -N, --nosync       Disable filesystem synchronization before poweroff or reboot\n");
	printf("  -p, --poweroff         Turn off the system (default behavior)\n");
	printf("  -r, --reboot           Restart the system\n");
	printf("  -w, --wtmp-only        Incompatible, exits with return status 1\n");
	printf("  -?, --help             Show this usage information\n");
	return ret;
}

// Main function for halt
int halt_main(int runlevel, int argc, char *argv[])
{
	// When given arguments
	if(argc != 1) {

		// Parse the given options
		int args;
		while((args = getopt_long(argc, argv, "dfhnNpqrw?", halt_options, NULL)) != -1) {
			switch(args) {

				// Display usage with a return status of 0
				case '?':
					return halt_usage(0);

				// Forcefully shutdown the system without killing all processes first
				case 'q':
				case 'f':
					force = true;
					break;

				// Force halt
				case 'h':
					runlevel |= HALT;
					break;

				// Turn off wall messages
				case 'l':
					wall = false;
					break;

				// Force poweroff
				case 'p':
					runlevel |= POWEROFF;
					break;

				// -d and -w are not not supported
				case 'd':
					printf("WARNING: Option 'd' is being ignored.\n");
					break;

				case 'w':
					printf("Option 'w' is not supported!\n");
					return 1;

				// Disable filesystem sync
				case 'N':
				case 'n':
					dosync = false;
					break;

				// Force reboot
				case 'r':
					runlevel |= REBOOT;
					break;

				// Show usage, but with a return status of 1
				default:
					return halt_usage(1);
			}
		}
	}

	return halt(runlevel);
}
