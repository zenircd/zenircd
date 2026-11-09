# Updating a WebIRC user's IP without reconnecting

ZenIRCd extension. Stock UnrealIRCd does not have this.

## The problem

A WebIRC gateway sends the user's real IP once, with `WEBIRC`, before
registration. Gateways that keep IRC sessions open while the browser is closed
(bouncer-style) can find that the user comes back days later from a different
IP. Without this extension the network keeps seeing the old IP until the IRC
connection is re-established, which means a visible quit and rejoin.

## Configuration

Opt in per gateway with `allow-ip-update` in a named `proxy` block of type
`webirc`:

```
proxy mygateway {
	type webirc;
	match { ip 192.0.2.1; }
	password "secret";
	allow-ip-update yes;
}
```

The default is `no`. The option is rejected for other proxy types and for
unnamed legacy `webirc { }` blocks.

## Protocol

The gateway sends `WEBIRC` again on the already registered connection, with the
same syntax as during registration:

```
WEBIRC <password> <gateway> <hostname> <ip>
```

Any options after `<ip>` are ignored in this case. The password is checked
against the proxy block the connection originally came in through.

Replies:

| Reply | Meaning |
|-------|---------|
| `NOTE WEBIRC IP_UPDATED <ip> <host> :...` | Done |
| `NOTE WEBIRC IP_UNCHANGED <ip> :...` | Same IP and host, nothing done |
| `FAIL WEBIRC NOT_WEBIRC :...` | Connection did not come in through a named WebIRC proxy block |
| `FAIL WEBIRC IP_UPDATE_DISABLED :...` | `allow-ip-update` is not enabled for that block |
| `FAIL WEBIRC INVALID_PASSWORD :...` | Wrong password (also logged as `WEBIRC_IP_UPDATE_BAD_PASSWORD`) |
| `FAIL WEBIRC INVALID_IP <ip> :...` | Not a valid IP address |

Servers without this extension answer `462 ERR_ALREADYREGISTRED`, so a gateway
can send the update unconditionally and fall back to "reconnect to update" when
it does not get `IP_UPDATED`.

## What happens on the server

The update is treated much like a reconnect from the new address, but the user
keeps their channels:

- The IP-change hooks run, the same ones that run for WEBIRC during
  registration: Z-Lines, DNSBL (blacklist), connect-flood, GeoIP, reputation and
  known-users. Any of them can disconnect the user.
- K-Lines and G-Lines are rechecked against the new IP and host, and shuns are
  re-evaluated.
- The real host, the cloaked host and, unless the user has a vhost (`+t`), the
  displayed host are updated. Users in common channels see this as `CHGHOST`,
  or as a part/rejoin if `set::allow-userhost-change force-rejoin` is set.
- The change is logged as `WEBIRC_IP_UPDATE`.

Not re-evaluated: allow { } block, connection class and `maxperip`. These only
apply at connect time.

## Server-to-server

Other servers are told with:

```
:<sid> CHGIP <uid> <base64 ip> <realhost>
```

`<sid>` is the user's server. The message is server-sourced on purpose: the
command parser looks commands up by the type of the source, so a user-sourced
server-only command would be dropped. Each server recalculates the cloaked host
itself, as it does for `UID`. The updated GeoIP moddata is broadcast with
`MD`.

Stock UnrealIRCd servers ignore `CHGIP`, because unknown commands from servers
are dropped without an error. They keep the old IP and host for that user, and
so do ZenIRCd servers that are only reachable through them. Mixed networks
therefore show the new IP only on ZenIRCd servers that have a path of ZenIRCd
servers to the user's server.
