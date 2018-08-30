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

// Universal variables
static int current_signal = 0;
static int single_user = 1;

// Shows usage for init
static int usage(void)
{
	printf("%s: Option not permitted\n", __progname);
	printf("Usage: %s [mode] ...\n", __progname);
	printf("  0         Poweroff\n");
	printf("  6         Reboot\n");
	return 1;
}

// Execute a command and return its pid
static pid_t sh(const char *cmd)
{
	pid_t cfork = fork();
	if(cfork == 0) {
		setsid();
		execl("/bin/sh", "/bin/sh", cmd, (char*)0);
	}

	return cfork;
}

// Single user mode
static void single(const char *msg)
{
	// Print msg
	printf(CYAN "* " WHITE "%s" RESET "\n", msg);

	// Use a shell of the user's choice
	char shell[100];
	printf(CYAN "* " WHITE "Shell to use for single user (defaults to /bin/sh):" RESET " ");

	// Obtain user input
	FILE *binsh;
	int len = scanf("%s", shell);
	if(len == 0)
		binsh = NULL;
	else
		binsh = fopen(shell, "r");

	// If the given shell is invalid, check for the existence of /bin/sh
	if(binsh == NULL) {
		binsh = fopen("/bin/sh", "r");
		if(binsh == NULL) {
			if(len == 0)
				printf(RED "* Could not open /bin/sh, powering off!" RESET "\n");
			else
				printf(RED "* Could not open either %s or /bin/sh, powering off!" RESET "\n", shell);

			kill(1, SIGUSR2);
			return;

		// Output a warning
		} else {
			if(len != 0)
				printf(PURPLE "* " YELLOW "Could not open %s, defaulting to /bin/sh" RESET "\n", shell);
			memcpy(shell, "/bin/sh", 8);
		}
	}

	// Close the file descriptor
	fclose(binsh);

	// Fork the shell into a seperate process
	pid_t single = fork();
	if(single == 0)
		execl(shell, shell, (char*)0);

	// Poweroff when the shell exits
	waitpid(single, NULL, 0);
	kill(1, SIGUSR2);
}

// Execute rc(8) (multi-user)
static void bootrc(void)
{
	// Locate rc(8)
	char rc[19];
	FILE *shrc = fopen("/etc/leaninit.d/rc", "r");
	if(shrc == NULL) {
		shrc = fopen("/etc/rc", "r");
		if(shrc == NULL) {
			single("Neither /etc/rc or /etc/leaninit.d/rc could be found, falling back to single user...");
			return;
		} else
			memcpy(rc, "/etc/rc", 8);
	} else
		memcpy(rc, "/etc/leaninit.d/rc", 19);

	// Close the file descriptor
	fclose(shrc);

	// Output a message to the console
	printf(CYAN "* " WHITE "Executing %s" RESET "\n", rc);

	// Run rc(8)
	sh(rc);
}

// Run either bootrc() or single() depending on the mode
static void *initmode(void *ptr)
{
	// Run single-user if single_user == 0, otherwise run multi-user
	if(single_user == 0)
		single("Booting into single user mode...");
	else
		bootrc();

	pthread_exit(ptr);
}

// This perpetual loop kills all zombie processes
__attribute((noreturn)) static void *zloop(void *unused)
{
	free(unused);
	for(;;)
		wait(NULL);
}

// Halts, reboots or turns off the system
static void sighandle(int signal)
{
	current_signal = signal;
	return;
}

// The main function
int main(int argc, char *argv[])
{
	// PID 1
	if(getpid() == 1) {

		// Open DEFAULT_TTY
		int tty = open(DEFAULT_TTY, O_RDWR);
		login_tty(tty);

		// Login as root (FreeBSD)
#		ifdef FreeBSD
		setlogin("root");
#		endif

		// Print to DEFAULT_TTY the current platform LeanInit is running on
		struct utsname uts;
		uname(&uts);
		printf(CYAN "* " WHITE "LeanInit is running on %s %s %s" RESET "\n", uts.sysname, uts.release, uts.machine);

		// Single user support
		int args;
		while((args = getopt(argc, argv, "s")) != -1) {
			switch(args) {
				case 's':
					single_user = 0;
					break;
			}
		}

		// Start zloop() and initmode() in seperate threads
		pthread_t loop, runrc;
		pthread_create(&loop, NULL, zloop, 0);
		pthread_create(&runrc, NULL, initmode, 0);

		// Handle relevant signals while ignoring others
		struct sigaction actor;
		memset(&actor, 0, sizeof(actor)); // Without this sigaction is ineffective
		actor.sa_handler = sighandle;     // Set the handler to sighandle()
		sigaction(SIGUSR1, &actor, (struct sigaction*)NULL);  // Halt
		sigaction(SIGUSR2, &actor, (struct sigaction*)NULL);  // Poweroff
		sigaction(SIGINT,  &actor, (struct sigaction*)NULL);  // Reboot
		pause(); // Wait for a signal to be sent to init

		// Run rc.shutdown
		pid_t final = sh("/etc/leaninit.d/rc.shutdown");
		waitpid(final, NULL, 0);

		// Synchronize all file systems
		sync();

		// Call reboot(2)
		switch(current_signal) {
			case SIGUSR1: // Halt
				return reboot(SYS_HALT);
			case SIGUSR2: // Poweroff
				return reboot(SYS_POWEROFF);
			case SIGINT:  // Reboot
				return reboot(RB_AUTOBOOT);
		}
	}

	// Prevent anyone but root from running this
	if(getuid() != 0) {
		printf(RED "* Permission denied" RESET "\n");
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
