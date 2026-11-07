# Test Changelog

Completed test work. Append-only history.

## Entry Format

```
- [x] `VERSION` `YYYY-MM` Brief description -- details. **Files:** `file`
```

Within each category, items are grouped by version (newest first), sorted by date within a version (newest first).

---

## Contents

- [Tests](#tests)

## Tests

### 5.13.0
- [x] `5.13.0` `2026-11-07` `test_max_children` leaves evidence and cannot outlive its caller -- SIGPIPE is ignored so the exit cleanup always runs (a proxy child closing a connection before the test wrote to it used to kill the process silently, leaving MUDitM and the echo server holding the caller's output pipe open); stdout is line-buffered and the first line is the test's pid, so a caller sees each step as it happens and can tell this run's MUDitM (its config is named after the pid) from any other; an optional second argument names a directory where the MUDitM log is kept as `muditm-selftest.log` (an old one that cannot be removed fails the run, since it would satisfy the log check by itself); cleanup gives each helper five seconds after SIGTERM, then SIGKILL. **Files:** `tests/test_max_children.c`

### 5.11.0
- [x] `5.11.0` `2026-06-20` `test_max_children` connection limit test -- verifies `max-children` config by connecting N+1 clients and asserting the last is deferred (not rejected). Built separately from main binary via `make tests`. **Files:** `tests/test_max_children.c`, `makefile`
