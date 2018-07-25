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

#include "init.h"

// Location of the init script
#ifndef OVERRIDE
static const char *rc_init = "/etc/leaninit/rc";
#else
static const char *rc_init = "/etc/rc";
#endif

// Halts, reboots or turns off the system
int halt(int runlevel)
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
			halt_notify("The system is now being sent into hibernate (S4)");
			return reboot(RB_SW_SUSPEND);
#endif

		// For bad signals (never reached)
		default:
			printf("Something went wrong, received mode %i\n", runlevel);
			return runlevel;
	}
}

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
	printf("%s: Option not permitted\n", __progname);
	printf("Usage: %s [mode] ...\n", __progname);
	printf("  0         Poweroff\n");
	printf("  6         Reboot\n");
	printf("  7         Halt\n");
#ifdef LINUX
	printf("  8         Hibernate\n");
#endif
	return 1;
}

// The main function
int main(int argc, char *argv[])
{
	// Prevent anyone but root from running this
	if(getuid() != 0) {
		printf("Permission denied\n");
		return 1;
	}

	// Default booleans for halt
	dosync = true;
	force  = false;
	wall   = true;

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
