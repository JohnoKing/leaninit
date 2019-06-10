/*
 * Copyright (c) 2018-2019 Johnothan King. All rights reserved.
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
 * halt - Shutdown, reboot or halt system
 */

#include "inc.h"

// Main function for halt
int main(int argc, char *argv[])
{
	// halt(8) can only be run by root
	if(!getuid()) {
		printf(RED "* Permission denied" RESET "\n");
		return 1;
	}

	// Long options for halt
	struct option halt_long_options[] = {
		{ "firmware-setup", no_argument, 0, 'F' },
		{ "force",    no_argument, 0, 'f' },
		{ "halt",     no_argument, 0, 'h' },
		{ "no-wall",  no_argument, 0, 'l' },
		{ "poweroff", no_argument, 0, 'p' },
		{ "reboot",   no_argument, 0, 'r' },
		{ "help",     no_argument, 0, '?' },
	};

	// Bool and int variables
	bool force = false; // If this is true, skip sending a signal to init
	bool osin  = false; // If this is true, call os-indications(8) before rebooting
	bool wall  = true;  // Used for syslog(3) messages
	int signal;         // For signals that will be sent to init

	// Set the signal to send to init(8) using __progname
	if(!strstr(__progname,      "halt"))     signal = SIGUSR1; // Halt
	else if(!strstr(__progname, "poweroff")) signal = SIGUSR2; // Poweroff
	else if(!strstr(__progname, "reboot"))   signal = SIGINT;  // Reboot

	// Hibernate
#	ifdef Linux
	else if(!strstr(__progname, "zzz"))
		return reboot(RB_SW_SUSPEND);
#	endif

	// Not valid
	else {
		printf(RED "* You cannot run halt as %s" RESET "\n", __progname);
		return 1;
	}

	// Parse any given options
	int args;
	while((args = getopt_long(argc, argv, "fFhlpr?", halt_long_options, NULL)) != -1) {
		switch(args) {

			// Display usage info
			case '?':
				printf("Usage: %s [-fFhlpr?]\n",  __progname);
				printf("  -f, --force            Do not send a signal to init, call reboot(2) directly\n");
				printf("  -F, --firmware-setup   Reboot into the firmware setup\n");
				printf("  -h, --halt             Force halt, even when called as poweroff or reboot\n");
				printf("  -l, --no-wall          Turn off wall messages\n");
				printf("  -p, --poweroff         Force poweroff, even when called as halt or reboot\n");
				printf("  -r, --reboot           Force reboot, even when called as halt or poweroff\n");
				printf("  -?, --help             Show this usage information\n");
				return 1;

			// --force
			case 'f':
				force = true;
				break;

			// Firmware setup
			case 'F':
				osin = true;
				break;

			// Force halt
			case 'h':
				signal = SIGUSR1;
				break;

			// Turn off wall messages
			case 'l':
				wall = 1;
				break;

			// Force poweroff
			case 'p':
				signal = SIGUSR2;
				break;

			// Force reboot
			case 'r':
				signal = SIGINT;
				break;
		}
	}

	// Syslog
	if(wall) {
		openlog(__progname, LOG_CONS, LOG_AUTH);
		syslog(LOG_CRIT, "The system is going down NOW!");
		closelog();
	}

	// Run os-indications(8) if --firmware-setup was passed
	if(osin) {
		pid_t child = fork();
		if(child) {
			char *cargv[] = { "os-indications", "-q", NULL };
			execve("/sbin/os-indications", cargv, environ);
		}

		waitpid(child, NULL, 0);
	}

	// Skip init if force is true
	if(force) {
		sync(); // Always call sync(2)

		switch(signal) {
			case SIGUSR1: // Halt
				return reboot(SYS_HALT);
			case SIGUSR2: // Poweroff
				return reboot(SYS_POWEROFF);
			case SIGINT:  // Reboot
				return reboot(RB_AUTOBOOT);
		}
	}

	// Send the correct signal to init
	return kill(1, signal);
}
