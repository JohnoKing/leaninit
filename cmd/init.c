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
#ifdef COMPAT
#define RC "/etc/leaninit/rc"
#else
#define RC "/etc/rc"
#endif

// Halts, reboots or turns off the system
static int halt(int signal)
{
	// Synchronize the filesystems
	sync();

	// Kill all processes
	kill(-1, SIGTERM);
	kill(-1, SIGKILL);

	// Call reboot(2)
	switch(signal) {
		case SIGUSR1: // Halt
			return reboot(SYS_HALT);

		case SIGUSR2: // Power-off
			return reboot(SYS_POWEROFF);

		case SIGINT:  // Reboot
			return reboot(RB_AUTOBOOT);
	}

	return 1;
}

static void sighandle(int signal)
{
	switch(signal) {
		case SIGUSR1:       // Halt
			halt(SIGUSR1);
			break;
		case SIGUSR2:       // Power-off
			halt(SIGUSR2);
			break;
		case SIGINT:        // Reboot
			halt(SIGINT);
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
		execl("/bin/sh", "/bin/sh", RC, NULL);

	// Suspend init
	for(;;) {
		sleep(1);
		signal(SIGUSR1, sighandle);
		signal(SIGUSR2, sighandle);
		signal(SIGINT,  sighandle);
	}
}

// Shows usage for init(8)
static int usage(void)
{
	printf("%s: Option not permitted\n", __progname);
	printf("Usage: %s [mode] ...\n", __progname);
	printf("  0         Poweroff\n");
	printf("  6         Reboot\n");
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

		// Fallback
		default:
			return usage();
	}
}
