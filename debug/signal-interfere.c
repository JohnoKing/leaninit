/*
 * Copyright © 2019-2021 Johnothan King. All rights reserved.
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
 * signal-interfere -- This regression test exploits the LeanInit v1.3.6 signal handler
 */

#include <leaninit.h>

// Show usage information
static cold noreturn void usage(void)
{
    printf("Usage: %s [-dAIiRT12?] [delay] ...\n"
           "  -d, --delay   Delay for up to x seconds (max 255)\n"
           "  -A, --alarm   SIGALRM\n"
           "  -I, --int     SIGINT\n"
           "  -i, --ill     SIGILL\n"
           "  -R, --hup     SIGHUP\n"
           "  -T, --term    SIGTERM\n"
           "  -1, --usr1    SIGUSR1\n"
           "  -2, --usr2    SIGUSR2\n"
           "  -?, --help    Show this usage information\n",
           __progname);
    exit(1);
}

int main(int argc, char *argv[])
{
    // This program must be run as root and requires an argument
    if very_unlikely (getuid() != 0) {
        printf(RED "* Permission denied!" RESET "\n");
        return 1;
    } else if unlikely (argc < 2) {
        usage();
        __builtin_unreachable();
    }

    // Long options struct
    struct option long_options[] = { { "delay", required_argument, NULL, 'd' }, { "alarm", no_argument, NULL, 'A' },
                                     { "int", no_argument, NULL, 'I' },         { "ill", no_argument, NULL, 'i' },
                                     { "hup", no_argument, NULL, 'R' },         { "term", no_argument, NULL, 'T' },
                                     { "usr1", no_argument, NULL, '1' },        { "usr2", no_argument, NULL, '2' },
                                     { "help", no_argument, NULL, '?' },        { NULL, 0, NULL, 0 } };

    // Get options
    unsigned int delay = 0;
    int signal = 0;
    int args;
    while ((args = getopt_long(argc, argv, "dAIiRT012?", long_options, NULL)) != -1)
        switch (args) {

            // Set the number of seconds to delay sending the signal (--delay)
            case 'd':
                delay = (unsigned int)atoi(optarg);
                break;

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

            // Display usage info
            case '?':
                usage();
                __builtin_unreachable();
        }

    // Create the signal-interfere daemon. The daemon will not send a signal to init(8) until it is sent SIGTERM.
    pid_t daemon = fork();
    if (daemon == 0) {
        struct sigaction actor;
        actor.sa_handler = SIG_IGN;
        actor.sa_flags = 0;
        sigaction(SIGTERM, &actor, NULL);
        pause();
        if (delay)
            sleep(delay);
        return kill(1, signal);
    } else if unlikely (daemon == -1) {
        perror(RED "* fork() failed with" RESET);
        return 1;
    }

    // Finish
    printf(CYAN "* " WHITE "Signal-Interfere is now running in the background with PID %d." RESET "\n", daemon);
    return 0;
}
