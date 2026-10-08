# (Free)MiNT oldstuff package recovery

https://freemint.github.io/sparemint/sparemint/html/packages/oldstuff.html contains RPM and SRPM with only binary executables, not source code. SRPM contains one additional file,
`getty.1`, which is an older build of `getty`.

This makes it not only hard to update the tools but on top of that, they come from vastly different sources. Most of them
were taken from [KGMD 1.0](archives/basic.tar.gz) (Knarf's German MiNT Distribution by Frank Bartels, July 1995), which
ships both the binaries and their sources.

| (S)RPM file | Taken from | Source available in |
|---|---|---|
| /sbin/init | [KGMD](archives/basic.tar.gz): boot/multitos/init.prg (1) | [KGMD](archives/basic.tar.gz): usr/src/init/mintos-1.4.1.tar.gz (2) |
| /usr/sbin/getty | [KGMD](archives/basic.tar.gz): usr/etc/getty | [KGMD](archives/basic.tar.gz): usr/src/init/mintos-1.4.1.tar.gz (2) |
| oldstuff-1.0/getty.1 (SRPM only) | [MiNTOS 1.4.1](https://github.com/freemint/oldstuff/releases/download/mintos-1.4.1/mnts141b.tgz): usr/sbin/getty | [MiNTOS 1.4.1](https://github.com/freemint/oldstuff/releases/download/mintos-1.4.1/mnts141s.tgz) |
| /usr/bin/last | [MiNTOS 1.4.1](https://github.com/freemint/oldstuff/releases/download/mintos-1.4.1/mnts141b.tgz): usr/ucb/last | [MiNTOS 1.4.1](https://github.com/freemint/oldstuff/releases/download/mintos-1.4.1/mnts141s.tgz) |
| /sbin/reboot | [KGMD](archives/basic.tar.gz): usr/etc/reboot | [KGMD](archives/basic.tar.gz): usr/src/halt+reboot/ (3) |
| /sbin/execgem | [KGMD](archives/basic.tar.gz): usr/etc/execgem | [KGMD](archives/basic.tar.gz): usr/src/ttyvdev/ (4) |
| /sbin/execmtos | [KGMD](archives/basic.tar.gz): usr/etc/execmtos | [KGMD](archives/basic.tar.gz): usr/src/ttyvdev/ (4) |
| /sbin/shutdown | [shutdown 0.5](archives/shutdown.tar.gz) (5) | [shutdown 0.5](archives/shutdown.tar.gz) (5) |
| /etc/gettytab | [KGMD](archives/basic.tar.gz): etc/gettytab (6) | – |
| /etc/ttytab | [KGMD](archives/basic.tar.gz): etc/ttytab.con (7) | – |

All binaries are byte-identical to the file they were taken from, except `init` (1). The text files `gettytab` and `ttytab` were modified (6, 7).

1. Only the version string differs: "(KGMD 1.0-RELEASE) " was removed from it.
2. [MiNTOS 1.4.1](https://github.com/freemint/oldstuff/releases/tag/mintos-1.4.1) modified by Oliver Sturm and Kay Roemer:
   `init` uses `setsid()`, sets `TERM` from `ttytab` and supports `mgetty`; `getty` adds 38400 bps.
3. `halt.c` built with `-DREBOOT`. It is Ulrich Kuehn's `reboot.c`, first posted to the
   [MiNT mailing list](https://mikro.naprvyraz.sk/mint/199411/msg00054.html) in November 1994.
4. [ttyvdev 0.8](https://github.com/freemint/oldstuff/releases/tag/ttyvdev-0.8) + Frank Bartels' Makefile changes
   (`ttyvdev-0.7.diffs`, `ttyvdev-0.8.diffs`), which make `execmtos` start `/usr/multitos/gem.sys`.
5. By Draco. Available from [YesCREW's MiNT page](https://yescrew.atari.org/eng/mint.htm).
6. MiNTOS 1.4.1 `etc/gettytab` as modified by KGMD, with the login banner changed to "FreeMiNT".
7. Rewritten from KGMD's `ttytab.con`: German comments kept, `ttyS*` device names, N.AES/XaAES entries added.
