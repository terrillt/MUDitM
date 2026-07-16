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

### 5.11.1
- [x] `5.11.1` `2026-07-15` Proactive IP injection (`newenv_immediate_ip`) — sends client IP to game server at connection time via unsolicited NEW-ENVIRON IS, before any telnet negotiation. Ensures game server has the real IP for connection-time bans, login notifications, and session logging. Config-gated, default off. **Files:** `proxy.c`, `muditm.conf`
- [x] `5.11.1` `2026-07-15` Silent MNES fallback (`newenv_fallback`) — when a client ignores DO NEW-ENVIRON (sends neither WILL nor WONT), MUDitM responds WILL on behalf of the silent client after a configurable timeout (`newenv_fallback_ms`, default 2000). Triggers the normal `mnes_request` path for proxy-side fields. New client-side WILL handler tracks late arrivals and corrects state. Config-gated, default off. **Files:** `proxy.c`, `handlers.c`, `handlers.h`, `muditm.h`, `muditm.conf`
- [x] `5.11.1` `2026-07-14` **BUG** MNES COMPRESSION reported config intent, not actual state — `mnes_request` checked `mccp_mode == MCCP_ENABLE` (the config setting) instead of whether MCCP2 was actually negotiated. All connections reported "MCCP2" even when the client ignored or refused the offer (e.g. raw telnet, terminal emulators). Fix: `mccp_nego` state machine (NONE → OFFERED → ACCEPTED/REFUSED) tracks the actual negotiation. `mnes_request` reports "MCCP2" if accepted, "none" otherwise (including while still negotiating). `mccp2_do` and `mccp2_dont` send proactive MNES updates to correct the initial report if negotiation completes after MNES fires. **Files:** `mccp.h`, `proxy.h`, `mccp.c`, `handlers.c`

### 5.11.0
- [x] `5.11.0` `2026-06-02` MNES variable injection — sends SECURITY (TLS version or "plaintext"), COMPRESSION (MCCP2 or none), and TRUSTED_IPADDRESS (kernel-verified client IP via `getpeername`) alongside existing PROXY_NAME and IPADDRESS. Game server uses TRUSTED_IPADDRESS for locked security decisions, standard IPADDRESS for display. **Files:** `handlers.c`
- [x] `5.11.0` `2026-05-31` **BUG** Fix respond_wont sending DONT instead of WONT — respond_wont() sent IAC DONT instead of IAC WONT in response to IAC DO, violating RFC 854. Debug log already printed "WONT", masking the bug on the wire. Verified with raw byte capture before and after fix. **Files:** `handlers.c`

## Security

### 5.11.0
- [x] `5.11.0` `2026-06-18` DoS hardening: max-children and listen-backlog — configurable `max-children` caps concurrent forked child processes (default 0 = unlimited). When at capacity, new connections wait in the kernel listen backlog instead of forking unboundedly. `listen-backlog` controls the kernel accept queue depth (default 16, was hardcoded 1). Child count tracked via global counter incremented on fork, decremented in SIGCHLD handler. Capacity transitions logged. EINTR handling added to accept loop. **Files:** `muditm.c`, `muditm.conf`
- [x] `5.11.0` `2026-06-02` TLS 1.2 minimum version — `SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)` after `SSL_CTX_new`. Rejects TLS 1.0/1.1 at handshake. Belt-and-suspenders on OpenSSL 3.x which already defaults to 1.2+. **Files:** `muditm.c`
- [x] `5.11.0` `2026-05-31` Format security hardening — added -Wformat-security to CFLAGS. Fixed respond_X parameter from char to int to prevent implicit truncation of telnet constants (DONT=254, DO=253). **Files:** `makefile`, `handlers.c`
- [x] `5.11.0` `2026-05-31` Ignore SIGPIPE to prevent proxy crash on client disconnect — default SIGPIPE disposition kills the child process when a client disconnects mid-write. SIG_IGN causes write() to return EPIPE instead. Prerequisite for fixing the short write issue noted in TODO. **Files:** `muditm.c`

## TLS

### 5.11.0
- [x] `5.11.0` `2026-06-18` `sun_path` truncation warning fix — `snprintf` with `%s` on a 256-byte source into 108-byte `sun_path` triggered `-Wformat-truncation`. Changed to `strncpy` with `sizeof - 1`. **Files:** `skmud/skmud.c`
- [x] `5.11.0` `2026-05-31` SKMUD admin socket notification — overrides muditm_notify to send cert alerts via Unix domain socket to SKMUD control socket. Protocol: `notify log <message>`. Config: `[skmud] control_socket = <path>`. **Files:** `skmud/skmud.c`, `skmud/skmud.h`, `muditm.c`
- [x] `5.11.0` `2026-05-31` TLS auto-detection on single port — peeks first byte with 250ms poll timeout. 0x16 (TLS ClientHello) triggers SSL handshake, any other byte proceeds as plaintext. Config: `security = auto`. Tested with Mudlet, TinTin++, gnutls-cli. **Files:** `muditm.c`
- [x] `5.11.0` `2026-05-31` Certificate expiry check at startup and per-connection — startup check warns for certs expiring within 30 days, exits on expired cert. Per-fork throttled check uses flock-based .cert-expiry-warned file to prevent duplicate alerts (hourly <7 days, daily otherwise). Pluggable notification callback (muditm_notify function pointer) for routing alerts to external systems. **Files:** `certcheck.c`, `certcheck.h`, `muditm.c`, `muditm.h`
