# ZenIRCd

ZenIRCd is an IRC server developed primarily for the **ZEnet IRC Network**.

It is **based on [UnrealIRCd](https://www.unrealircd.org) 6**, not a rewrite.
ZenIRCd keeps UnrealIRCd's mature code base, protocol and module ABI, and adds
its own identity, roadmap and release cycle. All credit for the underlying
server goes to the UnrealIRCd team and contributors (see [Attribution](#attribution)).

| | |
|---|---|
| Current version | **ZenIRCd 0.1.0** (pre-release) |
| Based on | UnrealIRCd **6.2.8-git** (upstream development branch `unreal60_dev`; includes all 6.2.7.1 security fixes, including CVE-2026-114713) |
| License | GNU GPL version 2 (see [LICENSE](LICENSE) and [License](#license)) |
| Status | Early development, see [docs/TODO.md](docs/TODO.md) |

## Compatibility with UnrealIRCd

ZenIRCd can be linked into an existing UnrealIRCd 6 network and works with the same
services (Anope, Atheme), clients and configuration. To keep that compatibility, the
following are deliberately **unchanged**:

- the server-to-server protocol, including the `UnrealIRCd-` software prefix in `PROTOCTL EAUTH`
- the module ABI (`unrealircd-6`), so third-party UnrealIRCd 6 modules still compile and load
- database file formats, configuration syntax, and `unrealircd.org/*` message tags and capabilities
- the UnrealIRCd version number used for config `@if` checks and module compatibility (`6.2.8-git`, the upstream code this tree contains)

What ZenIRCd changes:

| | UnrealIRCd | ZenIRCd |
|---|---|---|
| Version reply / banner | `UnrealIRCd-6.2.8-git` | `ZenIRCd-0.1.0` (based on UnrealIRCd 6.2.8-git) |
| Default install directory | `~/unrealircd` | `~/zenircd-server` |
| Control script / binaries | `unrealircd`, `bin/unrealircd`, `bin/unrealircdctl` | `zenircd`, `bin/zenircd`, `bin/zenircdctl` (old names kept as symlinks) |
| Main config file | `conf/unrealircd.conf` | `conf/zenircd.conf` (falls back to `conf/unrealircd.conf`) |
| PID / control socket | `data/unrealircd.pid`, `.ctl` | `data/zenircd.pid`, `.ctl` |
| `upgrade`, `hot-patch`, `cold-patch` | download from unrealircd.org | **disabled** (they would install UnrealIRCd code over ZenIRCd) |
| Crash reports | optional upload to the UnrealIRCd team | written locally, **never uploaded** |

When migrating an existing UnrealIRCd install, update scripts, cron jobs and systemd
units that refer to the PID file name or the old install path. Example units are in
[`extras/startup/`](extras/startup/).

## Building and installing

ZenIRCd must not be built or run as root.

```sh
git clone https://github.com/zenircd/zenircd.git
cd zenircd
./Config          # answer the questions (default install dir: ~/zenircd-server)
make
make install
cd ~/zenircd-server
./zenircd start
```

Requirements are the same as for UnrealIRCd 6: a C compiler, GNU make,
OpenSSL development headers and pkg-config. Bundled copies of argon2, c-ares,
libsodium, jansson and PCRE2 are used if the system ones are missing.
Only Linux is currently tested. The Windows build files are inherited from UnrealIRCd and untested.

## Configuration and documentation

Configuration is identical to UnrealIRCd 6, so the
[UnrealIRCd 6 documentation](https://www.unrealircd.org/docs/UnrealIRCd_6_documentation)
applies. Read `zenircd` wherever it says `unrealircd`.
Start from `doc/conf/examples/example.conf`.

ZenIRCd-specific documents:

- [docs/README.md](docs/README.md): project charter, priorities and ZEnet requirements
- [docs/TODO.md](docs/TODO.md): phased roadmap and task list
- [docs/AUDIT.md](docs/AUDIT.md): baseline record, technical and security audit

## Testing

The full upstream test suite (three linked servers with Anope and Atheme) runs in a
throwaway Docker container:

```sh
extras/build-tests/docker/run
```

Do **not** run the test framework directly on a development machine or server.
It kills every process of the current user whose name matches `ircd`, `services`
and similar, and it overwrites service configurations. The script tests all
git-tracked files in the working tree, including uncommitted changes.

## Reporting bugs and security issues

- Bugs: https://github.com/zenircd/zenircd/issues
- Security issues: **do not** open a public issue. See [SECURITY.md](SECURITY.md).

If you can reproduce a problem on stock UnrealIRCd too, please also report it
upstream, following the [UnrealIRCd security policy](https://github.com/unrealircd/unrealircd/blob/unreal60_dev/SECURITY.md)
for security issues.

## Attribution

ZenIRCd is built on UnrealIRCd, developed by Bram Matthys (Syzop),
Krzysztof Beresztant (k4be), Gottem, i and many other contributors
(see `/CREDITS` on a running server and [doc/Authors](doc/Authors)).
UnrealIRCd itself is based on the original irc2 code written by Jarkko Oikarinen;
the authors of that and other earlier code are credited in the source file headers.

All existing copyright notices are preserved. ZenIRCd does not claim to have been
written from scratch. If you can, consider
[supporting the UnrealIRCd project](https://www.unrealircd.org/index/donations).

## License

ZenIRCd is distributed under the GNU General Public License version 2 ([LICENSE](LICENSE)).
Many source files are licensed "version 1, or (at your option) any later version"
or "version 2 or later", while a few state GPLv2 only. The combined work should
therefore be treated as GPLv2. Details are in [docs/AUDIT.md](docs/AUDIT.md) Part E.
