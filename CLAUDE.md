# MUDitM — SKMUD Fork

TLS termination proxy for MUD servers. Forked from [RahjIII/MUDitM](https://github.com/RahjIII/MUDitM) (v1.0, Feb 2024). This fork adds macOS build support, bug fixes, security hardening, and SKMUD-specific features.

## Quick Reference

- **Upstream**: `github.com/RahjIII/MUDitM` (Jeff Jahr, The Last Outpost MUD)
- **Fork**: `github.com/terrillt/MUDitM`
- **License**: LGPL-3.0
- **Parent project**: SKMUD (`/Users/mud/SKMUD/`), submodule at `src/externals/MUDitM/`
- **Integration docs**: `docs/integration-plan.md`, `SKMUD/docs/modernization-roadmap.md`
- **Architecture decision**: ADR-015 in `SKMUD/docs/project_notes/decisions.md`

## Branch Strategy

| Branch | Purpose |
|--------|---------|
| `main` | Upstream mirror. PR-worthy changes only. Cherry-pick from dev when ready. |
| `master` | SKMUD stable. Deployed alongside production MUD server. |
| `dev` | SKMUD development. All work goes here first. |

**Rule**: Never commit directly to `main`. Work on `dev`, cherry-pick to `main` for PRs.

## Commit Message Style

- **PR-worthy commits** (destined for `main`): Upstream style — concise subject line, detailed explanatory paragraphs describing what changed, why, and how it was tested.
- **SKMUD-specific commits** (stay on `dev`/`master`): SKMUD style — subject line, then categorized bullet points (Server/Infrastructure/Documentation) with sub-bullets for details.

## Build

### macOS (dev environment)
```bash
brew install pkgconf glib pcre2 openssl zlib
make clean && make
```

### Linux (modern distro — Rocky 9, Debian 12, etc.)
```bash
# Install: gcc, make, pkg-config, libglib2.0-dev, libpcre2-dev, libssl-dev, zlib1g-dev
make clean && make
```

**Note**: Does NOT build on CentOS 7 (requires OpenSSL 1.1.0+, CentOS 7 ships 1.0.2).

## Run

```bash
# Daemon mode (fork per connection):
./muditm -c muditm-dev.conf

# Debug mode (foreground, single connection):
./muditm -d -c muditm-dev.conf

# Version:
./muditm -v
```

## Configuration

See `muditm.conf` for the upstream example config with comments.

Dev config (`muditm-dev.conf`, gitignored):
- Listens on port 2027 (TLS + plaintext via auto-detect)
- Forwards to SKMUD game server on localhost:2026
- MNES IPADDRESS forwarding enabled
- MCCP2 compression enabled both sides

Key config option for this fork:
```ini
[client]
security = auto    # peek first byte: TLS if 0x16, plaintext otherwise
security = SSL     # require TLS (upstream default)
security = none    # no TLS
```

## Architecture

- **Fork-per-connection**: Parent process accepts, forks child per client. Child handles one session.
- **Pattern matching**: PCRE2 regex on the byte stream to intercept telnet negotiations (MNES, MCCP2).
- **TLS**: OpenSSL. Cert loaded after fork so updates take effect on next connection.
- **Compression**: MCCP2 on both client and game sides. Decompresses game→client, re-compresses client→game.
- **IP forwarding**: Injects client's real IP as MNES NEW-ENVIRON IPADDRESS variable.

## SKMUD Integration

SKMUD's `comm.cpp` already handles:
- MNES IPADDRESS (stores in `d->client_ip`, used for proxy checks)
- 127.0.0.1 marked as `PROXY_ALLOWED` (migration 5.9.0-008)
- TLS detection on raw port (sends handshake_failure alert) — bypassed when MUDitM is in front

## Change History

- `docs/server-changelog.md` — runtime behavior changes
- `docs/infra-changelog.md` — build system and project structure changes
