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

#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Macros for enable and disable
#define ENABLE 10
#define DISABLE 11

// Program name
extern char *__progname;

// Base paths
static char svc_path[119]  = "/etc/leaninit/svc/";
static char svce_path[120] = "/etc/leaninit/svce/";

// Long options for lsvc
static struct option lsvc_options[] = {
	{ "disable", required_argument, NULL, 'd' },
	{ "enable",  required_argument, NULL, 'e' },
	{ "help",    no_argument,       NULL, '?' },
};

// Usage info
static int usage(int ret, const char *msg, ...)
{
	// Error message
	va_list extra_arg;
	va_start(extra_arg, msg);
	vprintf(msg, extra_arg);
	va_end(extra_arg);

	// Usage info
	printf("Usage: %s [-de?] service ...\n", __progname);
	printf("  -d, --disable        Disable a service\n");
	printf("  -e, --enable         Enable a service\n");
	printf("  -?, --help           Display this text\n");

	// Return the specified status
	return ret;
}

// Function which can enable or disable the specified service (svc)
static int modify_svc(char *svc, int action)
{
	// Exit if the service name is too long
	if(strlen(svc) > 100)
		return usage(1, "The service name '%s' is too long!\n", svc);

	// Paths and file descriptors for the service (svc)
	strncat(svc_path, svc, 100);
	strncat(svce_path, svc, 100);
	FILE *svc_read  = fopen(svc_path, "r");
	FILE *svce_read = fopen(svce_path, "r");

	// Make sure the service actually exists
	if(svc_read == NULL) {
		if(svce_read != NULL) {
			fclose(svce_read);
			printf("There is an error in your configuration, %s appears to exist in /etc/leaninit/svce but not in /etc/leaninit/svc\n", svc);
			return 1;
		} else {
			printf("The service %s does not exist!\n", svc);
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
				// Make the hardlink (with error checking)
				if(link(svc_path, svce_path) != 0) {
					printf("The service %s could not be enabled due to the hardlink failing with errno %s\n", svc, strerror(errno));
					return 1;
				}

				// Cleanup
				printf("The service %s has been enabled.\n", svc);
				return 0;
			} else {
				fclose(svce_read);
				printf("The service %s is already enabled.\n", svc);
				return 0;
			}

		// Disable
		case DISABLE:
			if(svce_read == NULL) {
				printf("The service %s is not enabled.\n", svc);
				return 0;
			}

			// Remove the hardlink (which disables the service), then exit
			fclose(svce_read);
			if(unlink(svce_path) !=  0) {
				printf("The service %s could not be disabled due to unlink failing with errno %s\n", svc, strerror(errno));
				return 1;
			}

			printf("The service %s has been disabled.\n", svc);
			return 0;

		// Fallback
		default:
			printf("Something went wrong, received action %i\n", action);
			return 1;
	}
}

int main(int argc, char *argv[])
{
	// This must be run as root
	if(getuid() != 0)
		return usage(1, "%s must be run as root!\n", __progname);

	// Show usage info if given no arguments
	if(argc == 1)
		return usage(1, "Too few arguments passed to %s!\n", __progname);

	// Get the arguments
	int args;
	while((args = getopt_long(argc, argv, "d:e:?", lsvc_options, NULL)) != -1) {
		switch(args) {

			// Disable
			case 'd':
				return modify_svc(optarg, DISABLE);

			// Enable
			case 'e':
				return modify_svc(optarg, ENABLE);

			// Show usage (return status is 0)
			case '?':
				return usage(0, "");

			// Fallback (return status is 1)
			default:
				return usage(1, "");
		}
	}

	// If we got here due to the user not passing a normal argument (e.g. 'h' without a hyphen), exit
	return usage(1, "You must pass arguments to %s correctly!\n", __progname);
}
