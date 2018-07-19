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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Macros for portable power management
#ifdef LINUX
#define SYS_POWEROFF RB_POWER_OFF
#define SYS_HALT     RB_HALT_SYSTEM
#endif

#ifdef FREEBSD
#define SYS_POWEROFF RB_POWEROFF
#define SYS_HALT     RB_HALT
#endif

// __progname functions better here
extern char *__progname;

// Location of the init script
#ifndef OVERRIDE
static const char *rc_init = "/etc/leaninit/rc";
#else
static const char *rc_init = "/etc/rc";
#endif

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
static int usage(void)
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

// Halts, reboots or turns off the system
static int halt(int action, char *dosync)
{
	// Synchronize the filesystems if dosync is set to true
	if(strncmp(dosync, "n", 1) != 0)
		sync();

	// Act as per the runlevel
	switch(action) {

		// Poweroff
		case SYS_POWEROFF:
			return reboot(SYS_POWEROFF);

		// Reboot
		case RB_AUTOBOOT:
			return reboot(RB_AUTOBOOT);

		// Halt
		case SYS_HALT:
			return reboot(SYS_HALT);
#ifdef LINUX
		// Hibernate
		case RB_SW_SUSPEND:
			return reboot(RB_SW_SUSPEND);
#endif

		// For bad signals (never reached)
		default:
			printf("Something went wrong, received mode %i\n", action);
			return 2;
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

	// Run the init script if LeanInit is PID 1
	if(getpid() == 1)
		rc();

	// When LeanInit is not PID 1
	else {
		// Arguments are required when not PID 1
		if(argc == 1)
			return usage();

		// Do not sync if told to not do so
		char *dosync;
		if(argc > 2)
			dosync = argv[2];
		else
			dosync = "s";

		switch(*argv[1]) {

			// Poweroff
			case '0':
				return halt(SYS_POWEROFF, dosync);

			// Reboot
			case '6':
				return halt(RB_AUTOBOOT, dosync);

			// Halt
			case '7':
				return halt(SYS_HALT, dosync);

#ifdef LINUX
			// Hibernate (Disabled on FreeBSD)
			case '8':
				return halt(RB_SW_SUSPEND, dosync);
#endif
			// Fallback
			default:
				return usage();
		}
	}

	// This should not be reached
	return 1;
}
