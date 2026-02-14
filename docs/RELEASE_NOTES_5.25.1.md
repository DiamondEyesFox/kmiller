# KMiller 5.25.1 Release Notes

Date: 2026-02-14

## Highlights

- Safe release-track integration onto `origin/main` (`release/v5.25.1`).
- Quick Look now previews **audio and video** files with playback controls.
- Finder-style UX polish for tabs, context menus, and selection behavior.

## Fixed

- Added tab-strip empty-space right-click context menu with `New Tab`.
- Fixed horizontal scrollbar theming consistency (including views that previously showed white bottom bars).
- Right-click now selects unselected items first (Miller and classic views), matching Finder expectations.
- Empty-space `New Folder` now creates in the clicked column/folder target in Miller view.
- File chooser filters are now enforced in open mode (glob and MIME-aware matching).
- Main window runtime settings (toolbar/hidden/preview/view/zoom) now persist reliably.
- Miller selection status accounting now updates selected count/size.

## Quick Look

- Added media preview support for common audio/video formats.
- Added media playback controls (play/pause + seek) in Quick Look.
- Improved unsupported-file fallback with file/type messaging.

## Internal

- Added `FileOpsService` abstraction and routed more open/copy/move/trash/delete operations through it.
- Added lightweight linkage smoke checks (`ctest` + `tools/smoke_release.sh`).
- Version metadata updated to `5.25.1`.

## Audit

Updated parity roadmap:

- `docs/FINDER_PARITY_AUDIT.md`
