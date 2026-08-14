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

### 5.11.0
- [x] `5.11.0` `2026-06-20` `test_max_children` connection limit test -- verifies `max-children` config by connecting N+1 clients and asserting the last is deferred (not rejected). Built separately from main binary via `make tests`. **Files:** `tests/test_max_children.c`, `makefile`
