/*
 * Copyright (c) 2018 Johnothan King. All rights reserved.
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
 * lgetty - A minimal getty that respawns itself
 */

#include "inc.h"

static int usage(void)
{
	printf("Usage: %s tty & ...\n", __progname);
	return 1;
}

int main(int argc, char *argv[])
{
	// An argument is required
	if(argc < 2)
		return usage();

	// Find login(1)
	char login_cmd[19];
	if(access("/bin/login", X_OK) == 0)
		memcpy(login_cmd, "/bin/login ", 12);
	else if(access("/usr/bin/login", X_OK) == 0)
		memcpy(login_cmd, "/usr/bin/login ", 16);
	else {
		printf(RED "* Could not find login(1) (please symlink it to either /bin/login or /usr/bin/login and give it executable permissions)" RESET "\n");
		return 127;
	}

	// Pass -p to login when debug mode is enabled
	unsigned int debug = 1;
	if((argc > 2) && (strcmp(argv[2], "debug") == 0)) {
		strncat(login_cmd, "-p ", 4);
		debug = 0;
	}

	// Infinite loop
	int status;
	for(;;) {

		// Child process for login(1)
		pid_t login = fork();
		if(login == 0) {

			// This is required for the subshell to have job control on FreeBSD
#			ifdef FreeBSD
			revoke(argv[1]);
#			endif

			// Verify the tty exists
			int tty = open(argv[1], O_RDWR | O_NOCTTY);
			if(tty == -1 || isatty(tty) == 0)
				return usage();

			// Set the tty as the controlling terminal
			login_tty(tty);
			dup2(tty, STDIN_FILENO);
			dup2(tty, STDOUT_FILENO);
			dup2(tty, STDERR_FILENO);
			ioctl(tty, TIOCSCTTY, 1);

			// Get user input
			char input[100], cmd[119];
			printf(CYAN "\n* " WHITE "%s login:" RESET " ", argv[1]);
			scanf("%s", input);
			memcpy(cmd,  login_cmd, 19);
			strncat(cmd, input,    100);

			// Because -p is passed to login(1) to preserve the environment, $HOME must be corrected (for debug mode)
			if(debug == 0) {
				if(strcmp("root", input) == 0)
					putenv("HOME=/root");
				else {
					char home[112];
					memcpy(home, "HOME=/home/", 12);
					strncat(home, input, 100);
					putenv(home);
				}
			}

			// Execute login(1)
			return execl("/bin/sh", "/bin/sh", "-mc", cmd, NULL);
		}

		// Prevent spamming
		waitpid(login, &status, 0);
		if(WEXITSTATUS(status) != 0) {
			printf(RED "* login(1) exited with a return status of %d" RESET "\n", WEXITSTATUS(status));
			return status;
		}
	}
}
