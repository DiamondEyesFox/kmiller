# KMiller 5.25.4 Release Notes

Date: 2026-04-30

## Highlights

- Daily-driver parity pass focused on reducing reasons to fall back to Dolphin or Thunar.
- Settings truthfulness fixes for Miller column width and symlink navigation.
- File operation feedback and refresh improvements.
- Open With backend extraction and GIO fallback for mixed Hyprland/KDE/GNOME MIME environments.
- Archive and file-operation QA expansion.
- New repeatable headless and guided manual QA harnesses for release checks.

## Fixed And Improved

- Pane navigation history now routes through `PaneNavigationState`.
- Miller column width setting now applies to current and future Miller columns.
- Follow symbolic links now affects directory symlink navigation in Pane and Miller views.
- Unimplemented file-extension setting is hidden until display behavior is real.
- Paste/move refreshes destination and source parent folders.
- Duplicate uses centralized `FileOpsService::copyAs()`.
- Duplicate success reports through the status bar instead of interrupting with a modal.
- Copy, move, trash, permanent delete, and new-folder operations report status-bar success/error feedback.
- Open With dialog uses shared modal presentation and Finder-style custom command input.
- Open With discovery and launching now live in `OpenWithService`.
- GIO MIME associations are used as a fallback when KDE's service cache is stale.
- Added `Show in Parent Folder` for single local selections.
- Search field now communicates recursive search and shows an active state while searching.
- Inline rename cleanup no longer blanket-disconnects all delegate `closeEditor` handlers.

## QA And Release Checks

- Added `--path <folder>` for launching KMiller against a disposable fixture path.
- Added `--qa-fixture <folder>` for noninteractive file-operation checks.
- Added `--qa-ui-logic <folder>` for initial-path and symlink-follow checks.
- Added `--qa-open-with <file>` for noninteractive Open With app discovery checks.
- Added `--qa-archive <folder>` for noninteractive archive create/extract/conflict checks.
- Added `tools/qa_headless.sh` to run fixture operations, UI logic checks, and Xvfb GUI smoke.
- Added `tools/kmiller_manual_qa_harness.py` and `tools/run_manual_qa_harness.sh` for guided manual release QA.

## Verification

- `cmake --build build --parallel 4`
- `ctest --test-dir build --output-on-failure`
- `tools/qa_headless.sh build/kmiller`
- Guided manual QA report: `/home/diamondeyesfox/Downloads/Kmiller Testing Folder/KMiller QA Report 20260430-162740.md`
