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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Base paths
char svc_path[273]  = "/etc/leaninit/svc/";
char svce_path[274] = "/etc/leaninit/svce/";

// Usage info for lsvc
static void usage(const char *msg, ...)
{
	// Error message
	va_list extra_arg;
	va_start(extra_arg, msg);
	vprintf(msg, extra_arg);
	va_end(extra_arg);

	// Usage info
	printf("\nUsage: lsvc [-de] service ...\n");
	printf("  -d            Disable a service\n");
	printf("  -e            Enable a service\n");

	// Always exit with exit code 1
	exit(1);
}

// Function which disables the specified service (svc)
static void disable(char *svc)
{
	// Path and file descriptor for the service (svc)
	strncat(svce_path, svc, 255);
	FILE *svce_read = fopen(svce_path, "r");

	// Error checking
	if(svce_read == NULL) {
		strncat(svc_path, svc, 255);
		FILE *svc_read = fopen(svc_path, "r");
		if(svc_read != NULL) {
			fclose(svc_read);
			printf("The service %s is not enabled.\n", svc);
			exit(0);
		} else
			usage("The service %s does not exist!\n", svc);
	}

	// Remove the hardlink (which disables the service), then exit
	fclose(svce_read);
	unlink(svce_path);
	printf("The service %s has been disabled.\n", svc);
	exit(0);
}

// Function which enables the specified service (svc)
static void enable(char *svc)
{
	// Paths and file descriptors for the service (svc)
	strncat(svc_path, svc, 255);
	strncat(svce_path, svc, 255);
	FILE *svc_read  = fopen(svc_path, "r");
	FILE *svce_read = fopen(svce_path, "r");

	// Error checking
	if(svc_read == NULL)
		usage("The service %s does not exist!\n", svc);
	else if(svce_read != NULL) {
		fclose(svce_read);
		printf("The service %s is already enabled.\n", svc);
		exit(0);
	}

	// Make the hardlink
	link(svc_path, svce_path);

	// Cleanup
	fclose(svc_read);
	printf("The service %s has been enabled.\n", svc);
	exit(0);
}

int main(int argc, char *argv[])
{
	// This must be run as root
	if(getuid() != 0)
		usage("You must run lsvc as root!\n");

	// Show usage info if given no arguments
	if(argc == 1)
		usage("Do you want to enable or disable a service?\n");

	// Exit if the service name is too long
	if(strlen(argv[2]) > 255)
		usage("The service name '%s' is too long!\n", argv[2]);

	// Get the arguments
	int args;
	while((args = getopt(argc, argv, "d:e:")) != -1) {
		switch(args) {

			// Disable
			case 'd':
				disable(argv[2]);

			// Enable
			case 'e':
				enable(argv[2]);

			// Fallback
			default:
				usage("");
		}
	}

	// If we got here due to the user not passing a normal argument (e.g. 'h' without a hyphen), exit
	usage("You must pass arguments to lsvc correctly!\n");
}
