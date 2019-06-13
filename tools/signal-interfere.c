/*
 * Copyright (c) 2019 Johnothan King. All rights reserved.
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
 * signal-interfere - This program exploits the init(8) signal handler
 */

#include <sys/wait.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
extern char *__progname;

int main(int argc, char *argv[])
{
    // This program must be run as root and requires an argument
    if(getuid() != 0) {
        printf("Permission denied!\n");
        return 1;
    } else if(argc < 2) {
        printf("Signal-Interfere requires an argument!\n");
        return 1;
    }

    // Long options struct
    struct option long_options[] = {
        { "alarm", no_argument, 0, 'A' },
        { "int",   no_argument, 0, 'I' },
        { "ill",   no_argument, 0, 'i' },
        { "hup",   no_argument, 0, 'R' },
        { "term",  no_argument, 0, 'T' },
        { "zero",  no_argument, 0, '0' },
        { "usr1",  no_argument, 0, '1' },
        { "usr2",  no_argument, 0, '2' },
        { "help",  no_argument, 0, '?' },
    };

    // Get options
    int signal = 0;
    int args;
    while((args = getopt_long(argc, argv, "AIiRT012?", long_options, NULL)) != -1) {
        switch(args) {

            // Display usage info
            case '?':
                printf("Usage: %s [-AIiRT012?]\n",  __progname);
                printf("  -A, --alarm            SIGALRM\n");
                printf("  -I, --int              SIGINT\n");
                printf("  -i, --ill              SIGILL\n");
                printf("  -R, --hup              SIGHUP\n");
                printf("  -T, --term             SIGTERM\n");
                printf("  -0, --zero             kill(1, 0)\n");
                printf("  -1, --usr1             SIGUSR1\n");
                printf("  -2, --usr2             SIGUSR2\n");
                printf("  -?, --help             Show this usage information\n");
                return 1;

            case 'A':
                signal = SIGALRM;
                break;

            case 'I':
                signal = SIGINT;
                break;

            case 'i':
                signal = SIGILL;
                break;

            case 'R':
                signal = SIGHUP;
                break;

            case 'T':
                signal = SIGTERM;
                break;

            case '1':
                signal = SIGUSR1;
                break;

            case '2':
                signal = SIGUSR2;
                break;
        }
    }

    // Create the signal-interfere daemon. The daemon will not send a signal to init(8) until it is sent SIGTERM.
    pid_t daemon = fork();
    if(daemon == 0) {
        struct sigaction actor;
        memset(&actor, 0, sizeof(actor));
        actor.sa_handler = SIG_IGN;
        sigaction(SIGTERM, &actor, NULL);
        wait(NULL);
        return kill(1, signal);
    }

    // Finish
    printf("Signal-Interfere is now running in the background with pid %d.\n", daemon);
    return 0;
}