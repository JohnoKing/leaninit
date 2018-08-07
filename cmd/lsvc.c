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
 * lsvc - A program which enables and disables LeanInit services
 */

#include "inc.h"

// Base paths
static char svcd_path[123] = "/etc/leaninit.d/svc.d/";
static char svce_path[123] = "/etc/leaninit.d/svc.e/";

// Force flag
static char *force    = "n";
static bool force_svc = false;

// Long options for lsvc
static struct option lsvc_options[] = {
	{ "disable", required_argument, 0, 'd' },
	{ "enable",  required_argument, 0, 'e' },
	{ "force",   no_argument,       0, 'f' },
	{ "info",    required_argument, 0, 'i' },
	{ "restart", required_argument, 0, 'r' },
	{ "stop",    required_argument, 0, 'q' },
	{ "start",   required_argument, 0, 's' },
	{ "status",  required_argument, 0, 'i' },
	{ "help",    no_argument,       0, '?' },
};

// Usage info
static int usage(const char *starcolor, const char *textcolor, const char *msg, ...)
{
	// Error message
	va_list vargs;
	va_start(vargs, msg);
	printf(COLOR_BOLD "%s* %s", starcolor, textcolor);
	vprintf(msg, vargs);
	printf(COLOR_RESET);
	va_end(vargs);

	// Usage info
	printf("Usage: %s [-defirqs?] service ...\n", __progname);
	printf("  -d, --disable        Disable a service\n");
	printf("  -e, --enable         Enable a service\n");
	printf("  -f, --force          Force lsvc to do the specified action\n");
	printf("  -i, --info, --status Shows a service's current status\n");
	printf("  -r, --restart        Restart a service\n");
	printf("  -q, --stop           Stop a service\n");
	printf("  -s, --start          Start a service\n");
	printf("  -?, --help           Display this text\n");

	// Return 1
	return 1;
}

// Shows the current status of the specified service
static int status_svc(const char *svc)
{
	// Exit if the service name is too long
	if(strlen(svc) > 100)
		return usage(COLOR_RED, COLOR_LIGHT_RED, "The service name '%s' is too long!\n", svc);

	// Get the path the service's status file
	char status_path[129] = "/var/log/leaninit/";
	char status[20];
	strncat(status_path, svc, 100);
	strncat(status_path, ".status", 8);

	// Attempt to open the .status file
	FILE *svc_status = fopen(status_path, "r");
	if(svc_status == NULL) {
		printf(COLOR_BOLD COLOR_LIGHT_BLUE"* " COLOR_WHITE "There is no current status for %s\n" COLOR_RESET, svc);
		return 0;
	}

	// Get the status of svc
	fgets(status, 20, svc_status);
	fclose(svc_status);
	printf(COLOR_BOLD COLOR_LIGHT_BLUE "* " COLOR_WHITE "The current status of %s is:" "%s" COLOR_RESET, svc, status);

	// Always return 0
	return 0;
}

