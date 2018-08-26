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
static pid_t sh(const char *cmd);
static void  *sigloop(void *earg);
static void  bootrc(void);
static void  sighandle(int signal);
static void  single(const char *msg);
static int   usage(void);

// The main function
int main(int argc, char *argv[])
{
	// PID 1
	if(getpid() == 1) {

		// Open DEFAULT_TTY
		int tty = open(DEFAULT_TTY, O_RDWR);
		login_tty(tty);

#ifdef FreeBSD
		// Login as root (FreeBSD)
		setlogin("root");
#endif

		// Print to DEFAULT_TTY the current platform LeanInit is running on
		struct utsname uts;
		uname(&uts);
		printf(CYAN "* " WHITE "LeanInit is running on %s %s %s" RESET "\n", uts.sysname, uts.release, uts.machine);

		// Start the infinite loop in a seperate thread
		pthread_t loop;
		pthread_create(&loop, NULL, sigloop, 0);

		// Single user support
		int args;
		while((args = getopt(argc, argv, "s")) != -1) {
			switch(args) {
				case 's':
					single("Booting into single user mode...");
					return pthread_join(loop, NULL);
			}
		}

		// Proceed with standard boot
		bootrc();
		return pthread_join(loop, NULL);
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

// Single user mode
static void single(const char *msg)
{
	// Print msg
	printf(CYAN "* " WHITE "%s" RESET "\n", msg);

	// Use a shell of the user's choice
	char shell[100];
	printf(CYAN "* " WHITE "Shell to use for single user (defaults to /bin/sh):" RESET " ");
	scanf("%s", shell);

	// Make sure that the shell exists
	FILE *binsh = fopen(shell, "r");
	if(binsh == NULL) {
		binsh = fopen("/bin/sh", "r");
		if(binsh == NULL) {
			printf(RED "* Could not open either %s or /bin/sh, powering off!" RESET "\n", shell);
			sighandle(SIGUSR2);
			return;
		} else {
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
	sighandle(SIGUSR2);
}

// Catch signals while killing zombie processes
__attribute((noreturn))static void *sigloop(void *earg)
{
	// Handle SIGUSR1, SIGUSR2 and SIGINT with sigaction(2)
	struct sigaction actor;
	memset(&actor, 0, sizeof(actor)); // Without this sigaction is ineffective
	actor.sa_handler = sighandle;     // Set the handler to sighandle()
	sigaction(SIGUSR1, &actor, (struct sigaction*)NULL);  // Halt
	sigaction(SIGUSR2, &actor, (struct sigaction*)NULL);  // Poweroff
	sigaction(SIGINT,  &actor, (struct sigaction*)NULL);  // Reboot

	// This perpetual loop kills all zombie processes
	free(earg);
	for(;;)
		waitpid(-1, NULL, 0);
}

// Halts, reboots or turns off the system
static void sighandle(int signal)
{
	// Run rc.shutdown
	pid_t final = sh("/etc/leaninit.d/rc.shutdown");
	waitpid(final, NULL, 0);

	// Synchronize the file systems (hardcoded)
	sync();

	// Call reboot(2)
	switch(signal) {
		case SIGUSR1: // Halt
			reboot(SYS_HALT);
			break;

		case SIGUSR2: // Poweroff
			reboot(SYS_POWEROFF);
			break;

		case SIGINT:  // Reboot
			reboot(RB_AUTOBOOT);
			break;
	}
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
