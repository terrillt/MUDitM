# Bugs

Active bugs and investigations in MUDitM. **Open only** -- when a bug
is resolved, move it to the appropriate changelog with structured
fields (Issue / Root Cause / Solution / Prevention) and delete it
from here.

Numbering is `BUG-MUDITM-NNN`, independent of SKMUD's `BUG-NNN`. A
defect whose root cause is in the SKMUD integration belongs in
SKMUD's `bugs.md` even if the symptom appears here.

## Entry Format

```
### BUG-MUDITM-NNN: Brief description
- **Status**: Open | Investigating | Blocked
- **Found**: YYYY-MM-DD
- **Symptom**: what is observed
- **Root cause**: if known; say "unknown" rather than guessing
- **Workaround**: if any
- **Next step**: the specific thing that would move it forward
```

---

## Open

### BUG-MUDITM-001: Silent death on test

- **Status**: Open, under observation
- **Found**: 2026-08-10
- **Symptom**: MUDitM process disappeared on test after an extended idle period (~14 hours). No log entry, no crash report, no sanitizer output.
- **Root cause**: unknown
- **Workaround**: `src/startup` watchdog restarts MUDitM within 30 seconds
- **Next step**: wait for recurrence with signal handlers deployed; the `muditm-<boot>.err` file and connection log will distinguish crash from clean stop

**Investigation**: ASAN `log_path` was added in 5.12.2 to capture sanitizer reports. After 2,240 reports were collected, all were LeakSanitizer exit reports (55KB each, `g_key_file_new` / `g_key_file_get_string` never freed). Zero use-after-free, buffer overflow, double-free, or SEGV reports. Zero watchdog restarts observed. The leak reports were eliminated by freeing config resources at exit.

Signal handlers were added to distinguish crashes from clean stops. A SIGTERM now logs "Received signal 15, shutting down." to the connection log. A SIGSEGV/SIGBUS/SIGABRT writes a crash identifier with backtrace to `log/muditm-<boot>.err`. The next occurrence will leave evidence.

**Not confirmed as a crash**: the original death may have been a deliberate stop that looked like a crash because nothing logged the parent's exit. Blocked on observing the next occurrence with the instrumentation in place.

**Related**: `src/startup` watchdog monitors MUDitM and restarts it within 30 seconds if missing.
