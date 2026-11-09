# Security Policy

## Supported versions

Only the latest ZenIRCd release (and the `main` branch) receives security fixes.
ZenIRCd is based on UnrealIRCd 6. Security fixes released by UnrealIRCd are
reviewed and applied to ZenIRCd as quickly as possible.

## Reporting a vulnerability

**Do not report security issues in public issues, pull requests or IRC channels.**

Use GitHub's private vulnerability reporting for this repository:
<https://github.com/zenircd/zenircd/security/advisories/new>
(on the repository page: *Security* → *Report a vulnerability*).

Please include:
- the ZenIRCd version (`./zenircd version`) and, if relevant, your configuration (redact passwords and keys)
- a reproducer, for example the IRC traffic that triggers the issue
- for memory-safety issues: AddressSanitizer output. Answer *Yes* to the
  AddressSanitizer question in `./Config` to build with it.

If you use automated tools (fuzzers, static analysers, AI), please confirm the
issue on a running server before reporting it.

## Issues that also affect UnrealIRCd

ZenIRCd shares most of its code with UnrealIRCd. If the issue can be reproduced on
stock UnrealIRCd, please **also** report it to the UnrealIRCd team following
[their security policy](https://github.com/unrealircd/unrealircd/blob/unreal60_dev/SECURITY.md),
so all UnrealIRCd-based servers get fixed. We coordinate disclosure with upstream
and do not publish details of an upstream issue before upstream has released a fix.

## Scope

In general in scope: issues triggerable by regular users or unauthenticated
clients involving memory safety (out-of-bounds access, use-after-free), information
disclosure (for example revealing a cloaked user's IP address), privilege escalation,
denial of service (crash, hang, resource exhaustion) or remote code execution.
This includes the WebIRC/proxy trust model.

Issues that require IRCOp rights, server-to-server access or editing configuration
files are assessed case by case. Issues that require shell access as the user
running ZenIRCd are generally not security issues.
