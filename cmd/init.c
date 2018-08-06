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
 * leaninit - A fast init system
 */

#include "inc.h"

// Location of the init script
#ifdef COMPAT
#define RC "/etc/leaninit.d/rc"
#else
#define RC "/etc/rc"
#endif

// Functions
static int halt(int signal);
static int single(void);
static int usage(void);
static void bootrc(void);
static void cmd(const char *cmd);
static void open_tty(void);
static void sighandle(int signal);

// The main function
int main(int argc, char *argv[])
{
	// PID 1
	if(getpid() == 1) {

		// Single user support
		int args;
		while((args = getopt(argc, argv, "s")) != -1) {
			switch(args) {
				case 's':
					open_tty();
					printf(COLOR_BOLD COLOR_CYAN "* " COLOR_WHITE "Booting into single user mode...\n" COLOR_RESET);
					return single();
					break;
			}
		}

		// Proceed with standard boot
		bootrc();
	}

	// Prevent anyone but root from running this
	if(getuid() != 0) {
		printf(COLOR_BOLD COLOR_RED "* " COLOR_LIGHT_RED "%s\n" COLOR_RESET, strerror(EPERM));
		return 1;
	}

	// Emulate some SysV-like behavior when re-executed
	if(argc == 1)
		return usage();

	// Only runlevels 0 and 6 are supported
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

// Open DEFAULT_TTY
static void open_tty(void) {
	// close(2) should not be run
	int tty = open(DEFAULT_TTY, O_RDWR);
	login_tty(tty);

	// Print to DEFAULT_TTY the current platform LeanInit is running on
	struct utsname uts;
	uname(&uts);
	printf(COLOR_BOLD COLOR_CYAN "* " COLOR_WHITE "LeanInit is running on %s %s %s\n" COLOR_RESET, uts.sysname, uts.release, uts.machine);
}


// Shows usage for init
static int usage(void)
{
	printf("%s: Option not permitted\n", __progname);
	printf("Usage: %s [mode] ...\n", __progname);
	printf("  0         Poweroff\n");
	printf("  1         Single user mode\n");
	printf("  6         Reboot\n");
	return 1;
}

// Execute the init script in a seperate process.
static void bootrc(void)
{
	// Run open_tty() first
	open_tty();

	// Now execute rc(8)
	pid_t shrc = fork();
	if(shrc == 0)
		execl("/bin/sh", "/bin/sh", RC, (char*)0);

	// Handle SIGUSR1, SIGUSR2 and SIGINT with sigaction(2)
	struct sigaction actor;
	memset(&actor, 0, sizeof(actor)); // Without this sigaction is ineffective
	actor.sa_handler = sighandle;
	sigaction(SIGUSR1, &actor, (struct sigaction*)NULL);  // Halt
	sigaction(SIGUSR2, &actor, (struct sigaction*)NULL);  // Poweroff
	sigaction(SIGINT,  &actor, (struct sigaction*)NULL);  // Reboot

	// This perpetual loop kills all zombie processes
	for(;;)
		wait(0);
}

// Single user mode
static int single(void)
{
	// Login as root
	setlogin("root");

	// Use a shell of the user's choice
	char shell[100];
	printf("Shell to use for single user (defaults to /bin/sh):\n");
	scanf("%s", shell);

	// Error checking
	FILE *optsh = fopen(shell, "r");
	if(optsh == NULL) {
		FILE *defsh = fopen("/bin/sh", "r");
		if(defsh == NULL)
			return halt(SIGUSR2); // Abandon to halt()
		else {
			fclose(defsh);
			memcpy(shell, "/bin/sh", 7);
		}
	} else
		fclose(optsh);

	// Fork the shell into a seperate process
	int rescue = fork();
	if(rescue == 0)
		return execl(shell, shell, (char*)0);

	// Shutdown when done
	wait(0);
	return halt(SIGUSR2);
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

// Execute a command and wait until it finishes
static void cmd(const char *cmd)
{
	int cfork = fork();
	if(cfork == 0)
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char*)0);
	wait(0);
}

// Handle signals given to init
static void sighandle(int signal)
{
	if(signal == SIGUSR1 || signal == SIGUSR2 || signal == SIGINT)
		halt(signal);
}
