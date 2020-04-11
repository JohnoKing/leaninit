/*
 * Copyright © 2018-2020 Johnothan King. All rights reserved.
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
 * stall - This program is used to bug test LeanInit's runlevel switching and shutdown functionality
 */

#include <leaninit.h>

int main(int argc, char *argv[])
{
    // This program must be run as root
    if(getuid() != 0) {
        printf(RED "* Permission denied!" RESET "\n");
        return 1;
    }
    // SIGSTOP mode can be enabled by passing --sigstop to stall
    unsigned char sigstop = 1;
    if(argc > 2 && strstr(argv[1], "--sigstop") != 0)
        sigstop = 0;

    // Create the stall process itself
    pid_t stall_pid = fork();
    if(stall_pid == 0) {

        // Ignore SIGTERM when stall is not in SIGSTOP mode
        if(sigstop != 0) {
            struct sigaction actor;
            memset(&actor, 0, sizeof(actor));
            actor.sa_handler = SIG_IGN;
            sigaction(SIGTERM, &actor, NULL);
        }

        // Eternal loop
        for(;;)
            sleep(100);

    } else if(stall_pid == -1) {
        perror(RED "* fork() failed with" RESET);
        return 1;
    }

    // Output info
    if(sigstop != 0) {
        printf(CYAN "* " WHITE "Stall is now running in the background with PID %d." RESET "\n", stall_pid);
        printf(CYAN "* " WHITE "Stall cannot be killed with SIGTERM (use SIGKILL instead)." RESET "\n");
        printf(CYAN "* " WHITE "To execute stall in SIGSTOP mode, pass --sigstop when executing stall." RESET "\n");
    } else {
        kill(stall_pid, SIGSTOP);
        printf(CYAN "* " WHITE "Stall is now in the background paused by SIGSTOP with PID %d." RESET "\n", stall_pid);
        printf(CYAN "* " WHITE "You must send stall SIGCONT then SIGTERM to kill it." RESET "\n");
    }

    return 0;
}
