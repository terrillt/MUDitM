# MUDitM -- SKMUD Fork

TLS termination proxy for MUD servers. Forked from [RahjIII/MUDitM](https://github.com/RahjIII/MUDitM) (v1.0, Feb 2024). This fork adds macOS build support, bug fixes, security hardening, and SKMUD-specific features.

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

Two kinds of tests live in `tests/`:

**C tests** (standalone binaries):
- `test_max_children.c` -- connection limit test. Built via `make tests`.

**Python tests** (SK test harness, future):
- `test_server_muditm_*.py` -- server chain (raw connection, no login)
- `test_unit_muditm_*.py` -- unit chain (server-less)

Python tests follow SK naming conventions and are collected by the
SKMUD test harness via the same extended glob pattern used for SKALD
submodule tests. The harness is already wired to discover tests from
submodule paths. Integration tests should skip if MUDitM is not
running (check `pgrep -x muditm`).

No Python tests exist yet. When adding one, follow the pattern in
`src/externals/SKALD/tests/` and document in SKMUD's
`tests/run_tests.py` glob list.

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

SKMUD's `comm.cpp` handles MNES variables from MUDitM:
- `IPADDRESS` -> `d->claimed_ip` (untrusted display)
- `TRUSTED_IPADDRESS` -> `d->trusted_ip` (locked, used for proxy checks)
- `SECURITY`, `COMPRESSION`, `PROXY_NAME` -> proxy-guarded, shown in `terminals`
- Reverse DNS on trusted_ip (background thread)
- 127.0.0.1 marked `PROXY_ALLOWED` (migration 5.9.0-008)

## Commit Rules

- Never commit directly to `main`. Work on `dev`, cherry-pick to `main` for upstream PRs.
- PR-worthy commits: upstream style (concise subject, explanatory paragraphs).
- SKMUD-specific commits: SKMUD style (subject + categorized bullets).
