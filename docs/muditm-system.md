# MUDitM System

TLS termination proxy for MUD servers. Fork-per-connection model with MCCP2 compression, MNES variable injection, and auto-detect TLS. Forked from [RahjIII/MUDitM](https://github.com/RahjIII/MUDitM) (v1.0, Feb 2024).

## Contents

- [Process Model](#process-model)
- [Signal Handling](#signal-handling)
- [TLS](#tls)
- [MCCP2 Compression](#mccp2-compression)
- [MNES / NEW-ENVIRON](#mnes--new-environ)
- [Proxy Loop](#proxy-loop)
- [Connection Logging](#connection-logging)
- [Configuration](#configuration)
- [Certificate Expiry](#certificate-expiry)
- [SKMUD Extensions](#skmud-extensions)
- [DoS Protection](#dos-protection)
- [Crash Diagnostics](#crash-diagnostics)

## Process Model

Fork-per-connection. The parent process creates a dual-stack IPv4/IPv6 listening socket (`new_mommie()`) with `IPV6_V6ONLY=0` so one socket handles both address families. `SO_REUSEADDR` and `SO_LINGER(off)` are set.

The `demonize()` function enters an infinite accept loop. When `demon=true` (the default), it forks for each accepted connection:

**Parent**: increments `child_count`, closes the client socket, continues the loop. SIGCHLD is blocked across the fork/increment with `sigprocmask` to prevent a race where a child exits between `fork()` and `child_count++`.

**Child**: restores the signal mask, closes the listening socket, breaks out of the loop. Then:

1. Ignores SIGPIPE
2. Auto-detects TLS (peeks first byte) or forces TLS
3. Loads SSL context post-fork (so cert updates take effect per-connection)
4. Configures MCCP2 compression on the client side
5. Connects to the game server
6. Optionally sends a stunnel PROXY v1 header
7. Configures MCCP2 compression on the game side
8. Enters `muditm_proxy()` -- the main data relay loop
9. On exit: logs I/O stats, frees endpoints, SSL context, config strings, GKeyFile, logs "Shutdown complete."

When `demon=false` (debug mode, `-d` flag), the parent handles one connection directly without forking.

**Cleanup paths**: two `goto` labels (`cleanup_game`, `cleanup_client`) handle early exits. If the game connection fails, an error message is sent to the client before cleanup. All `g_malloc`'d config strings are freed with `g_free()` and the GKeyFile is released with `g_key_file_free()`.

**Connection close**: when `read_endpoint()` returns 0, the proxy logs which side hung up and returns. `bytes_recv == -1` with `errno != EAGAIN/EWOULDBLOCK` also triggers exit.

## Signal Handling

| Signal | Handler | Scope | Behavior |
|--------|---------|-------|----------|
| SIGCHLD | `zombie_killer()` | Parent only | `SA_RESTART` flag. Reaps all zombies via `waitpid(-1, WNOHANG)` loop, decrements `child_count`. Saves/restores errno. |
| SIGTERM, SIGINT | `shutdown_handler()` | Both (inherited) | No `SA_RESTART` -- `accept()` returns EINTR. Sets `shutdown_requested` flag. Loop checks at top of each iteration, logs "Received signal N, shutting down." and exits cleanly through the normal cleanup path. |
| SIGSEGV, SIGBUS, SIGABRT | `fatal_handler()` | Both (inherited) | Async-signal-safe message to stderr with PID and parent/child role. Backtrace on glibc/macOS (guarded by `alarm(5)` against malloc deadlock). Re-raises with `SIG_DFL` for core dump. See [Crash Diagnostics](#crash-diagnostics). |
| SIGPIPE | `SIG_IGN` | Child only | Set immediately after fork, before any I/O. Write returns EPIPE instead of killing the process. |

The fatal handler determines parent vs child by comparing `getpid()` to `parent_pid` (stored in `main()` before signal installation). Children inherit all handlers across `fork()`.

## TLS

**Auto-detection** (`security = auto`): the child peeks the first byte with `recv(MSG_PEEK)` after a 250ms `poll()` timeout. Byte `0x16` (TLS ClientHello record type) triggers SSL handshake; any other byte proceeds as plaintext. This allows a single port to serve both TLS and plaintext clients.

**Post-fork cert loading**: the `SSL_CTX` is created and configured after the fork. Each child reads the cert/key files from disk, so certificate renewal (Let's Encrypt) takes effect on the next connection without restarting the parent.

**SSL context setup** (`configure_context()`):
- `SSL_CTX_set_ecdh_auto(ctx, 1)` -- automatic ECDH curve selection
- `SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)` -- rejects TLS 1.0/1.1
- Certificate file, private key file, and optional chain certificate file loaded
- `SSL_CTX_set_default_verify_paths()` for CA trust store

**Game-side TLS**: when `[game] security = SSL`, the game connection also uses TLS with `SSL_connect()`.

## MCCP2 Compression

MUDitM supports MCCP2 (Mud Client Compression Protocol version 2, telopt 86) on both the client side (as a compression server) and the game side (as a compression client). It decompresses game-to-client data and optionally re-compresses client-to-game data.

### Negotiation State Machine

```
NONE -> OFFERED -> ACCEPTED (compression active)
                -> REFUSED  (no compression)
```

States: `MCCP_NEGO_NONE` (initial), `MCCP_NEGO_OFFERED` (WILL sent), `MCCP_NEGO_ACCEPTED` (client sent DO), `MCCP_NEGO_REFUSED` (client sent DONT).

### Compression Modes

| Mode | Client-side | Game-side |
|------|-------------|-----------|
| `enable` | Offer MCCP2, compress if accepted | Accept game's MCCP2, decompress |
| `disable` | Refuse all MCCP offers (WONT) | Refuse all game offers (DONT) |
| `ignore` | Forward negotiations, disable matching when compression starts | Same |

### Client-side (MUDitM as server)

- `offer_compression()`: sends `IAC WILL MCCP2` to client, sets state to OFFERED
- `mccp2_do()`: client agrees. Guards against duplicate DO. Initializes deflate stream with `Z_BEST_COMPRESSION`, sends `IAC SB MCCP2 IAC SE` marker, sets state to ACCEPTED. Sends updated MNES `COMPRESSION=MCCP2` to game.
- `mccp2_dont()`: client refuses. Frees the z_stream, sets state to REFUSED. Sends MNES `COMPRESSION=none`.

### Game-side (MUDitM as client)

- On game's `IAC WILL MCCP2`: responds `IAC DO MCCP2`
- `mccp2_sb_start()`: on game's `IAC SB MCCP2 IAC SE`, initializes inflate stream. All subsequent game data is decompressed.

### MNES Interaction

MCCP negotiation state drives the `COMPRESSION` MNES variable. If negotiation completes after the initial MNES exchange, proactive updates correct the reported value.

## MNES / NEW-ENVIRON

MUDitM injects proxy metadata into the telnet NEW-ENVIRON (MNES) exchange between client and game server.

### Variables Injected

| Variable | Value | Purpose |
|----------|-------|---------|
| `PROXY_NAME` | `"MUDitM-1.0"` | Proxy identification |
| `SECURITY` | TLS version string or `"plaintext"` | Connection encryption status |
| `COMPRESSION` | `"MCCP2"` or `"none"` | Compression negotiation result |
| `CLIENTPORT` | Client's TCP source port | NAT correlation |
| `IPADDRESS` (configurable) | Client's IP address | Display/logging (untrusted) |
| `TRUSTED_IPADDRESS` | Client's IP address (hardcoded) | Security decisions (proxy-guarded) |

The `IPADDRESS` variable name is configurable via `[muditm] newenv_ipaddress` (semicolon-separated list). `TRUSTED_IPADDRESS` is always sent regardless of config.

### Proactive Injection

When `newenv_immediate_ip = true`, MUDitM sends the client's IP and port via unsolicited MNES IS subnegotiation to the game server immediately at connection time, before any telnet negotiation. This ensures the game has the real IP for connection-time decisions (bans, session logging).

### Silent Client Fallback

When `newenv_fallback = true` and a client ignores `DO NEW-ENVIRON` (sends neither WILL nor WONT), MUDitM responds WILL on behalf of the silent client after `newenv_fallback_ms` (default 2000ms). This triggers the normal MNES variable injection for clients that don't support the protocol.

### Client WONT Interception

When a client sends `IAC WONT NEW-ENVIRON`, MUDitM replaces it with `IAC WILL NEW-ENVIRON` before forwarding to the game. This lets MUDitM answer SEND requests on behalf of non-MNES clients. The SEND request is consumed (not forwarded to the client) since the client wouldn't understand it.

### IPv4-Mapped Address Handling

When a client connects via IPv4 to the dual-stack socket, the address appears as `::ffff:192.168.1.1`. The `::ffff:` prefix is stripped in log output and MNES variables to show the plain IPv4 address.

## Proxy Loop

`muditm_proxy()` is the main data relay loop between client and game endpoints.

**Setup**: registers telnet pattern handlers for both directions, resets I/O stats, offers MCCP2, optionally sends proactive IP, sets both sockets to non-blocking, creates a two-entry poll structure.

**Main loop**:
1. Check MNES fallback timeout (if configured)
2. `poll()` both sockets with 1000ms timeout
3. For each ready socket: read into the endpoint's input buffer
4. Run PCRE2 pattern matching on the input

**Pattern matching**: each endpoint has a compiled PCRE2 regex combining all its patterns via `|` alternation. Uses `PCRE2_PARTIAL_HARD` for partial matching across buffer boundaries. On a match, the handler is called with `(iobuf, match_length, from_endpoint, to_endpoint, config)`. Handler return: 1 = consumed the match bytes; 0 = forward them. Unmatched data is forwarded to the other side.

All IAC sequences are intercepted via binary pattern matching -- raw byte arrays embedded in the PCRE2 alternation.

## Connection Logging

**Format**: `asctime [PID] message`

Example:
```
Thu Aug 13 18:52:42 2026 [570329] Starting MUDitM-1.0
Thu Aug 13 18:52:42 2026 [570329] Accepting Client Connections.
Thu Aug 13 18:52:53 2026 [571335] Connect from ::ffff:192.168.1.5 port 50804
Thu Aug 13 18:52:53 2026 [571335] Client SSL accepted on socket 5
Thu Aug 13 18:52:53 2026 [571335] Sent IPADDRESS '192.168.1.5' to Game
Thu Aug 13 18:52:58 2026 [571335] Game has closed the connection.
Thu Aug 13 18:52:58 2026 [571335] Client sock 243 B in, 3.23 KB out, 5.52 sec
Thu Aug 13 18:52:58 2026 [571335] Shutdown complete.
```

The PID identifies the process: the parent logs startup/capacity messages, children log their session lifecycle. Parent PID appears at "Starting MUDitM" and never appears in "Connect from" lines.

**Log destination**: configured via `[muditm] log-file`. If set, opened with `fopen(path, "a")` (append mode). If unset, defaults to stderr. Each write is followed by `fflush()`.

**Buffer size**: 8192 bytes (`LOG_BUF_LEN`).

**Debug mode**: `muditm_debug()` calls `muditm_log()` only when `-d` flag is set.

## Configuration

GLib GKeyFile format (INI-style). Default path: `/etc/muditm.conf`, overridden with `-c`.

### All Config Keys

| Section | Key | Type | Default | Description |
|---------|-----|------|---------|-------------|
| `[muditm]` | `demon` | bool | true | Fork-per-connection mode |
| `[muditm]` | `listen` | int | 4143 | Listening port |
| `[muditm]` | `listen-backlog` | int | 16 | Kernel listen queue depth |
| `[muditm]` | `max-children` | int | 0 | Max concurrent children (0=unlimited) |
| `[muditm]` | `log-file` | string | (stderr) | Log file path |
| `[muditm]` | `stunnelproxy` | bool | false | Send stunnel PROXY v1 header |
| `[muditm]` | `newenv_ipaddress` | string list | `IPADDRESS` | MNES variable names for client IP (`;` separated) |
| `[muditm]` | `newenv_immediate_ip` | bool | false | Send IP before negotiation |
| `[muditm]` | `newenv_fallback` | bool | false | Respond WILL for silent clients |
| `[muditm]` | `newenv_fallback_ms` | int | 2000 | Fallback timeout in ms |
| `[ssl]` | `cert` | string | `cert.pem` | Certificate file |
| `[ssl]` | `key` | string | `key.pem` | Private key file |
| `[ssl]` | `chain` | string | (empty) | Chain certificate (optional) |
| `[game]` | `host` | string | `::` | Game server address |
| `[game]` | `service` | string | `4000` | Game server port |
| `[game]` | `security` | string | `none` | Game TLS: `SSL` or `none` |
| `[game]` | `compression` | string | `enable` | Game MCCP: `ignore`/`disable`/`enable` |
| `[client]` | `security` | string | `none` | Client TLS: `SSL`/`auto`/`none` |
| `[client]` | `compression` | string | `enable` | Client MCCP: `ignore`/`disable`/`enable` |
| `[skmud]` | `control_socket` | string | (none) | Unix socket for SKMUD notifications |

### Per-Environment Configs

| File | Env | Listen | Game | max-children | Security |
|------|-----|--------|------|-------------|----------|
| `muditm-dev.conf` | macOS dev | 2026 | 127.0.0.1:2027 | 0 (unlimited) | auto |
| `muditm-test.conf` | Docker test | 2026 | 127.0.0.1:2027 | 100 | auto |
| `muditm-ci.conf` | CI pipeline | 1996 | 127.0.0.1:1997 | 100 | auto |
| `muditm-prod.conf` | Production | 1996 | 127.0.0.1:1997 | 900 | auto |

Port architecture: MUDitM=N (front door), MUD=N+1 (internal). All SKMUD configs enable `newenv_immediate_ip`, `newenv_fallback`, and `control_socket`.

### Config Parsing

Three helpers simplify GKeyFile access:
- `get_conf_string(gkf, group, key, default)` -- returns `g_strdup(default)` if key missing
- `get_conf_int(gkf, group, key, default)` -- returns default on GError, frees the error
- `get_conf_boolean(gkf, group, key, default)` -- same pattern

All config values are read once at startup and used for the process lifetime (parent and children via fork).

## Certificate Expiry

Two checks ensure TLS certificates are monitored:

**Startup check** (`check_cert_expiry()`): runs pre-fork, pre-listen, only when TLS is needed. Reads the X509 cert and computes days remaining via `ASN1_TIME_diff`. Expired certs cause immediate exit with an error notification. Certs expiring within 30 days produce a warning.

**Per-connection throttled check** (`check_cert_expiry_throttled()`): runs post-fork in each child, with file-lock-based throttling to avoid flooding. Uses `flock(LOCK_EX|LOCK_NB)` on a `.cert-expiry-warned` file. Notification interval: 1 hour when <7 days remaining, 1 day otherwise. Multiple children checking simultaneously are serialized by the lock.

Both checks use the `muditm_notify` callback, which defaults to `muditm_log()` but is overridden by SKMUD to send alerts via the control socket.

## SKMUD Extensions

The `skmud/` subdirectory contains SK-specific code separated from upstream.

**`skmud_init(gkf)`**: called from `main()` before cert check. Reads `[skmud] control_socket` from config. If set, replaces the `muditm_notify` function pointer with `skmud_notify` and logs "SKMUD notifications enabled via <path>".

**`skmud_notify(msg)`**: logs the message via `muditm_log()` (always), then sends it to the SKMUD admin control socket as `notify log <msg>\n` via a fire-and-forget Unix domain socket connection. Connection failures are logged and swallowed.

## DoS Protection

**max-children**: configurable cap on concurrent forked children. When at capacity, the parent stops calling `accept()` and enters a 100ms poll spin. New connections queue in the kernel's listen backlog instead of forking unboundedly. Capacity transitions are logged once ("At max children", "Below max children").

**listen-backlog**: controls the kernel accept queue depth (default 16). Connections beyond the backlog while at max-children are refused by the kernel.

**Environment limits**: production allows 900 children (below the MUD's ~1020 fd ceiling), test/CI allow 100 (covers the 27-connection test harness with headroom), dev is unlimited.

## Crash Diagnostics

The fatal signal handler writes to stderr (not to the log file, which uses `fprintf`):

```
FATAL [570329] parent SIGABRT
/usr/lib/x86_64-linux-gnu/libasan.so.8(+0x923d9) [0x7553c56883d9]
/home/mud/SKMUD/src/externals/MUDitM/muditm(+0x9dc7) [0x597c4b856dc7]
...
```

The message is constructed using only async-signal-safe functions (`write()`, `getpid()`). The backtrace is available on glibc and macOS (`#ifdef HAVE_BACKTRACE`). An `alarm(5)` fires before the backtrace call to prevent deadlock if the crash occurred inside malloc.

After logging, the handler resets to `SIG_DFL` and re-raises the signal, producing a core dump (if enabled) and the correct exit status. Under AddressSanitizer, the re-raise also triggers the sanitizer's own report to the `log_path` file.

**SKMUD integration**: the parent project's `src/startup` exports `ASAN_OPTIONS` and `TSAN_OPTIONS` with per-binary `log_path` prefixes (`asan-merc` vs `asan-muditm`). `scripts/start-muditm.sh` overrides the sanitizer options to use the `muditm` prefix and redirects stderr to `log/muditm-<boot_index>.err`, giving the fatal handler a durable destination. Without this, `nohup ... 2>&1` would discard stderr.

**Distinguishing crashes from clean stops**: before the signal handlers were added, a SIGTERM kill and a SIGSEGV crash were indistinguishable -- both just stopped the process with no log entry. Now a clean stop logs "Received signal 15, shutting down." to the connection log, and a crash writes the FATAL line to the err file.
