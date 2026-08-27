# MUDitM -- SKMUD Fork

TLS termination proxy for MUD servers. Forked from [RahjIII/MUDitM](https://github.com/RahjIII/MUDitM).

## Authority

This is a git submodule of SKMUD. All write restrictions in the parent repo's root `AGENTS.md` ("Authority and Write Restrictions") apply here — commits and pushes here move the gitlink every SKMUD checkout resolves against.

Additional fork constraints:
- Changes must not rely on tools or patterns the upstream author isn't using. Test with stock system toolchain.
- Work on `dev` branch; cherry-pick to `main` for upstream-ready PRs.
- Never modify upstream's original files on `main` without a clear upstream-submission plan.

**See also:**
- `docs/muditm-system.md` -- Detailed architecture: process model, signal handling, TLS, MCCP2, MNES, proxy loop, logging, config reference, cert expiry, crash diagnostics
- `docs/project_notes/key_facts.md` -- Ports, environments, build requirements, branch strategy
- `docs/project_notes/bugs.md` -- Active bugs and investigations
- `docs/project_notes/roadmap.md` -- Planned work, upstream PRs
- `docs/project_notes/pull-requests.md` -- PR tracking for upstream submissions
- `docs/project_notes/server-changelog.md` -- Runtime behavior changes
- `docs/project_notes/infra-changelog.md` -- Build system and project structure changes
- `docs/project_notes/test-changelog.md` -- Test work
- `docs/integration-plan.md` -- Original macOS port plan (historical)

## Build

```bash
# macOS
brew install pkgconf glib pcre2 openssl zlib
make clean && make

# Linux
make clean && make

# Sanitizer builds
make clean && make EXTRA_CFLAGS="-fsanitize=address -fno-omit-frame-pointer" EXTRA_LDFLAGS="-fsanitize=address"

# Tests (separate from main binary)
make tests
./tests/test_max_children
```

## Run

```bash
./muditm -c muditm-dev.conf      # daemon mode (fork per connection)
./muditm -d -c muditm-dev.conf   # debug mode (foreground, single connection)
./muditm -v                       # version
```

## Tests

**C tests:** `test_max_children.c` — connection limit test. Built via `make tests`.

**Python tests:** None yet. When adding, follow SKALD's `tests/` naming pattern (`test_server_muditm_*.py` / `test_unit_muditm_*.py`). Skip if MUDitM not running (`pgrep -x muditm`).

## Configuration

See `docs/muditm-system.md` "Configuration" for the full config key reference and `docs/project_notes/key_facts.md` for per-environment port mappings.

| File | Environment | Ports |
|------|-------------|-------|
| `muditm-dev.conf` | macOS dev (gitignored) | 2026 -> 2027 |
| `muditm-test.conf` | Docker test | 2026 -> 2027 |
| `muditm-ci.conf` | CI pipeline | 1996 -> 1997 |
| `muditm-prod.conf` | Production | 1996 -> 1997 |

## Architecture

Fork-per-connection proxy with PCRE2 pattern matching on the telnet byte stream. See `docs/muditm-system.md` for details.

- **TLS**: auto-detect (first-byte peek) or forced. Cert loaded post-fork for live renewal.
- **MCCP2**: compression on both sides. State machine tracks negotiation for MNES reporting.
- **MNES**: injects IPADDRESS, TRUSTED_IPADDRESS, SECURITY, COMPRESSION, PROXY_NAME, CLIENTPORT.
- **Signals**: SIGTERM/SIGINT clean shutdown, SIGSEGV/SIGBUS/SIGABRT crash with backtrace, SIGCHLD zombie reaping, SIGPIPE ignored.
- **DoS**: `max-children` caps forks, `listen-backlog` controls kernel queue.

## SKMUD Integration

See `docs/muditm-system.md` "MNES Variable Mapping" for how `comm.cpp` handles MNES variables from MUDitM.
