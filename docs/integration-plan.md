# MUDitM macOS Port — Full Plan

## Phase 1 — macOS Build Portability ✓ COMPLETE

Minimum changes to compile on macOS. See `docs/infra-changelog.md` for details.

## Phase 2 — Configure and Test MUDitM with SKMUD ✓ COMPLETE

### Context
Verify MUDitM works as a TLS proxy in front of SKMUD on the dev environment. MUDitM listens on port 2027 (TLS), forwards to game server on port 2026 (plaintext). During this phase only TLS connections work through MUDitM — plaintext clients connect directly to 2026 as before.

### Steps

**2a. Generate self-signed TLS certificate**
```bash
cd src/externals/MUDitM
openssl req -nodes -new -x509 -keyout key.pem -out cert.pem -days 1 -subj "/CN=localhost"
```
- `cert.pem` and `key.pem` in the MUDitM directory (gitignored)
- 1-day expiry to allow testing cert expiry detection (Phase 6)
- Regenerate with `-days 3650` once expiry testing is complete

**2b. Write dev config file**

Create `muditm-dev.conf`:
```ini
[muditm]
demon = true
listen = 2027
log-file =
newenv_ipaddress = IPADDRESS

[ssl]
cert = cert.pem
key = key.pem

[game]
host = 127.0.0.1
service = 2026
security = none
compression = enable

[client]
security = SSL
compression = enable
```

No `chain` line needed for self-signed certs. Log to stderr for dev visibility.

**2c. Start MUDitM**

Requires MUD server already running on port 2026.
```bash
# Daemon mode (fork per connection):
./muditm -c muditm-dev.conf

# Debug mode (foreground, single connection):
./muditm -d -c muditm-dev.conf
```

**2d. Test TLS connection**

Connect with a TLS-capable client:
```bash
openssl s_client -connect localhost:2027 -quiet
```
Should see the MUD greeting. Type commands, verify responses flow correctly.

Also test with Mudlet configured for TLS on localhost:2027.

### Verification
1. MUDitM starts without errors
2. `openssl s_client -connect localhost:2027` → TLS works locally, MUD greeting appears
3. Can log in and execute commands through the proxy
4. MNES IPADDRESS is received by the game server (check with `terminals <name>` — Trusted IP shows remote IP, not 127.0.0.1)
5. Ctrl+C MUDitM → client disconnects cleanly

### Files
- `muditm-dev.conf` (gitignored)
- `cert.pem` (gitignored)
- `key.pem` (gitignored)

**Note:** Phase 2 produces no committed code — all artifacts are gitignored local config/certs.

## Phase 3 — SIGPIPE Bug Fix ✓ COMPLETE (separate upstream PR)

- Add `signal(SIGPIPE, SIG_IGN)` in `muditm.c` after `demonize()` returns, before SSL setup
- Process-wide ignore — causes `write()` to return -1/EPIPE instead of crashing
- Real bug on both platforms

**Verification:** Start MUDitM, connect a client, kill the client mid-output (e.g., during a large room description). MUDitM parent process should remain alive and accept new connections.

## Phase 4 — Security Hardening ✓ COMPLETE (separate upstream PR)

- Add `-Wformat-security` to CFLAGS in makefile
- Fix `respond_X` parameter from `char` to `int` — telnet constants (DONT=254, DO=253) overflow signed char

**Verification:** `make clean && make` produces zero new warnings from the new flags.

## Phase 5 — WONT Protocol Bug Fix ✓ COMPLETE (separate upstream PR)

- Fix `respond_wont` sending DONT instead of WONT — RFC 854 violation
- The debug log already printed "WONT", masking the bug on the wire

**Verification:** Python script captures raw telnet bytes. Before: proxy responded with ff fe (IAC DONT). After: proxy responds with ff fc (IAC WONT).

## Phase 6 — Certificate Expiry Check ✓ COMPLETE (separate upstream PR)

### Startup check
- Read cert's `notAfter` field via `X509_get0_notAfter()` and `ASN1_TIME_diff()`
- If within 30 days of expiry: warn via `muditm_notify()`
- If already expired: error via `muditm_notify()` and exit

### Per-connection throttled check
- Each forked child checks cert after loading SSL context
- flock-based throttle file (`.cert-expiry-warned`) in same directory as cert
- Hourly alerts for certs expiring within 7 days, daily otherwise
- Guards against clock skew (negative time diff)

### Notification callback hook
- `notify_fn muditm_notify` function pointer in `muditm.h`
- Default: logs via `muditm_log()`
- Override to route alerts to external systems

### certcheck module
- `certcheck.c` / `certcheck.h` — isolated from `muditm.c`
- `cert_days_remaining()`, `cert_throttle_path()`, `cert_throttle_touch()`
- `check_cert_expiry()` (startup), `check_cert_expiry_throttled()` (per-fork)

**Verification:** Tested with 365-day cert (no warning), 1-day cert (warns at startup), and expired cert (exits with error).

## Phase 7 — SKMUD Admin Socket Notification ✓ COMPLETE (SKMUD-specific)

### skmud/ subdirectory
- `skmud/skmud.c` — overrides `muditm_notify` to send alerts via Unix domain socket
- `skmud/skmud.h` — declares `skmud_init(GKeyFile *gkf)`
- Protocol: `notify log <message>\n` to SKMUD admin control socket
- Config: `[skmud] control_socket = <path>`
- Fire-and-forget: if socket not available, log and continue

### Integration
- `skmud_init(gkf)` called at startup before cert check
- If `[skmud]` config section present, overrides notification delivery
- Without `[skmud]` section, default no-op behavior, no crash

**Verification:** Start MUDitM + SKMUD with `[skmud]` config section. Cert warning fires and admin socket notification appears in-game on `notify log` channel. Remove `[skmud]` section → default behavior, no crash.

## Phase 8 — TLS Auto-Detection ✓ COMPLETE (SKMUD-specific)

### Design
- Peek first byte with `poll()` + `recv(MSG_PEEK)` after `demonize()` returns
- 250ms poll timeout — fast enough to be imperceptible, long enough for slow connections
- `0x16` = TLS ClientHello → SSL handshake
- Any other byte → plaintext proxy

### Config
```ini
[client]
security = auto    # peek first byte: TLS if 0x16, plaintext otherwise
security = SSL     # require TLS (upstream default)
security = none    # no TLS
```

### Cert check extended
- Startup cert check runs for both `SSL` and `auto` modes
- Per-fork throttled check runs when TLS is actually used

**Verification:**
1. Start MUDitM with `security = auto`
2. `openssl s_client -connect localhost:2027` → TLS works, MUD greeting appears
3. `telnet localhost:2027` → plaintext works, MUD greeting appears
4. Both connections show correct IPADDRESS via `stat me`
