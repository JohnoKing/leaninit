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
 * A fast init system
 */

#include <sys/reboot.h>
#include <sys/wait.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

// Macros for power management
#define POWEROFF 0
#define REBOOT   6
#define HALT     7

#ifdef LINUX
#define SLEEP    8
#define SYS_POWEROFF RB_POWER_OFF
#define SYS_HALT     RB_HALT_SYSTEM
#endif

#ifdef FREEBSD
#define SYS_POWEROFF RB_POWEROFF
#define SYS_HALT     RB_HALT
#endif

// argv[0] is not sufficent
extern char *__progname;

// Location of the init script
#ifndef OVERRIDE
static const char *rc_init = "/etc/leaninit/rc";
#else
static const char *rc_init = "/etc/rc";
#endif

// Default booleans for halt
static bool dosync = true;
static bool force  = false;
static bool wall   = true;

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
static int init_usage(void)
{
	printf("%s: Option not permitted\nUsage: %s [mode] ...\n", __progname, __progname);
	printf("  0         Poweroff\n");
	printf("  6         Reboot\n");
	printf("  7         Halt\n");
#ifdef LINUX
	printf("  8         Hibernate\n");
#endif
	return 1;
}

// Notify the system of shutdown
static void halt_notify(const char *message)
{
	// Output a message if wall is true
	if(wall == true)
		syslog(LOG_NOTICE, "%s", message);

	// Kill all processes with SIGTERM if the force flag is not passed
	if(force == false)
		kill(-1, SIGTERM);
}

// Shows usage for halt(8)
static int halt_usage(int ret)
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

// Halts, reboots or turns off the system
static int halt(int runlevel)
{
	// Synchronize the filesystems if dosync is set to true
	if(dosync == true)
		sync();

	// Act as per the runlevel
	switch(runlevel) {

		// Poweroff
		case POWEROFF:
			halt_notify("The system is now powering off!");
			return reboot(SYS_POWEROFF);

		// Reboot
		case REBOOT:
			halt_notify("The system is now rebooting!");
			return reboot(RB_AUTOBOOT);

		// Halt
		case HALT:
			halt_notify("The system will now halt!");
			return reboot(SYS_HALT);
#ifdef LINUX
		// Hibernate
		case SLEEP:
			force = true;
			halt_notify("The system is now being sent into sleep mode.");
			return reboot(RB_SW_SUSPEND);
#endif

		// For bad signals (never reached)
		default:
			printf("Something went wrong, received mode %i\n", runlevel);
			return 2;
	}
}

// Main function for halt
static int halt_main(int runlevel, int argc, char *argv[])
{
	bool stop_opt = false; // Prevent -hpr from being possible

	// When given arguments
	if(argc != 1) {

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
					return halt_usage(0);

				// Forcefully shutdown the system without killing all processes first
				case 'q':
				case 'f':
					force = true;
					break;

				// Force halt
				case 'h':
					if(stop_opt == true)
						return halt_usage(1);
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
						return halt_usage(1);
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
						return halt_usage(1);
					runlevel = REBOOT;
					stop_opt = true;
					break;

				// Show usage, but with a return status of 1
				default:
					return halt_usage(1);
			}
		}
	}

	return halt(runlevel);
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

		// Run the init script if LeanInit is PID 1
		if(getpid() == 1) {
			rc();

		// When LeanInit is not PID 1
		} else {
			if(argc == 1)
				return init_usage();

			switch(*argv[1]) {

				// Poweroff
				case '0':
					return halt(POWEROFF);

				// Reboot
				case '6':
					return halt(REBOOT);

				// Halt
				case '7':
					return halt(HALT);
#ifdef LINUX
				// Hibernate (Disabled on FreeBSD)
				case '8':
					return halt(SLEEP);
#endif
				// Fallback
				default:
					return init_usage();
			}
		}
	}

	// Halt
	if(strncmp(__progname, "halt", 4) == 0 || strncmp(__progname, "lhalt", 5) == 0)
		return halt_main(HALT, argc, argv);

	// Poweroff
	if(strncmp(__progname, "poweroff", 8) == 0 || strncmp(__progname, "lpoweroff", 9) == 0)
		return halt_main(POWEROFF, argc, argv);

	// Reboot
	if(strncmp(__progname, "reboot", 6) == 0 || strncmp(__progname, "lreboot", 7) == 0)
		return halt_main(REBOOT, argc, argv);

#ifdef LINUX
	// Sleep
	if(strncmp(__progname, "zzz", 3) == 0 || strncmp(__progname, "lzzz", 4) == 0)
		return halt(SLEEP);
#endif

	// This should not be reached, give an error and exit
	printf("LeanInit cannot be executed as %s\n", __progname);
	return 1;
}
