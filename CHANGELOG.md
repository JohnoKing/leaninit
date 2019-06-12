W.I.P. changelog for LeanInit v2.0.0

**v2.0.0 Release Candidate 1 Changelog:**
* Heavily optimized various parts of LeanInit to improve performance and decrease its footprint.
    * LeanInit's LDFLAGS are now the following: `-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now`.
    * LeanInit's ELF binaries are now stripped of `.comment`, `.gnu-version`, `.GCC.command.line`, `.note.gnu.gold-version` and with `--strip-unneeded` after being built to decrease file size.
    * ZFS file system mounting is now faster.
    * The `zloop()` thread and system shutdown are now both faster as the `zstatus` variable has been replaced with usage of waitpid(3).
    * Services that are not compatible with the target OS will no longer be installed with `make install`.
    * `lservice --status-all` is now faster.
    * Various parts of rc.svc(8) are now faster.
    * Removed the useless usage of `return (0);` from the LeanInit signal handler and stall.
    * `unsigned int` is now used in place of `size_t`.
    * More strings are globbed to increase stability and performance.
    * Globs are now used in place of executing ls(1) when desirable.
    * Archaic usage of `\` has been removed from rc.svc(8).
    * The .gitignore file is now a little bit shorter.
* Added support for standard getties such as agetty(8) and the FreeBSD getty(8).
    * The format for /etc/leaninit/ttys has been drastically changed and is now very similar to the BSD format.
    * LGetty as a result has been removed from the main LeanInit source tree.
* Ported os-indications(8) to FreeBSD using the libefivar API.
* Moved the services folder from `/etc/leaninit/svc.d` to `/etc/leaninit/svc`.
* The consolekit and fstrim services have been removed.
* run() is now called automatically when rc.svc(8) is sourced if the environment variable `$__SVC` is not set to false.
* Reimplemented the old fork() function from pre-1.0 commits with the purpose of writing the PIDs of forked commands.
* Support for `--firmware-setup` has been added to lreboot(8).
* os-indications(8) now supports the `--quiet` flag, which disables output.
* Added support for a new function called restart() to LeanInit services. This function will be run instead of main() when a service is restarted (if this function exists).
* The elogind service now has more comments to improve readability.
* Replaced the erroneous 'NetworkManager' comment in the wicd service with a 'Wicd' comment.
* Removed support for runlevel variables as they were useless and caused race conditions.
    * The command lrunlevel(8) has consequently been removed as well.
* Removed support for `/var/log/leaninit.log.2` (as well as `/var/log/leaninit.log.old.2`) due to security concerns.
* Removed outdated information from the `README.md` file.
* Fixed a bug that caused the wpa_supplicant service to fail to restart on FreeBSD.
* Fixed a bug that caused os-indications(8) to display the wrong name in verbose output if it was renamed to something else.
* Usage information for services is now accurate even if the service is not located in `/etc/leaninit/svc` and/or if no arguments are given.
* `DEF` and `#ifdef` ordering across LeanInit is now more consistent.
* The default rc.conf(5) file has been slightly changed.
* .gitignore will now ignore the built binary of stall.
* Changed the message `Launching sysctl(8)...` to `Executing sysctl(8)...` in rc(8).
* Spaces are now used instead of tabs to improve formatting.
* Improved the formatting of `init.c` specifically.
* Made various improvements to the man pages:
    * Added information about `$__svcname` and `$__svcpid` to the rc.svc(8) man page.
    * Changed 'path' to 'default path' in the rc.svc(8) man page.
	* Improved the formatting of the rc.conf(5) and rc.svc(8) man pages.
    * Fixed some grammar in the os-indications(8) man page.
    * Changed the self reference of `halt(8)` to `halt` in the lhalt(8) man page.
    * Updated the dates in every single man page.
    * Updated the copyright notice in the rc.shutdown(8) man page.

To upgrade from LeanInit v1 to v2, run `make upgrade`.
