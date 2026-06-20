# Infrastructure Changelog

Completed build system, project structure, and tooling work. Append-only history.

## Entry Format

```
- [x] `VERSION` `YYYY-MM` Brief description — details. **Files:** `file`
```

Within each category, items are grouped by version (newest first), sorted by date within a version (newest first).

---

## Contents

- [Configuration](#configuration)
- [Build System](#build-system)
- [Project Structure](#project-structure)

## Configuration

### 5.11.0
- [x] `5.11.0` `2026-06-18` Connection limits in SKMUD configs — `max-children` and `listen-backlog` set per environment. Production: 900/16. Test and CI: 100/16. Upstream example config updated with documented defaults (0/16). **Files:** `muditm.conf`, `muditm-test.conf`, `muditm-ci.conf`, `muditm-prod.conf`
- [x] `5.11.0` `2026-06-08` CI pipeline config — `muditm-ci.conf` for CI environments (local and GitHub Actions). Listens on 1996, forwards to MUD on 1997 (matching production port architecture). Uses `demon=true` for fork-per-connection mode. Started by `run-ci.sh` when the binary exists in the image. **Files:** `muditm-ci.conf`
- [x] `5.11.0` `2026-06-06` Test config cert path consolidation — cert/key paths changed from `src/externals/MUDitM/test-cert.pem` to `/home/mud/certs/fullchain.pem`. Docker image generates self-signed fallback at that path; VPS volume mount overlays with Let's Encrypt. Same config works both environments. **Files:** `muditm-test.conf`
- [x] `5.11.0` `2026-06-02` Committed config files for test and production — `muditm-test.conf` (Docker: 2026→2027, self-signed cert, skmud control socket) and `muditm-prod.conf` (production: 1996→1997, Let's Encrypt cert paths, skmud control socket). Port architecture: MUDitM=N (front door), MUD=N+1 (internal). Dev config remains gitignored. **Files:** `muditm-test.conf`, `muditm-prod.conf`

## Build System

### 5.11.0
- [x] `5.11.0` `2026-06-20` `make tests` target — builds test binaries separately from main binary. Not part of `all` (Xcode env conflicts). Called by deploy.sh and auto-built by pytest on demand. `make clean` removes test binaries. `.dSYM` added to .gitignore. **Files:** `makefile`, `.gitignore`
- [x] `5.11.0` `2026-06-19` Sanitizer and coverage build support — EXTRA_CFLAGS/EXTRA_LDFLAGS variables appended to Makefile flags, enabling ASan/TSan/gcov builds via command line. Docker, CI, and deploy.sh pass matching flags automatically. **Files:** `makefile`, `.gitignore`
- [x] `5.11.0` `2026-05-31` Subdirectory compilation support — added mkdir -p in build rule for skmud/ subdirectory object files. **Files:** `makefile`
- [x] `5.11.0` `2026-05-31` certcheck.c and skmud/skmud.c added to MUDITM_CFILES — certificate expiry module and SKMUD-specific extensions compiled into the binary. **Files:** `makefile`
- [x] `5.11.0` `2026-05-31` macOS build support — replaced malloc.h with stdlib.h (glibc-specific, doesn't exist on macOS). Use pkg-config for OpenSSL and PCRE2 discovery (Homebrew non-standard prefixes). Default compiler to cc instead of hardcoded gcc. Removed ctags from default build target (still available via `make tags`). Changed rm -rI to rm -rf (GNU-only flag). **Files:** `makefile`, `handlers.c`, `iobuf.c`, `proxy.c`

## Project Structure

### 5.11.0
- [x] `5.11.0` `2026-05-31` docs/ directory — server and infrastructure changelogs for this fork. **Files:** `docs/server-changelog.md`, `docs/infra-changelog.md`
- [x] `5.11.0` `2026-05-31` CLAUDE.md project guide — build, run, config, architecture, branch strategy, commit style, SKMUD integration reference. **Files:** `CLAUDE.md`
- [x] `5.11.0` `2026-05-31` skmud/ subdirectory — SK-specific extensions separated from upstream code. **Files:** `skmud/skmud.c`, `skmud/skmud.h`
- [x] `5.11.0` `2026-05-31` .gitignore for fork — build artifacts, certificates, dev config, macOS metadata. **Files:** `.gitignore`
