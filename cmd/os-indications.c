/*
 * Copyright © 2019-2020 Johnothan King. All rights reserved.
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
 * os-indications -- Set OsIndications for booting into firmware setup.
 *
 * The libefivar API is used when compiled on FreeBSD, while the efivarfs and
 * C Standard APIs are used on Linux to avoid requiring an extra dependency.
 *
 * This small tool only supports Linux and FreeBSD at this point in time.
 */

#include <leaninit.h>

int main(int argc, char *argv[])
{
#ifndef Linux
    // GUID for OsIndications
    efi_guid_t global_guid = EFI_GLOBAL_GUID;
#endif

    // Error checks
    if unlikely(getuid() != 0) {
        printf(RED "* Permission denied!" RESET "\n");
        return 1;
#ifndef Linux
    } else if very_unlikely(efi_variables_supported() == 0) {
#else
    } else if very_unlikely(access("/sys/firmware/efi/efivars/OsIndicationsSupported-8be4df61-93ca-11d2-aa0d-00e098032b8c", R_OK) != 0) {
#endif
        printf(RED "* This system does not support OsIndications!" RESET "\n");
        return 1;
    }

    // Bitmask flags variable and long options
    bool verbose = true;
    bool unset   = false;
    struct option long_options[] = {
        { "quiet", no_argument, NULL, 'q' },
        { "unset", no_argument, NULL, 'u' },
        { "help",  no_argument, NULL, '?' },
        {  NULL,             0, NULL,  0  }
    };

    // Parse options
    int args;
    while ((args = getopt_long(argc, argv, "qu?", long_options, NULL)) != -1)
        switch (args) {

            // Display usage info
            case '?':
                printf("Usage: %s [-qu?]\n"
                       "  -q, --quiet   Disable output (unless there was an error)\n"
                       "  -u, --unset   Revert changes made by os-indications\n"
                       "  -?, --help    Show this usage information\n", __progname);
                return 1;

            // Quiet mode
            case 'q':
                verbose = false;
                break;

            // Unset OsIndications
            case 'u':
                unset = true;
                break;
        }

#ifdef Linux
    // Write efi_data (Linux efivarfs API)
    if likely(!unset) {
        FILE *fd = fopen("/sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c", "w+");
        unsigned int efi_data[2] = { 0x07, 0x01 };
        if unlikely(fwrite(efi_data, 2, 3, fd) == 0) {
            fclose(fd);
            printf(RED "* Failed to write the changes to OsIndications!" RESET "\n");
            return 1;
        }
        fclose(fd);

    // Delete OsIndications to unset it
    } else
        unlink("/sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c");

#else
    // Set OsIndications for booting into firmware setup (FreeBSD libefivar API)
    unsigned char efi_boot = 0x01;
    if likely(!unset)
        efi_set_variable(global_guid, "OsIndications", &efi_boot, 1, 0x07);

    // Delete OsIndications to unset it
    else
        efi_del_variable(global_guid, "OsIndications");
#endif

    // Notify the user of the change if --quiet was not passed
    if (verbose) {
        if likely (!unset) {
            printf(CYAN "* " WHITE "This system will now boot into the firmware's UI the next time it boots.\n"
                   CYAN "* " WHITE "Run `%s --unset` to revert this change." RESET "\n", __progname);
        } else {
            printf(CYAN "* " WHITE "This system will NOT boot into the firmware's UI the next time it boots.\n"
                   CYAN "* " WHITE "Any prior changes made by %s have been reverted." RESET "\n", __progname);
        }
    }

    return 0;
}
