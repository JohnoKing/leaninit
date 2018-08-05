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

// Execute a command and wait until it finishes
static void cmd(const char *cmd)
{
	int cfork = fork();
	if(cfork == 0)
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char*)0);
	wait(0);
}

// Halts, reboots or turns off the system
static int halt(int signal)
{
	// Kill all processes
	kill(-1, SIGTERM);
	kill(-1, SIGKILL);

	// Synchronize the filesystems
	sync();

	// Remount root as read-only and unmount other filesystems
	cmd("mount -o remount,ro /");
	cmd("umount -a");

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

/* Execute the init script in a seperate process.
 * Only run if init is PID 1
 */
static void bootrc(void)
{
	/* Open up the tty (eliminates the need for '> /dev/tty')
	 * close(2) CANNOT be run, at least on FreeBSD.
	 */
	int tty = open(DEFAULT_TTY, O_RDWR);
	login_tty(tty);

	// Print to DEFAULT_TTY the current platform LeanInit is running on
	struct utsname uts;
	uname(&uts);
	printf("LeanInit is running on %s %s %s\n", uts.sysname, uts.release, uts.machine);

	// Run the init script
	pid_t shrc = fork();
	if(shrc == 0)
		execl("/bin/sh", "/bin/sh", RC, (char*)0);

	// Loop that kills all zombie processes while waiting for a signal
	for(;;) {
		wait(0);                     // Kill all zombie processes
		signal(SIGUSR1, sighandle);  // Halt
		signal(SIGUSR2, sighandle);  // Poweroff
		signal(SIGINT,  sighandle);  // Reboot
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
		bootrc();

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
