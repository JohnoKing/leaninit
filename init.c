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
#include <sys/wait.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define POWEROFF 0
#define REBOOT   6
#define HALT     7
#ifdef LINUX
#define SLEEP    8
#endif

// Location of the init script
#ifndef OVERRIDE
static const char *rc_init = "/etc/leaninit/rc";
#else
static const char *rc_init = "/etc/rc";
#endif

// argv[0] is not sufficent
extern char *__progname;

// Execute the init script in a seperate process
static void rc(void)
{
	pid_t sh_rc = fork();

	if(sh_rc == 0)
		execl("/bin/sh", "/bin/sh", rc_init, NULL);
	else {
		for(;;)
			wait(0);
	}

	// This should never be reached
	sync();
}

// Shows usage for init(8)
static int usage_init(void)
{
	printf("%s: Option not permitted\nUsage: %s [mode] ...\n", __progname, __progname);
	printf("  0            Poweroff\n");
	printf("  6            Reboot\n");
	printf("  7            Halt\n");
#ifdef LINUX
	printf("  8            Hibernate\n");
#endif
	return 1;
}

// Shows usage for halt(8)
static int usage_halt(int ret)
{
	printf("Usage: %s [-dfhnprw]\n", __progname);
	printf("  -d            Ignored for compatibility (LeanInit currently does not write a wtmp entry on shutdown)\n");
	printf("  -f            Ignored for compatibility\n");
	printf("  -h            Show this usage information\n");
	printf("  -n            Disable filesystem synchronization before poweroff or reboot\n");
	printf("  -r            Restart the system\n");
	printf("  -p            Powers off the system (default behavior)\n");
	printf("  -w            Ignored for compatibility\n");
	return ret;
}

// Halts, reboots or turns off the system
static void halt(int runlevel, bool dosync)
{
	if(dosync == true)
		sync();

	// Act as per the runlevel
	switch(runlevel) {
#ifdef LINUX
		case HALT:
			reboot(RB_HALT_SYSTEM);
			break;
		case POWEROFF:
			reboot(RB_POWER_OFF);
			break;
		case SLEEP:
			reboot(RB_SW_SUSPEND); // Hibernate, currently disabled on FreeBSD
			break;
#endif
#ifdef FREEBSD
		case HALT:
			reboot(RB_HALT);
			break;
		case POWEROFF:
			reboot(RB_POWEROFF);
			break;
#endif
		case REBOOT:
			reboot(RB_AUTOBOOT);
			break;
		default:
			printf("Something went wrong, received mode %i\n", runlevel);
			exit(2);
	}
}

// The main function
int main(int argc, char *argv[])
{
	// Prevent anyone but root from running this
	if(getuid() != 0) {
		printf("Permission denied\n");
		return 1;
	}

	// When ran as init(8)
	if(strncmp(__progname, "init", 4) == 0 || strncmp(__progname, "linit", 5) == 0) {

		// Defaults to verbose boot
		if(getpid() == 1) {
			rc();

		} else {
			if(argc == 1)
				return usage_init();

			switch(*argv[1]) {

				// Poweroff
				case '0':
					halt(POWEROFF, true);
					break;

				// Reboot
				case '6':
					halt(REBOOT, true);
					break;

				// Halt
				case '7':
					halt(HALT, true);
					break;
#ifdef LINUX
				// Hibernate (Disabled on FreeBSD)
				case '8':
					halt(SLEEP, true);
					break;
#endif
				// Fallback
				default:
					return usage_init();
			}

			return 0;
		}

		// This should not be reached
		return 1;
	}

	// When ran as halt(8)
	if(strncmp(__progname, "halt", 4) == 0 || strncmp(__progname, "lhalt", 5) == 0) {
		int  runlevel  = POWEROFF; // 0 is the default runlevel for halt
		bool dosync    = true;     // Synchronize filesystems by default

		// When given arguments
		if(argc != 1) {
			int args;
			while((args = getopt(argc, argv, "dfhnprw")) != -1) {
				switch(args) {

					// Display usage with a return status of 0
					case 'h':
						return usage_halt(0);

					// Ignore these options
					case 'd':
					case 'f':
					case 'p':
						printf("Option %c is being ignored\n", args);
						break;

					// -w is not not supported
					case 'w':
						printf("WARNING: Option 'w' is not supported!\n");
						break;

					// Disable filesystem sync
					case 'n':
						dosync = false;
						break;

					// Set the runlevel to 6 for reboot
					case 'r':
						runlevel = REBOOT;
						break;

					// Show usage, but with a return status of 1
					default:
						return usage_halt(1);
				}
			}
		}

		halt(runlevel, dosync);
		return 0;
	}

	// When ran as poweroff
	if(strncmp(__progname, "poweroff", 8) == 0 || strncmp(__progname, "lpoweroff", 9) == 0) {
		halt(POWEROFF, true);
		return 0;
	}

	// When ran as reboot
	if(strncmp(__progname, "reboot", 6) == 0 || strncmp(__progname, "lreboot", 7) == 0) {
		halt(REBOOT, true);
		return 0;
	}

	// This should not be reached, give an error and exit
	printf("LeanInit cannot be executed as %s\n", __progname);
	return 1;
}
