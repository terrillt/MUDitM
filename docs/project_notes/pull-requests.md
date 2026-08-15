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

- **Status**: Ready
- **Commit**: `70de79c` (main: `4bac241`)
- **Description**: `stdlib.h` instead of `malloc.h`, pkg-config for
  OpenSSL/PCRE2, `cc` default compiler, ctags out of default target,
  `rm -rf` instead of `rm -rI`.

## PR-MUDITM-002: SIGPIPE crash fix

- **Status**: Ready
- **Commit**: `a6fd297` (main: `9cc74e5`)
- **Description**: Ignore SIGPIPE to prevent child crash on client
  disconnect mid-write.

## PR-MUDITM-003: Format security hardening

- **Status**: Ready
- **Commit**: `b14ba29` (main: `0442ed2`)
- **Description**: `-Wformat-security` flag, implicit conversion
  fix for telnet constants (char to int).

## PR-MUDITM-004: respond_wont RFC 854 fix

- **Status**: Ready
- **Commit**: `b902624` (main: `dd5063e`)
- **Description**: `respond_wont()` was sending DONT instead of WONT.

## PR-MUDITM-005: Certificate expiry checking

- **Status**: Ready
- **Commit**: `0f8eba3` (main: `e0c10bc`)
- **Description**: Startup check + per-connection throttled check
  with pluggable notification callback. Exits on expired cert,
  warns within 30 days, flock-based throttle prevents flood.

## PR-MUDITM-006: DoS hardening

- **Status**: Ready
- **Commit**: `81bd758` (main: `599b0d7`)
- **Description**: `max-children` fork cap, `listen-backlog` config,
  capacity transition logging, EINTR handling in accept loop.
  Includes `test_max_children` test binary.

## PR-MUDITM-007: Add .gitignore

- **Status**: Ready
- **Commit**: `d3e08e1` (main: `c4d9786`)
- **Description**: Ignore build outputs, ctags, TLS certs, cert-expiry
  throttle file, test binary, and macOS Finder metadata.

## PR-MUDITM-008: MNES COMPRESSION state fix

- **Status**: Ready
- **Commit**: `fd6eb94` (main: `b03169f`)
- **Description**: Report actual negotiated MCCP2 state instead of
  config intent. `mccp_nego` state machine with proactive MNES
  updates on late negotiation.

## PR-MUDITM-009: Silent MNES client fallback

- **Status**: Ready
- **Commit**: `707ea54` (main: `0e5d9c0`)
- **Description**: Handle clients that ignore DO NEW-ENVIRON by
  faking WILL after a configurable timeout. New `mnes_client_will`
  handler tracks late arrivals. Default off (`newenv_fallback`).

## PR-MUDITM-010: Proactive client IP injection

- **Status**: Ready
- **Commit**: `8947d33` (main: `20c99be`)
- **Description**: Send client IP to game server via unsolicited MNES
  IS IPADDRESS at connection time, before telnet negotiation completes.
  Default off (`newenv_immediate_ip`).

## PR-MUDITM-011: Client IP in negotiation log lines

- **Status**: Ready
- **Commit**: `5dbc090` (main: `a7317b9`)
- **Description**: All MNES and MCCP2 log messages include client IP
  via `addr_endpoint()`. Client source port in Connect log line.

## PR-MUDITM-012: MCCP2 duplicate DO fix

- **Status**: Ready
- **Commit**: `f2d0507` (main: `cfd444e`)
- **Description**: Guard against double compression init on repeated
  `DO COMPRESS2`. Duplicate logged and ignored.

## PR-MUDITM-013: Client port in MNES log

- **Status**: Ready
- **Commit**: `a9bfc43` (main: `5ca7715`)
- **Description**: Add client TCP source port to MNES negotiation
  log line for connection correlation.

## PR-MUDITM-014: Free config resources at exit

- **Status**: Ready
- **Commit**: `19a772f` (main: `46b475f`)
- **Description**: `g_strdup` instead of `strdup` in
  `get_conf_string`, `g_error_free` in `get_conf_int`/
  `get_conf_boolean`, free all config strings and GKeyFile at exit.

## PR-MUDITM-015: Signal handlers for shutdown and crash diagnosis

- **Status**: Ready
- **Commit**: `9366789` (main: `6153cd5`)
- **Description**: SIGTERM/SIGINT clean shutdown. SIGSEGV/SIGBUS/
  SIGABRT crash with PID, parent/child label, backtrace, alarm guard.

## PR-MUDITM-016: Uninitialized return value in close_endpoint

- **Status**: Ready
- **Commit**: `d3cdc9e` (main: `0e4db71`)
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
| `704eb14` | CLIENTPORT forwarding via MNES |
| `skmud/skmud.c` | Admin socket notification override |
| Config commits | Per-environment configs, cert paths, CI, fail2ban |
| `977b0c1`, `e69a88b` | Sanitizer/coverage build support, test binary target |
| Doc-only commits | Changelogs, CLAUDE.md, integration plan |
