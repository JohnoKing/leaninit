/*
 * Copyright (c) 2019-2020 Johnothan King. All rights reserved.
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
 * os-indications - This program sets OsIndications for booting into firmware setup.
 * The libefivar API is used when compiled on FreeBSD, while the efivarfs and
 * C Standard APIs are used on Linux to avoid requiring an extra dependency.
 */

#include <leaninit.h>

int main(int argc, char *argv[]) {

    // GUID for OsIndications
#   ifdef FreeBSD
    efi_guid_t global_guid = EFI_GLOBAL_GUID;
#   endif

    // Error checks
    if(getuid() != 0) {
        printf(RED "* Permission denied!" RESET "\n");
        return 1;
#   ifdef FreeBSD
    } else if(efi_variables_supported() == 0) {
#   else
    } else if(access("/sys/firmware/efi/efivars/OsIndicationsSupported-8be4df61-93ca-11d2-aa0d-00e098032b8c", R_OK) != 0) {
#   endif
        printf(RED "* This system does not support OsIndications!" RESET "\n");
        return 1;
    }

    // Long options and other variables
    vint_t efi_boot = 0x0000000000000001;
    vint_t verbose  = 0;
    vint_t unset    = 1;
    struct option long_options[] = {
        { "quiet", no_argument, 0, 'q' },
        { "unset", no_argument, 0, 'u' },
        { "help",  no_argument, 0, '?' },
    };

    // Parse options
    int args;
    while((args = getopt_long(argc, argv, "qu?", long_options, NULL)) != -1) {
        switch(args) {

            // Display usage info
            case '?':
                printf("Usage: %s [-qu?]\n", __progname);
                printf("  -q, --quiet            Disable output (unless there was an error)\n");
                printf("  -u, --unset            Revert changes made by os-indications\n");
                printf("  -?, --help             Show this usage information\n");
                return 1;

            // Quiet mode
            case 'q':
                verbose = 1;
                break;

            // Unset OsIndications
            case 'u':
                unset = 0;
                break;
        }
    }

    // FreeBSD libefivar section
#   ifdef FreeBSD

    // Set OsIndications for booting into firmware setup
    if(unset != 0)
        efi_set_variable(global_guid, "OsIndications", &efi_boot, 1, 0x0000000000000007);

    // Delete OsIndications to unset it
    else
        efi_del_variable(global_guid, "OsIndications");

    // Linux efivarfs section
#   else

    // Write efi_attr and efi_boot
    if(unset != 0) {
        FILE *fd = fopen("/sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c", "w+");
        unsigned int efi_attr = 0x0000000000000007;
        if(fwrite(&efi_attr, 1, 4, fd) == 0 || fwrite(&efi_boot, 1, 1, fd) == 0) {
            fclose(fd);
            printf(RED "* Failed to write the changes to OsIndications!" RESET "\n");
            return 1;
        }
        fclose(fd);

    // Delete OsIndications to unset it
    } else
        unlink("/sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c");
#   endif

    // Notify the user of the change if --quiet was not passed
    if(verbose == 0) {
        if(unset != 0) {
            printf(CYAN "* " WHITE "This system will now boot into the firmware's UI the next time it boots." RESET "\n");
            printf(CYAN "* " WHITE "Run `%s --unset` to revert this change." RESET "\n", __progname);
        } else {
            printf(CYAN "* " WHITE "This system will NOT boot into the firmware's UI the next time it boots." RESET "\n");
            printf(CYAN "* " WHITE "Any prior changes made by %s have been reverted." RESET "\n", __progname);
        }
    }

    return 0;
}
