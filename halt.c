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
 * halt - Halt, poweroff or reboot the system
 */

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

// ACPI macros (char)
#define POWEROFF " 0"
#define REBOOT   " 6"
#define HALT     " 7"
#ifdef LINUX
#define SLEEP    " 8"
#endif

// Location of LeanInit
#ifndef OVERRIDE
char init_cmd[25] = "/sbin/init";
#else
char init_cmd[25] = "/sbin/linit";
#endif

// argv[0] is not sufficent
extern char *__progname;

// Universal booleans for notify() and system(3)
static bool force  = false;
static bool dosync = true;
static bool wall   = true;

// Notify the system of shutdown
static void notify(char *message)
{
	// Output a message if wall is true
	if(wall == true)
		syslog(LOG_NOTICE, "%s", message);

	// Kill all processes with SIGTERM if the force flag is not passed
	if(force == false)
		kill(-1, SIGTERM);
}

// Shows usage for halt(8)
static int usage(int ret)
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

// The main function for halt
int main(int argc, char *argv[])
{
	bool stop_opt  = false;  // Prevent -hpr from being possible
	char *runlevel;          // Variable for defining the runlevel
	char *msg;               // Notification variable

	// Halt
	if(strncmp(__progname, "halt", 4) == 0 || strncmp(__progname, "lhalt", 5) == 0) {
		msg = "The system will now halt!";
		runlevel = HALT;
	}

	// Poweroff
	else if(strncmp(__progname, "poweroff", 8) == 0 || strncmp(__progname, "lpoweroff", 9) == 0) {
		msg = "The system is now powering off!";
		runlevel = POWEROFF;
	}

	// Reboot
	else if(strncmp(__progname, "reboot", 6) == 0 || strncmp(__progname, "lreboot", 7) == 0) {
		msg = "The system is now rebooting!";
		runlevel = REBOOT;
	}

#ifdef LINUX
	// Sleep
	else if(strncmp(__progname, "zzz", 3) == 0 || strncmp(__progname, "lzzz", 4) == 0) {
		force = true;
		msg = "The system is now being sent into sleep mode!";
		runlevel = SLEEP;
	}
#endif

	// Error and exit (not a valid binary)
	else {
		printf("Halt cannot be executed as %s\n", __progname);
		return 1;
	}

	// When given arguments
#ifdef LINUX
	if(argc != 1 && force != true) {
#else
	if(argc != 1) {
#endif

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

		// Parse the given options
		int args;
		while((args = getopt_long(argc, argv, "dfhnNpqrw?", halt_options, NULL)) != -1) {
			switch(args) {

				// Display usage with a return status of 0
				case '?':
					return usage(0);

				// Forcefully shutdown the system without killing all processes first
				case 'q':
				case 'f':
					force = true;
					break;

				// Force halt
				case 'h':
					if(stop_opt == true)
						return usage(1);
					msg = "The system will now halt!";
					runlevel = HALT;
					stop_opt = true;
					break;

				// Turn off wall messages
				case 'l':
					wall = false;
					break;

				// Force poweroff
				case 'p':
					if(stop_opt == true)
						return usage(1);
					msg = "The system is now powering off!";
					runlevel = POWEROFF;
					stop_opt = true;
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
					if(stop_opt == true)
						return usage(1);
					msg = "The system is now rebooting!";
					runlevel = REBOOT;
					stop_opt = true;
					break;

				// Show usage, but with a return status of 1
				default:
					return usage(1);
			}
		}
	}

	// Notify
	notify(msg);

	// Execute init with or without sync
	strncat(init_cmd, runlevel, 2);
	if(dosync == false)
		strncat(init_cmd, " n", 2);

	return system(init_cmd);
}
