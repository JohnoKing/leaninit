/*
 * Copyright (c) 2017-2018 Johnothan King. All rights reserved.
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
 * A minimal init system
 */

#include <sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define POWEROFF 0
#define REBOOT   6
#define SLEEP    7
#define HALT     8

// argv[0] is not sufficent
extern char *__progname;

// Execute the init script, located at either /etc/rc or /etc/leaninit/rc depending on the type of installation
static void rc(const char *mode)
{
#ifdef OVERRIDE
	execl("/bin/sh", "/bin/sh", "/etc/rc", mode, NULL);
#else
	execl("/bin/sh", "/bin/sh", "/etc/leaninit/rc", mode, NULL);
#endif

	// This is done in case this function stops to prevent data corruption
	sync();
}

// Shows usage for halt(8) when executed as halt
static int usage_halt(int ret)
{
	printf("Usage: %s [-dfhnprw]\n\n", __progname);
	printf("  -d            Ignored for compatibility (LeanInit currently does not write a wtmp entry on shutdown)\n");
	printf("  -f            Ignored for compatibility\n");
	printf("  -h            Show this usage information\n");
	printf("  -n            Disable filesystem synchronization before poweroff or reboot\n");
	printf("  -r            Restart the system\n");
	printf("  -p            Powers off the system (default behavior)\n");
	printf("  -w            Ignored for compatibility\n");
	exit(ret);
}

// Halts, reboots or turns off the system
static void halt(int level, int fs)
{
	if(fs != 0)
		sync();

	switch(level) {
#ifdef LINUX
		case HALT:
			reboot(RB_HALT_SYSTEM);
		case POWEROFF:
			reboot(RB_POWER_OFF);
		case SLEEP:
			reboot(RB_SW_SUSPEND); // Hibernate, currently disabled on FreeBSD
#endif
#ifdef FREEBSD
		case HALT:
			reboot(RB_HALT);
		case POWEROFF:
			reboot(RB_POWEROFF);
#endif
		case REBOOT:
			reboot(RB_AUTOBOOT);
		default:
			printf("Something went wrong, received mode %c\n", level);
			exit(2);
	}
}

// The main function
int main(int argc, char *argv[])
{
	// Prevent anyone but root from running this
	if(getuid() != 0) {
		printf("Permission denied\n");
		exit(1);
	}
 
	if(strncmp(__progname, "init", 4) == 0 || strncmp(__progname, "l-init", 6) == 0) {
		// Defaults to verbose boot
		if(argc == 1) {
			rc("v");

		// Determine what to do
		} else {
			switch(*argv[1]) {

				// Halt
				case 'h':
					halt(HALT, 1);

				// Poweroff
				case '0':
					halt(POWEROFF, 1);

				// Reboot
				case '6':
					halt(REBOOT, 1);

#ifdef LINUX
				// Hibernate (Disabled for now on FreeBSD)
				case '7':
					halt(SLEEP, 1);
#endif

				// Quiet boot, splash boot currently defaults to quiet boot
				case 'q':
				case 's':
					rc("q");

				// Verbose boot (default)
				case 'v':
					rc("v");

				// Fallback
				default:
					printf("%s: Option not permitted\n\nUsage: %s [mode] ...\n", __progname, __progname);
					return 1;
			}
		}
	}

	if(strncmp(__progname, "halt", 4) == 0 || strncmp(__progname, "l-halt", 6) == 0) {
		int dosync   = 1;        // Synchronize filesystems
		int level    = POWEROFF; // Default runlevel for halt

		if(argc == 1) {
			halt(POWEROFF, 1); // Default behavior

		} else {
			int args;
			while((args = getopt(argc, argv, "dfhnprw")) != -1) {
				switch(args) {

					// Display usage with a return status of 0
					case 'h':
						usage_halt(0);

					// Disable filesystem sync
					case 'n':
						dosync = 0;

					// -w is not not supported
					case 'w':
						printf("WARNING: Option 'w' is not supported!\n");

					// Ignore these options
					case 'd':
					case 'f':
					case 'p':
						printf("Option %s is being ignored\n", argv[1]);

					// Set the runlevel to 6 for reboot
					case 'r':
						level = REBOOT;

					// Show usage, but with a return status of 1
					default:
						printf("%s: invalid option -- %c\n", __progname, args);
						usage_halt(1);
				}
			}
		}

		halt(level, dosync);
	}

	if(strncmp(__progname, "poweroff", 8) == 0 || strncmp(__progname, "l-poweroff", 10) == 0)
		halt(POWEROFF, 1);

	if(strncmp(__progname, "reboot", 6) == 0 || strncmp(__progname, "l-reboot", 8) == 0)
		halt(REBOOT, 1);
}
