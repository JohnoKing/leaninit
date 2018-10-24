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
	char login_cmd[18];
	if(access("/bin/login", X_OK) == 0)
		memcpy(login_cmd, "/bin/login -p", 14);
	else if(access("/usr/bin/login", X_OK) == 0)
		memcpy(login_cmd, "/usr/bin/login -p", 18);
	else {
		printf(RED "* Could not find login(1) (please symlink it to either /bin/login or /usr/bin/login)" RESET "\n");
		return 127;
	}

	// Infinite loop
	int status;
	for(;;) {

		// Run login(1)
		pid_t login = fork();
		if(login == 0) {

			// The tty must exist
			int tty = open(argv[1], O_RDWR | O_NOCTTY);
			if(isatty(tty) == 0)
				return usage();

			// Set the tty as the controlling terminal
			login_tty(tty);
			dup2(tty, STDIN_FILENO);
			dup2(tty, STDOUT_FILENO);
			dup2(tty, STDERR_FILENO);
			ioctl(tty, TIOCSCTTY, 1);

			// Execute /bin/login with the -p flag to preserve the current environment
			printf(CYAN "* " WHITE "Executing /bin/login on %s" RESET "\n\n", argv[1]);
			return execl("/bin/sh", "/bin/sh", "-mc", login_cmd, NULL);
		}

		// Prevent spamming
		waitpid(login, &status, 0);
		if(WEXITSTATUS(status) != 0) {
			printf(RED "* login(1) exited with a return status of %d" RESET "\n", WEXITSTATUS(status));
			return status;
		}
	}
}