// This function can start, stop, restart, enable and disable LeanInit services
static int modify_svc(const char *svc, int action)
{
	// Exit if the service name is too long
	if(strlen(svc) > 100)
		return usage(COLOR_RED, COLOR_LIGHT_RED, "The service name '%s' is too long!\n", svc);

	// Paths and file descriptors for the service
	strncat(svcd_path, svc, 100);
	strncat(svce_path, svc, 100);
	FILE *svc_read  = fopen(svcd_path, "r");
	FILE *svce_read = fopen(svce_path, "r");

	// Make sure the service actually exists
	if(svc_read == NULL) {
		if(svce_read != NULL) {
			fclose(svce_read);
			printf(COLOR_BOLD COLOR_RED"* " COLOR_LIGHT_RED "There is an error in your configuration, %s appears to exist in /etc/leaninit.d/svc.e but not in /etc/leaninit.d/svc.d\n" COLOR_RESET, svc);
			return 1;
		} else {
			printf(COLOR_BOLD COLOR_RED "* " COLOR_LIGHT_RED "%s does not exist\n" COLOR_RESET, svc);
			return 1;
		}
	}

	// This is no longer needed
	fclose(svc_read);

	// What to do
	switch(action) {

		// Enable
		case ENABLE:
			if(svce_read == NULL) {
				// Enable the service
				FILE *enable = fopen(svce_path, "a");
				fclose(enable);
				printf(COLOR_BOLD COLOR_LIGHT_GREEN"* " COLOR_WHITE "%s has been enabled\n" COLOR_RESET, svc);
				return 0;
			} else {
				// The service is already enabled
				fclose(svce_read);
				printf(COLOR_BOLD COLOR_LIGHT_PURPLE"* " COLOR_YELLOW "%s is already enabled\n" COLOR_RESET, svc);
				return 0;
			}

		// Disable
		case DISABLE:
			// Return if the service is not enabled
			if(svce_read == NULL) {
				printf(COLOR_BOLD COLOR_LIGHT_PURPLE "* " COLOR_YELLOW "%s is not enabled\n" COLOR_RESET, svc);
				return 0;
			}

			// Remove the file in /etc/leaninit.d/svc.e
			fclose(svce_read);
			if(unlink(svce_path) != 0) {
				printf(COLOR_BOLD COLOR_RED "* " COLOR_LIGHT_RED "%s could not be disabled due to unlink failing with errno %s\n" COLOR_RESET, svc, strerror(errno));
				return 1;
			}
			printf(COLOR_BOLD COLOR_LIGHT_GREEN "* " COLOR_WHITE "%s has been disabled\n" COLOR_RESET, svc);
			return 0;

		// Start
		case START:
			if((svce_read == NULL) && (force_svc != true)) {
				printf(COLOR_BOLD COLOR_RED"* " COLOR_LIGHT_RED "%s is not enabled\n" COLOR_RESET, svc);
				return 1;
			} else if(svce_read != NULL)
				fclose(svce_read);

			// Execute svc-start
			return execl("/bin/sh", "/bin/sh", "/etc/leaninit.d/svc-start", svc, "lsvc", (char*)0);

		// Stop
		case STOP:
			if((svce_read == NULL) && (force_svc != true)) {
				printf(COLOR_BOLD COLOR_RED"* " COLOR_LIGHT_RED "%s is not enabled\n" COLOR_RESET, svc);
				return 1;
			} else if(svce_read != NULL)
				fclose(svce_read);

			// Execute svc-stop
			return execl("/bin/sh", "/bin/sh", "/etc/leaninit.d/svc-stop", svc, force, (char*)0);

		// Restart
		case RESTART:
			if((svce_read == NULL) && (force_svc != true)) {
				printf(COLOR_BOLD COLOR_RED"* " COLOR_LIGHT_RED "%s is not enabled\n" COLOR_RESET, svc);
				return 1;
			} else if(svce_read != NULL)
				fclose(svce_read);

			// First, stop the service
			pid_t stop = fork();
			if(stop == 0)
				return execl("/bin/sh", "/bin/sh", "/etc/leaninit.d/svc-stop", svc, force, (char*)0);

			// Then, start it again
			wait(0);
			return execl("/bin/sh", "/bin/sh", "/etc/leaninit.d/svc-start", svc, "lsvc", (char*)0);
	}

	return -1;
}

int main(int argc, char *argv[])
{
	// This must be run as root
	if(getuid() != 0) {
		printf(COLOR_BOLD COLOR_RED "* " COLOR_LIGHT_RED "%s\n" COLOR_RESET, strerror(EPERM));
		return 1;
	}

	// Show usage info if given no arguments
	if(argc == 1)
		return usage(COLOR_RED, COLOR_LIGHT_RED, "Too few arguments passed\n");

	// Get the arguments
	int args;
	while((args = getopt_long(argc, argv, "fd:e:i:r:q:s:?", lsvc_options, NULL)) != -1) {
		switch(args) {

			// Force flag
			case 'f':
				force_svc = true;
				force     = "f";
				break;

			// Disable
			case 'd':
				return modify_svc(optarg, DISABLE);

			// Enable
			case 'e':
				return modify_svc(optarg, ENABLE);

			// Status of a service
			case 'i':
				return status_svc(optarg);

			// Restart
			case 'r':
				return modify_svc(optarg, RESTART);

			// Stop
			case 'q':
				return modify_svc(optarg, STOP);

			// Start
			case 's':
				return modify_svc(optarg, START);

			// Show usage
			case '?':
				return usage(COLOR_LIGHT_BLUE, COLOR_WHITE, "");
		}
	}

	// If we got here due to the user not passing a normal argument (e.g. 'h' without a hyphen), exit
	return usage(COLOR_RED, COLOR_LIGHT_RED, "You must pass arguments correctly!\n");
}
