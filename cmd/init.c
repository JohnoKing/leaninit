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
static int single_user    = 1;

// Shows usage for init
static int usage(void)
{
	printf("%s: Option not permitted\n", __progname);
	printf("Usage: %s [mode] ...\n", __progname);
	printf("  0           Poweroff\n");
	printf("  1, S, s     Switch to single-user mode\n");
	printf("  2, 3, 4, 5  Switch to multi-user mode\n");
	printf("  Q, q        Reloads the current runlevel\n");
	printf("  6           Reboot\n");
	return 1;
}

// Execute a script
static void sh(const char *cmd)
{
	pid_t child = fork();
	if(child == 0) {
		setsid();
		execl("/bin/sh", "/bin/sh", cmd, (char*)0);
	}

	waitpid(child, (int*)0, 0);
}

// Single user mode
static void single(const char *msg)
{
	// Print msg
	printf(CYAN "* " WHITE "%s" RESET "\n", msg);

	// Use a shell of the user's choice
	char shell[101];
	printf(CYAN "* " WHITE "Shell to use for single user (defaults to /bin/sh):" RESET " ");
	scanf("%s", shell);

	// If the given shell is invalid, check for the existence of /bin/sh
	if(access(shell, X_OK) != 0) {
		if(access("/bin/sh", X_OK) != 0) {
			printf(RED "* Could not execute %s or /bin/sh, powering off!" RESET "\n", shell);
			kill(1, SIGUSR2);
			return;

		// Output a warning
		} else {
			printf(PURPLE "* " YELLOW "Could not execute %s, defaulting to /bin/sh" RESET "\n", shell);
			memcpy(shell, "/bin/sh", 8);
		}
	}

	// Login as root
	setenv("LOGNAME", "root", 1);
	setenv("USER", "root", 1);
	setenv("HOME", "/root", 1);

	// Fork the shell into a seperate process
	pid_t single = fork();
	if(single == 0)
		execl(shell, shell, (char*)0);
	waitpid(single, (int*)0, 0);

	// Poweroff when the shell exits (avoids conflicting with 'init 5')
	if(single_user == 0)
		kill(1, SIGUSR2);
}

// Execute rc(8) (multi-user)
static void bootrc(void)
{
	// Locate rc(8)
	char rc[19];
	if(access("/etc/leaninit.d/rc", X_OK) != 0) {
		if(access("/etc/rc", X_OK) != 0) {
			single_user = 0;
			single("Neither /etc/rc or /etc/leaninit.d/rc could be found, falling back to single user...");
			return;
		} else
			memcpy(rc, "/etc/rc", 8);
	} else
		memcpy(rc, "/etc/leaninit.d/rc", 19);

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
		wait((int*)0);
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
		pthread_create(&loop, (pthread_attr_t*)NULL, zloop, 0);
		pthread_create(&runrc, (pthread_attr_t*)NULL, initmode, 0);

		// Handle relevant signals while ignoring others
		struct sigaction actor;
		memset(&actor, 0, sizeof(actor)); // Without this sigaction is ineffective
		actor.sa_handler = sighandle;     // Set the handler to sighandle()
		sigaction(SIGUSR1, &actor, (struct sigaction*)NULL); // Halt
		sigaction(SIGUSR2, &actor, (struct sigaction*)NULL); // Poweroff
		sigaction(SIGTERM, &actor, (struct sigaction*)NULL); // Single-user
		sigaction(SIGILL,  &actor, (struct sigaction*)NULL); // Multi-user
		sigaction(SIGHUP,  &actor, (struct sigaction*)NULL); // Reloads everything
		sigaction(SIGINT,  &actor, (struct sigaction*)NULL); // Reboot

		// Signal handling loop
		for(;;) {
			// Wait for a signal to be sent to init
			pause();

			// Cancel when the runlevel is already the currently running one
			if(current_signal == SIGILL && single_user == 1)
				printf(PURPLE "* " YELLOW "LeanInit is already in multi-user mode..." RESET "\n");

			else if(current_signal == SIGTERM && single_user == 0)
				printf(PURPLE "* " YELLOW "LeanInit is already in single-user mode..." RESET "\n");

			// Switch the current runlevel
			else {
				// Run rc.shutdown
				sh("/etc/leaninit.d/rc.shutdown");
				kill(-1, SIGKILL);

				// Re-open DEFAULT_TTY
				close(tty);
				tty = open(DEFAULT_TTY, O_RDWR);
				login_tty(tty);

				// Synchronize all file systems
				sync();

				// Call reboot(2)
				switch(current_signal) {

					// Halt
					case SIGUSR1:
						return reboot(SYS_HALT);

					// Poweroff
					case SIGUSR2:
						return reboot(SYS_POWEROFF);

					// Switch to single-user
					case SIGTERM:
						single_user = 0;
						break;

					// Switch to multi-user
					case SIGILL:
						single_user = 1;
						break;

					// Reboot
					case SIGINT:
						return reboot(RB_AUTOBOOT);
				}

				// Reload
				pthread_kill(runrc, SIGTERM);
				pthread_create(&runrc, (pthread_attr_t*)NULL, initmode, 0);
			}
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

	// Runlevel support
	switch(*argv[1]) {

		// Poweroff
		case '0':
			return kill(1, SIGUSR2);

		// Single-user
		case '1':
		case 'S':
		case 's':
			return kill(1, SIGTERM);

		// Multi-user
		case '2':
		case '3':
		case '4':
		case '5':
			return kill(1, SIGILL);

		// Reload everything
		case 'Q':
		case 'q':
			return kill(1, SIGHUP);

		// Reboot
		case '6':
			return kill(1, SIGINT);

		// Fallback
		default:
			return usage();
	}
}
