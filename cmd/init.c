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
static unsigned int single_user = 1;
static int current_signal       = 0;

// Shows usage for init
static int usage(void)
{
	printf("%s: Option not permitted\n", __progname);
	printf("Usage: %s [runlevel] ...\n", __progname);
	printf("  0           Poweroff\n");
	printf("  1, S, s     Switch to single-user mode\n");
	printf("  2, 3, 4, 5  Switch to multi-user mode\n");
	printf("  Q, q        Reload the current runlevel\n");
	printf("  6           Reboot\n");
	return 1;
}

// Execute a script
static void sh(const char *cmd)
{
	pid_t child = fork();
	if(child == 0) {
		setsid();
		execl("/bin/sh", "/bin/sh", cmd, NULL);
	}

	waitpid(child, NULL, 0);
}

// Single user mode
static void single(const char *msg)
{
	// Start single user mode as runlevel 1
	setenv("RUNLEVEL", "1", 1);
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
	setenv("HOME",    "/root", 1);
	setenv("LOGNAME", "root",  1);
	setenv("USER",    "root",  1);

	// Fork the shell into a seperate process
	if(fork() == 0)
		execl(shell, shell, NULL);
}

// Execute rc(8) (multi-user)
static void bootrc(void)
{
	// Start multi user mode as runlevel 5
	setenv("RUNLEVEL", "5", 1);

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

	// Run rc(8)
	printf(CYAN "* " WHITE "Executing %s" RESET "\n", rc);
	sh(rc);
}

// Run either bootrc() or single() depending on the mode
static void *initmode(__attribute((unused)) void *ptr)
{
	// Run single-user if single_user == 0, otherwise run multi-user
	if(single_user == 0)
		single("Booting into single user mode...");
	else
		bootrc();

	return NULL;
}

// This perpetual loop kills all zombie processes
__attribute((noreturn)) static void *zloop(__attribute((unused)) void *ptr)
{
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

		// Open the console
		int tty = open("/dev/console", O_RDWR);
		login_tty(tty);

		// Print the current platform LeanInit is running on
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
		pthread_create(&loop,  NULL, zloop,    NULL);
		pthread_create(&runrc, NULL, initmode, NULL);

		// Handle relevant signals while ignoring others
		struct sigaction actor;
		memset(&actor, 0, sizeof(actor)); // Without this sigaction is ineffective
		actor.sa_handler = sighandle;     // Set the handler to sighandle()
		sigaction(SIGUSR1, &actor, NULL); // Halt
		sigaction(SIGUSR2, &actor, NULL); // Poweroff
		sigaction(SIGTERM, &actor, NULL); // Single-user
		sigaction(SIGILL,  &actor, NULL); // Multi-user
		sigaction(SIGHUP,  &actor, NULL); // Reloads everything
		sigaction(SIGINT,  &actor, NULL); // Reboot

		// Signal handling loop
		for(;;) {
			// Wait for a signal to be sent to init
			pause();

			// Cancel when the runlevel is already the currently running one
			if(current_signal == SIGILL && single_user == 1)
				printf(PURPLE "* " YELLOW "LeanInit is already in multi-user mode..."  RESET "\n");

			else if(current_signal == SIGTERM && single_user == 0)
				printf(PURPLE "* " YELLOW "LeanInit is already in single-user mode..." RESET "\n");

			// Switch the current runlevel
			else {
				// Run rc.shutdown
				sh("/etc/leaninit.d/rc.shutdown");
				kill(-1, SIGKILL);

				// Re-open /dev/console
				close(tty);
				tty = open("/dev/console", O_RDWR);
				login_tty(tty);

				// Synchronize all file systems
				printf(CYAN "* " WHITE "Synchronizing all file systems..." RESET "\n");
				sync();

				// Handle the given signal properly
				switch(current_signal) {

					// Halt
					case SIGUSR1:
						return reboot(SYS_HALT);

					// Poweroff
					case SIGUSR2:
						return reboot(SYS_POWEROFF);

					// Reboot
					case SIGINT:
						return reboot(RB_AUTOBOOT);

					// Switch to single-user
					case SIGTERM:
						setenv("PREVLEVEL", "5", 1);
						single_user = 0;
						break;

					// Switch to multi-user
					case SIGILL:
						setenv("PREVLEVEL", "1", 1);
						single_user = 1;
						break;
				}

				// Reload
				pthread_kill(runrc, SIGTERM);
				pthread_join(runrc, NULL);
				pthread_create(&runrc, NULL, initmode, NULL);
			}
		}
	}

	// Prevent anyone but root from running this
	if(getuid() != 0) {
		printf(RED "* Permission denied" RESET "\n");
		return 1;
	}

	// Emulate some SysV-like behavior when re-executed
	if(argc < 2)
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
