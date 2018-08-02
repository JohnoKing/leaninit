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

#include "inc.h"

// Location of the init script
#ifndef OVERRIDE
static const char *rc_init = "/etc/leaninit/rc";
#else
static const char *rc_init = "/etc/rc";
#endif

// Halts, reboots or turns off the system
int halt(int runlevel)
{
	// Synchronize the filesystems
	sync();

	// Kill all processes
	kill(-1, SIGTERM);
	kill(-1, SIGKILL);

	// Call reboot(2)
	switch(runlevel) {
		case POWEROFF:
			return reboot(SYS_POWEROFF);

		case REBOOT:
			return reboot(RB_AUTOBOOT);

		case HALT:
			return reboot(SYS_HALT);

		// For bad signals (never reached)
		default:
			printf("Something went wrong, received mode %i\n", runlevel);
			return runlevel;
	}
}

static void sighandle(int signal)
{
	switch(signal) {
		case SIGUSR1:       // Halt
			halt(HALT);
			break;
		case SIGUSR2:       // Power-off
			halt(POWEROFF);
			break;
		case SIGINT:        // Reboot
			halt(REBOOT);
			break;
	}
}

// Execute the init script in a seperate process
static int bootrc(void)
{
	// Open up the tty (eliminates the need for '> /dev/tty')
	int tty = open(DEFAULT_TTY, O_RDWR);
	login_tty(tty);
	close(tty);

	// Print to DEFAULT_TTY the current platform LeanInit is running on
	struct utsname uts;
	uname(&uts);
	printf("LeanInit is running on %s %s %s\n", uts.sysname, uts.release, uts.machine);

	// Run the init script
	pid_t sh_rc = fork();
	if(sh_rc == 0)
		execl("/bin/sh", "/bin/sh", rc_init, NULL);

	// Suspend init
	for(;;) {
		sleep(1);
		signal(SIGUSR1, sighandle);
		signal(SIGUSR2, sighandle);
		signal(SIGINT,  sighandle);
	}

	// This should never be reached
	return 1;
}

// Shows usage for init(8)
static int usage(void)
{
	printf("%s: Option not permitted\n", __progname);
	printf("Usage: %s [mode] ...\n", __progname);
	printf("  0         Poweroff\n");
	printf("  6         Reboot\n");
	printf("  7         Halt\n");
#ifdef Linux
	printf("  8         Hibernate\n");
#endif
	return 1;
}

// The main function
int main(int argc, char *argv[])
{
	// Run the init script if LeanInit is PID 1 (getuid is skipped)
	if(getpid() == 1)
		return bootrc();

	// Prevent anyone but root from running this
	if(getuid() != 0) {
		printf("%s\n", strerror(EPERM));
		return 1;
	}

	// When re-executed (not PID 1)
	if(argc == 1)
		return usage();

	switch(*argv[1]) {

		// Poweroff
		case '0':
			return kill(1, SIGUSR2);

		// Reboot
		case '6':
			return kill(1, SIGINT);

		// Halt
		case '7':
			return kill(1, SIGUSR1);
#ifdef Linux
		// Hibernate (Linux only)
		case '8':
			return reboot(RB_SW_SUSPEND);
#endif
		// Fallback
		default:
			return usage();
	}
}
