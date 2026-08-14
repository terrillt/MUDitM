# Pull Requests

Tracking upstream PR submissions to [RahjIII/MUDitM](https://github.com/RahjIII/MUDitM).

Numbering is `PR-MUDITM-NNN`, chronological by commit order. One PR
per commit. PR-worthy commits are identified by upstream-style commit
messages and by touching only upstream code (no `skmud/`, no SK
config files).

## Status Legend

- **Pending** -- on `dev`/`master`, needs cherry-pick to `main`
- **Ready** -- on `main`, PR not yet submitted
- **Submitted** -- PR open on upstream
- **Merged** -- accepted upstream
- **Declined** -- rejected or withdrawn

---

## PR-MUDITM-001: macOS build portability

- **Status**: Pending
- **Commit**: `70de79c`
- **Description**: `stdlib.h` instead of `malloc.h`, pkg-config for
  OpenSSL/PCRE2, `cc` default compiler, ctags out of default target,
  `rm -rf` instead of `rm -rI`.

## PR-MUDITM-002: SIGPIPE crash fix

- **Status**: Pending
- **Commit**: `a6fd297`
- **Description**: Ignore SIGPIPE to prevent child crash on client
  disconnect mid-write.

## PR-MUDITM-003: Format security hardening

- **Status**: Pending
- **Commit**: `b14ba29`
- **Description**: `-Wformat-security` flag, implicit conversion
  fix for telnet constants (char to int).

## PR-MUDITM-004: respond_wont RFC 854 fix

- **Status**: Pending
- **Commit**: `b902624`
- **Description**: `respond_wont()` was sending DONT instead of WONT.

## PR-MUDITM-005: Certificate expiry checking

- **Status**: Pending
- **Commit**: `0f8eba3`
- **Description**: Startup check + per-connection throttled check
  with pluggable notification callback. Exits on expired cert,
  warns within 30 days, flock-based throttle prevents flood.

## PR-MUDITM-006: DoS hardening

- **Status**: Pending
- **Commit**: `81bd758`
- **Description**: `max-children` fork cap, `listen-backlog` config,
  capacity transition logging, EINTR handling in accept loop.
  Includes `test_max_children` test binary.

## PR-MUDITM-007: MNES COMPRESSION state fix

- **Status**: Pending
- **Commit**: `fd6eb94`
- **Description**: Report actual negotiated MCCP2 state instead of
  config intent. `mccp_nego` state machine with proactive MNES
  updates on late negotiation.

## PR-MUDITM-008: MCCP2 duplicate DO fix

- **Status**: Pending
- **Commit**: `f2d0507`
- **Description**: Guard against double compression init on repeated
  `DO COMPRESS2`. Duplicate logged and ignored.

## PR-MUDITM-009: Client IP in negotiation log lines

- **Status**: Pending
- **Commit**: `5dbc090`
- **Description**: All MNES and MCCP2 log messages include client IP
  via `addr_endpoint()`. Client source port in Connect log line.

## PR-MUDITM-010: Client port in MNES log

- **Status**: Pending
- **Commit**: `a9bfc43`
- **Description**: Add client TCP source port to MNES negotiation
  log line for connection correlation.

## PR-MUDITM-011: Free config resources at exit

- **Status**: Pending
- **Commit**: `19a772f`
- **Description**: `g_strdup` instead of `strdup` in
  `get_conf_string`, `g_error_free` in `get_conf_int`/
  `get_conf_boolean`, free all config strings and GKeyFile at exit.

## PR-MUDITM-012: Signal handlers for shutdown and crash diagnosis

- **Status**: Pending
- **Commit**: `9366789`
- **Description**: SIGTERM/SIGINT clean shutdown. SIGSEGV/SIGBUS/
  SIGABRT crash with PID, parent/child label, backtrace, alarm guard.

## PR-MUDITM-013: Uninitialized return value in close_endpoint

- **Status**: Pending
- **Commit**: `d3cdc9e`
- **Description**: `close_endpoint()` returned indeterminate value
  when socket was already negative. Initialize `ret` to 0.

---

## SKMUD-Only

Design decisions and deployment-specific work not appropriate for
upstream without the author's agreement on direction:

| Commits | Description |
|---------|-------------|
| `b089ee8` | TLS auto-detection (single-port via first-byte peek) |
| `89e9923` | MNES proxy variables (TRUSTED_IPADDRESS), TLS 1.2 minimum |
| `8947d33`, `707ea54` | Proactive IP injection, silent MNES fallback |
| `704eb14` | CLIENTPORT forwarding via MNES |
| `skmud/skmud.c` | Admin socket notification override |
| Config commits | Per-environment configs, cert paths, CI, fail2ban |
| `977b0c1`, `e69a88b` | Sanitizer/coverage build support, test binary target |
| Doc-only commits | Changelogs, CLAUDE.md, integration plan |
