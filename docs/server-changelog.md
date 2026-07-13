# Server Changelog

Completed runtime behavior changes. Append-only history.

## Entry Format

```
- [x] `VERSION` `YYYY-MM` Brief description — details. **Files:** `file.c`
```

Within each category, items are grouped by version (newest first), sorted by date within a version (newest first).

---

## Contents

- [Protocol](#protocol)
- [Security](#security)
- [TLS](#tls)

## Protocol

### 5.11.0
- [x] `5.11.0` `2026-06` MNES variable injection — sends SECURITY (TLS version or "plaintext"), COMPRESSION (MCCP2 or none), and TRUSTED_IPADDRESS (kernel-verified client IP via `getpeername`) alongside existing PROXY_NAME and IPADDRESS. Game server uses TRUSTED_IPADDRESS for locked security decisions, standard IPADDRESS for display. **Files:** `handlers.c`
- [x] `5.11.0` `2026-05-31` **BUG** Fix respond_wont sending DONT instead of WONT — respond_wont() sent IAC DONT instead of IAC WONT in response to IAC DO, violating RFC 854. Debug log already printed "WONT", masking the bug on the wire. Verified with raw byte capture before and after fix. **Files:** `handlers.c`

## Security

### 5.11.0
- [x] `5.11.0` `2026-06` TLS 1.2 minimum version — `SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)` after `SSL_CTX_new`. Rejects TLS 1.0/1.1 at handshake. Belt-and-suspenders on OpenSSL 3.x which already defaults to 1.2+. **Files:** `muditm.c`
- [x] `5.11.0` `2026-05-31` Format security hardening — added -Wformat-security to CFLAGS. Fixed respond_X parameter from char to int to prevent implicit truncation of telnet constants (DONT=254, DO=253). **Files:** `makefile`, `handlers.c`
- [x] `5.11.0` `2026-05-31` Ignore SIGPIPE to prevent proxy crash on client disconnect — default SIGPIPE disposition kills the child process when a client disconnects mid-write. SIG_IGN causes write() to return EPIPE instead. Prerequisite for fixing the short write issue noted in TODO. **Files:** `muditm.c`

## TLS

### 5.11.0
- [x] `5.11.0` `2026-05-31` SKMUD admin socket notification — overrides muditm_notify to send cert alerts via Unix domain socket to SKMUD control socket. Protocol: `notify log <message>`. Config: `[skmud] control_socket = <path>`. **Files:** `skmud/skmud.c`, `skmud/skmud.h`, `muditm.c`
- [x] `5.11.0` `2026-05-31` TLS auto-detection on single port — peeks first byte with 250ms poll timeout. 0x16 (TLS ClientHello) triggers SSL handshake, any other byte proceeds as plaintext. Config: `security = auto`. Tested with Mudlet, TinTin++, gnutls-cli. **Files:** `muditm.c`
- [x] `5.11.0` `2026-05-31` Certificate expiry check at startup and per-connection — startup check warns for certs expiring within 30 days, exits on expired cert. Per-fork throttled check uses flock-based .cert-expiry-warned file to prevent duplicate alerts (hourly <7 days, daily otherwise). Pluggable notification callback (muditm_notify function pointer) for routing alerts to external systems. **Files:** `certcheck.c`, `certcheck.h`, `muditm.c`, `muditm.h`
