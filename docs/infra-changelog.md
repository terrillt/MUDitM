# Infrastructure Changelog

Completed build system, project structure, and tooling work. Append-only history.

## Entry Format

```
- [x] `VERSION` `YYYY-MM` Brief description — details. **Files:** `file`
```

Within each category, items are grouped by version (newest first), sorted by date within a version (newest first).

---

## Contents

- [Build System](#build-system)
- [Project Structure](#project-structure)

## Build System

### 5.11.0
- [x] `5.11.0` `2026-05-31` Subdirectory compilation support — added mkdir -p in build rule for skmud/ subdirectory object files. **Files:** `makefile`
- [x] `5.11.0` `2026-05-31` certcheck.c and skmud/skmud.c added to MUDITM_CFILES — certificate expiry module and SKMUD-specific extensions compiled into the binary. **Files:** `makefile`
- [x] `5.11.0` `2026-05-31` macOS build support — replaced malloc.h with stdlib.h (glibc-specific, doesn't exist on macOS). Use pkg-config for OpenSSL and PCRE2 discovery (Homebrew non-standard prefixes). Default compiler to cc instead of hardcoded gcc. Removed ctags from default build target (still available via `make tags`). Changed rm -rI to rm -rf (GNU-only flag). **Files:** `makefile`, `handlers.c`, `iobuf.c`, `proxy.c`

## Project Structure

### 5.11.0
- [x] `5.11.0` `2026-05-31` docs/ directory — server and infrastructure changelogs for this fork. **Files:** `docs/server-changelog.md`, `docs/infra-changelog.md`
- [x] `5.11.0` `2026-05-31` CLAUDE.md project guide — build, run, config, architecture, branch strategy, commit style, SKMUD integration reference. **Files:** `CLAUDE.md`
- [x] `5.11.0` `2026-05-31` skmud/ subdirectory — SK-specific extensions separated from upstream code. **Files:** `skmud/skmud.c`, `skmud/skmud.h`
- [x] `5.11.0` `2026-05-31` .gitignore for fork — build artifacts, certificates, dev config, macOS metadata. **Files:** `.gitignore`
