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

// Functions
static void halt(int signal);
static int single(const char *msg);
static int usage(void);
static void bootrc(void);
static void sh(const char *cmd);
static void open_tty(void);
static void sigloop(void);

// The main function
int main(int argc, char *argv[])
{
	// PID 1
	if(getpid() == 1) {
		// Open DEFAULT_TTY
		open_tty();

		// Single user support
		int args;
		while((args = getopt(argc, argv, "s")) != -1) {
			switch(args) {
				case 's':
					return single("Booting into single user mode...");
			}
		}

		// Proceed with standard boot
		bootrc();
	}

	// Prevent anyone but root from running this
	if(getuid() != 0) {
		printf(COLOR_BOLD COLOR_RED "* " COLOR_LIGHT_RED "%s" COLOR_RESET "\n", strerror(EPERM));
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
static void open_tty(void)
{
	// close(2) should not be run
	int tty = open(DEFAULT_TTY, O_RDWR);
	login_tty(tty);

	// Print to DEFAULT_TTY the current platform LeanInit is running on
	struct utsname uts;
	uname(&uts);
	printf(COLOR_BOLD COLOR_CYAN "* " COLOR_WHITE "LeanInit is running on %s %s %s" COLOR_RESET "\n", uts.sysname, uts.release, uts.machine);

#ifdef FreeBSD
	// Login as root (FreeBSD)
	setlogin("root");
#endif
}


// Shows usage for init
static int usage(void)
{
	printf("%s: Option not permitted\n", __progname);
	printf("Usage: %s [mode] ...\n", __progname);
	printf("  0         Poweroff\n");
	printf("  6         Reboot\n");
	return 1;
}

// Execute rc(8) in a seperate process.
static void bootrc(void)
{
	// Locate rc(8)
	char rc[19];
	FILE *defrc = fopen("/etc/leaninit.d/rc", "r");
	if(defrc == NULL) {
		FILE *bsdrc = fopen("/etc/rc", "r");
		if(bsdrc == NULL)
			single("Neither /etc/rc or /etc/leaninit.d/rc could be found, falling back to single user...");
		else {
			memcpy(rc, "/etc/rc", 8);
			fclose(bsdrc);
		}
	} else {
		memcpy(rc, "/etc/leaninit.d/rc", 19);
		fclose(defrc);
	}

	// Output a message to the console
	printf(COLOR_BOLD COLOR_CYAN "* " COLOR_WHITE "Executing %s" COLOR_RESET "\n", rc);

	// Run rc(8)
	sh(rc);

	// Call sigloop()
	sigloop();
}

// Single user mode
static int single(const char *msg)
{
	// Print msg
	printf(COLOR_BOLD COLOR_CYAN "* " COLOR_WHITE "%s" COLOR_RESET "\n", msg);

	// Use a shell of the user's choice
	char shell[100];
	printf("Shell to use for single user (defaults to /bin/sh):\n");
	scanf("%s", shell);

	// Error checking
	FILE *optsh = fopen(shell, "r");
	if(optsh == NULL) {
		FILE *defsh = fopen("/bin/sh", "r");
		if(defsh == NULL) {
			printf(COLOR_BOLD COLOR_RED "* " COLOR_LIGHT_RED "Could not open either %s or /bin/sh, powering off!" COLOR_RESET "\n", shell);
			halt(SIGUSR2);
		} else {
			printf(COLOR_BOLD COLOR_LIGHT_PURPLE "* " COLOR_YELLOW "Could not open %s, defaulting to /bin/sh" COLOR_RESET "\n", shell);
			fclose(defsh);
			memcpy(shell, "/bin/sh", 7);
		}
	} else
		fclose(optsh);

	// Fork the shell into a seperate process
	int rescue = fork();
	if(rescue == 0) {
		int subshell = fork();
		if(subshell == 0)
			return execl(shell, shell, (char*)0);

		// Power-off when the shell exits
		wait(0);
		return kill(1, SIGUSR2);
	}

	// Call sigloop()
	sigloop();

	// Never reached
	return 1;
}

// Catch signals while killing zombie processes
static void sigloop(void)
{
	// Handle SIGUSR1, SIGUSR2 and SIGINT with sigaction(2)
	struct sigaction actor;
	memset(&actor, 0, sizeof(actor)); // Without this sigaction is ineffective
	actor.sa_handler = halt;
	sigaction(SIGUSR1, &actor, (struct sigaction*)NULL);  // Halt
	sigaction(SIGUSR2, &actor, (struct sigaction*)NULL);  // Poweroff
	sigaction(SIGINT,  &actor, (struct sigaction*)NULL);  // Reboot

	// This perpetual loop kills all zombie processes
	for(;;)
		wait(0);
}

// Halts, reboots or turns off the system
static void halt(int signal)
{
	// Run rc.shutdown
	sh("/etc/leaninit.d/rc.shutdown");

	// Kill all processes
	kill(-1, SIGTERM);
	kill(-1, SIGKILL);

	// Wait until kill(2) is finished
	while(wait(0) > 0);

	// Synchronize the file systems (hardcoded)
	sync();

	// Call reboot(2)
	switch(signal) {
		case SIGUSR1: // Halt
			reboot(SYS_HALT);
			break;

		case SIGUSR2: // Power-off
			reboot(SYS_POWEROFF);
			break;

		case SIGINT:  // Reboot
			reboot(RB_AUTOBOOT);
			break;
	}
}

// Execute a command and wait until it finishes
static void sh(const char *cmd)
{
	int cfork = fork();
	if(cfork == 0)
		execl("/bin/sh", "/bin/sh", cmd, (char*)0);
	wait(0);
}
