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
 * rungetty - Launch a getty with job control
 */

#include "inc.h"

static int usage(void)
{
	printf("Usage: %s tty command & ...\n", __progname);
	return 1;
}

int main(int argc, char *argv[])
{
	// At least two arguments are required
	if(argc < 3)
		return usage();

	// Infinite loop
	int status;
	for(;;) {
		// Launch the getty
		pid_t getty = fork();
		if(getty == 0) {

			// The tty must exist
			int tty = open(argv[1], O_RDWR | O_NOCTTY);
			if(tty < 0)
				return usage();

			// Set the tty as the controlling terminal
			login_tty(tty);
			ioctl(tty, TIOCSCTTY, 1);

			// Attempt to run the getty
			return execl("/bin/sh", "/bin/sh", "-mc", argv[2], (char*)0);
		}

		// Prevent getty spamming
		waitpid(getty, &status, 0);
		if(WEXITSTATUS(status) != 0) {
			printf(RED "* The child process exited with a status of %d" RESET "\n", WEXITSTATUS(status));
			return status;
		}
	}
}
