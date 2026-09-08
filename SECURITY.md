# Security Policy

## Supported Versions
* The latest *stable* release of ZenIRCd

See GitHub Releases for supported versions and End Of Life information.

## Scope

In general, issues triggered by regular users involving memory safety issues
(such as OOB read/write or UAF), sensitive information disclosure, privilege elevation,
Denial of Service (e.g. a crash), or remote code execution fall within the scope of
this security policy.

Issues that require IRCOp rights, server-to-server traffic, or editing of config
files may still fall within scope, but are classified case by case depending on
the impact and circumstances.

Issues that require shell access as the same user running ZenIRCd are not
considered security issues.

## Use of AI or other tools

It is normal and acceptable to use tools for finding security vulnerabilities.
We use them ourselves as well: AI, static code analyzers, fuzzing. This is all fine.

If a tool flagged an issue then we ask only **one extra thing**: that you
**reproduce the issue** on your own local server. So: confirm the issue by
actually running ZenIRCd with a reproducer (which usually means: by sending
IRC traffic to trigger the bug). This is because tools regularly flag something
as an issue but in practice it may be impossible to happen because of some extra
check somewhere or other requirements.

If you are trying to reproduce an issue, then we suggest running `./Config` and
answering `Yes` to the near-last question about AddressSanitizer (ASan),
especially for memory safety issues. Please include the reproducer and any
relevant ASan output in the bug report. ASan output is useful even if a normal
build does not visibly crash. It helps us a lot.

If you used AI, static code analyzers, fuzzing, or similar tools and fail to
follow the procedure above, expect us to ask you again to reproduce the issue.
If you refuse to do so, don't respond in a timely manner, or keep sending reports
without doing so after we asked, then we will close the bug report.

## Reporting a Vulnerability

Please report security issues privately via GitHub Security Advisories on
https://github.com/zenircd/zenircd or by email to info@zenet.org.

Do not report security issues as a public Pull Request, public issue, or in a
public IRC channel such as #zen-support.

If you found a real issue but are unsure if it is a security issue, report it
privately anyway. Better safe than sorry.

You should get a response or at least an acknowledgement soon. If you don't hear
back within a few days, please try to contact us again.
