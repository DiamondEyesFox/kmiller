# KMiller 5.25 Release Notes

## Fixed

- Launch failure caused by stale Poppler ABI linkage (`libpoppler.so.155` missing on updated systems).
- File chooser open mode now returns selected file(s) instead of current folder.
- Delete behavior now respects preferences:
  - `advanced/moveToTrash`
  - `advanced/confirmDelete`
- Added explicit permanent-delete action path for context menu "Delete Permanently".

## Internal

- Added `Pane::selectedUrls()` helper for consistent selection handling.
- Updated versioning metadata to 5.25 in source and fallback build version.

## Audit

A deep Finder parity roadmap is included in:

- `docs/FINDER_PARITY_AUDIT.md`
