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
static unsigned int zstatus     = 0;
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
	printf("  7           Halt\n");
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

// Open the tty
static int open_tty(void)
{
#	ifdef FreeBSD
	revoke(CONSOLE);
#	endif
	int tty = open(CONSOLE, O_RDWR | O_NOCTTY);
	login_tty(tty);
	dup2(tty, STDIN_FILENO);
	dup2(tty, STDOUT_FILENO);
	dup2(tty, STDERR_FILENO);
	ioctl(tty, TIOCSCTTY, 1);
	return tty;
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

	// Fork the shell into a seperate process
	if(fork() == 0) {
		open_tty();
		execl(shell, shell, NULL);
	}
}

// Execute rc(8) (multi-user)
static void multi(void)
{
	// Start multi-user mode as runlevel 5
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

// Run either single() or multi() depending on the runlevel
static void *chlvl(__attribute((unused)) void *ptr)
{
	// Run single-user if single_user == 0, otherwise run multi-user
	if(single_user == 0)
		single("Booting into single user mode...");
	else
		multi();

	return NULL;
}

// This perpetual loop kills all zombie processes
__attribute((noreturn)) static void *zloop(__attribute((unused)) void *ptr)
{
	for(;;) {
		pid_t pid = wait(NULL);
		if(pid == -1 && zstatus == 0)
			zstatus = 1;
		else if(pid != -1 && zstatus == 1)
			zstatus = 0;
	}
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
		int tty = open_tty();

		// Login as root
		setenv("HOME",    "/root", 1);
		setenv("LOGNAME", "root",  1);
		setenv("USER",    "root",  1);

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

		// Start zloop() and chlvl() in seperate threads
		pthread_t loop, runlvl;
		pthread_create(&loop,   NULL, zloop, NULL);
		pthread_create(&runlvl, NULL, chlvl, NULL);

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
				// Synchronize all file systems (Pass 1)
				printf(CYAN "* " WHITE "Synchronizing all file systems (Pass 1)..." RESET "\n");
				sync();

				// Run rc.shutdown (multi-user)
				if(single_user != 0)
					sh("/etc/leaninit.d/rc.shutdown");

				// Kill all remaining processes
				printf(CYAN "* " WHITE "Killing all remaining processes..." RESET "\n");
				kill(-1, SIGTERM);
				pthread_kill(runlvl, SIGKILL);
				pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
				unsigned int timer = 0;
				pthread_join(runlvl, NULL);
				while(timer != 70) {
					usleep(100000);
					pthread_mutex_lock(&mutex);
					if(zstatus != 0) break;
					pthread_mutex_unlock(&mutex);
					timer++;
				}
				kill(-1, SIGKILL);
				pthread_mutex_destroy(&mutex);

				// Reopen the console
				close(tty);
				tty = open_tty();

				// Synchronize file systems again (Pass 2)
				printf(CYAN "* " WHITE "Synchronizing all file systems (Pass 2)..." RESET "\n");
				sync();

				// Handle the given signal properly
				switch(current_signal) {

					// Halt
					case SIGUSR1:
						printf(CYAN "* " WHITE "The system will now halt!" RESET "\n");
						return reboot(SYS_HALT);

					// Poweroff
					case SIGUSR2:
						printf(CYAN "* " WHITE "The system will now poweroff!" RESET "\n");
						return reboot(SYS_POWEROFF);

					// Reboot
					case SIGINT:
						printf(CYAN "* " WHITE "The system will now reboot!" RESET "\n");
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

				// Reload the runlevel thread
				pthread_create(&runlvl, NULL, chlvl, NULL);
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

		// Halt
		case '7':
			return kill(1, SIGUSR1);

		// Fallback
		default:
			return usage();
	}
}
