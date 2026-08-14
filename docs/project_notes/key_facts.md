# Key Facts

Environment configuration, ports, versions, and constraints.

---

## Upstream

- **Repository**: `github.com/RahjIII/MUDitM` (Jeff Jahr, The Last Outpost MUD)
- **Version**: 1.0 (Feb 2024, `$Id: muditm.c,v 1.14`)
- **License**: LGPL-3.0

## Fork

- **Repository**: `github.com/terrillt/MUDitM`
- **Submodule path**: `src/externals/MUDitM/`
- **Architecture decision**: ADR-015 in SKMUD's `docs/project_notes/decisions.md`

## Branch Strategy

| Branch | Purpose |
|--------|---------|
| `main` | Upstream mirror. PR-worthy changes only. Cherry-pick from dev. |
| `master` | SKMUD stable. Deployed alongside production MUD server. |
| `dev` | SKMUD development. All work goes here first. |

## Port Architecture

MUDitM listens on port N (front door), forwards to the MUD server on port N+1 (internal).

| Environment | MUDitM Port | MUD Port | Config File |
|-------------|-------------|----------|-------------|
| Dev (macOS) | 2026 | 2027 | `muditm-dev.conf` (gitignored) |
| Test (Docker) | 2026 | 2027 | `muditm-test.conf` |
| CI | 1996 | 1997 | `muditm-ci.conf` |
| Production | 1996 | 1997 | `muditm-prod.conf` |

## Connection Limits

| Environment | max-children | Rationale |
|-------------|-------------|-----------|
| Production | 900 | Below MUD's ~1020 fd ceiling |
| Test | 100 | Covers 27-connection test harness with headroom |
| CI | 100 | Same as test |
| Dev | 0 (unlimited) | Local only, not exposed |

## Build Requirements

- GLib 2.0, OpenSSL 1.1.0+, PCRE2, zlib
- macOS: Homebrew `pkgconf glib pcre2 openssl zlib`
- Linux: `gcc, make, pkg-config, libglib2.0-dev, libpcre2-dev, libssl-dev, zlib1g-dev`
- Does NOT build on CentOS 7 (requires OpenSSL 1.1.0+, CentOS 7 ships 1.0.2)

## Commit Message Style

- **PR-worthy commits** (for `main`): upstream style -- concise subject, explanatory paragraphs
- **SKMUD-specific commits** (stay on `dev`/`master`): SKMUD style -- subject + categorized bullets
