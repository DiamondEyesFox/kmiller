# KMiller Finder Parity Audit (v5.25.1 working tree)

Date: 2026-02-14  
Scope: `/home/diamondeyesfox/kmiller-5.24-hotfix/src`

## Executive Summary

KMiller now launches reliably on this machine, and this pass fixed several user-facing correctness gaps (tab-strip context menu, horizontal scrollbar theming, right-click selection behavior, file chooser filter enforcement, persistence of core window settings, and Quick Look media preview for audio/video).

It is still not at Finder parity yet. The biggest remaining gaps are still around file operation UX/safety (progress + undo + conflict resolution), metadata/search depth (tags + smart folders), and runtime wiring for a few saved preferences.

## Fixed In This Pass

- **Empty tab-strip right-click menu** with `New Tab` (plus close actions on tab targets): `src/MainWindow.cpp:53`
- **Horizontal scrollbar theming hardening** for Dark/Finder themes (including nested scroll areas): `src/MainWindow.cpp:534`, `src/MainWindow.cpp:635`
- **Miller column scrollbar theming** to prevent white bottom bars under local widget styles: `src/MillerView.cpp:69`
- **Finder-like right-click selection behavior** in Miller and classic views: `src/MillerView.cpp:112`, `src/Pane.cpp:367`
- **Empty-space New Folder now respects target folder** (important in Miller subcolumns): `src/Pane.cpp:860`, `src/Pane.cpp:1119`
- **File chooser filter enforcement** in open mode (portal filters now materially affect selection): `src/FileChooserDialog.cpp:276`
- **Main window setting persistence** for toolbar/hidden/preview/view/zoom updates: `src/MainWindow.cpp:150`, `src/MainWindow.cpp:356`, `src/MainWindow.cpp:478`
- **Miller status bar selection accounting** now reports selected count/size in Miller view too: `src/Pane.cpp:1298`
- **Quick Look media support** now previews audio and video with in-dialog playback controls: `src/QuickLookDialog.cpp:79`, `src/QuickLookDialog.cpp:221`

## High-Impact Findings (Remaining)

### P1 - File operation UX is still below Finder-grade

- No centralized progress/cancel UI, no undo stack, and no built-in conflict-resolution flow for copy/move/trash/delete.
- Current wrapper only forwards raw KIO jobs and does not provide orchestration/policy layer: `src/FileOpsService.cpp:23`

### P1 - Some operations remain shell-based and fragile

- Archive create/extract relies on external tools (`zip`, `tar`, `7z`, `unrar`) with limited capability checks and no in-app progress model: `src/Pane.cpp:1487`, `src/Pane.cpp:1564`
- Directory duplicate still blocks on `cp -r` with timeout (`waitForFinished(10000)`), which can fail on larger trees and block responsiveness: `src/Pane.cpp:1674`

### P2 - Saved settings still not fully connected to behavior

These preferences are stored in settings UI but still have no runtime consumers outside `SettingsDialog`:

- `view/showThumbnails`
- `view/showFileExtensions`
- `view/millerColumnWidth`
- `advanced/defaultTerminal`
- `advanced/followSymlinks`

Evidence: only read/write locations are in `src/SettingsDialog.cpp:226` and `src/SettingsDialog.cpp:249`.

### P2 - Search remains shallow compared to Finder

- Current search is wildcard filtering in current folder only; no recursive mode, no attribute queries, no saved searches/smart folders: `src/Pane.cpp:362`

## Updated Finder Parity Scorecard

| Area | Approx. parity | Notes |
|---|---:|---|
| Core navigation (columns, tabs, back/forward) | 74% | Strong base, improved tab UX |
| File operations | 56% | Core actions work, but UX safety/progress/undo still behind |
| Preview & Quick Look | 72% | Now includes audio/video playback, still behind Finder plug-in breadth |
| Search & organization | 25% | Folder-local filter only |
| Metadata & tags | 10% | No Finder tag system yet |
| Reliability/release hardening | 62% | Launch/linkage improved; GitHub release flow still needs automation |
| Tests/QA | 22% | Basic smoke coverage present, no workflow integration tests |

## Roadmap To Finder Parity

### Phase A - Operations Safety Layer

1. Expand `FileOpsService` into a real controller with:
   - progress notifications,
   - conflict policy prompts,
   - cancellation,
   - undo journal for move/rename/trash.
2. Replace shell duplication path with KIO-native recursive copy/move semantics.
3. Replace archive shell calls with robust capability probing + async progress UI.

### Phase B - Preference Fidelity

1. Wire `view/millerColumnWidth` to runtime column sizing in `MillerView`.
2. Wire `view/showFileExtensions` and `view/showThumbnails` into item delegate/model presentation.
3. Wire `advanced/defaultTerminal` and `advanced/followSymlinks` to actual commands/navigation logic.

### Phase C - Finder Feature Layer

1. Add recursive search mode and indexed metadata filters.
2. Add saved searches (smart folders).
3. Add tags/labels and tag-based sidebar filtering.

### Phase D - QA and Release Discipline

1. Add integration tests for open/copy/move/trash/duplicate/archive flows.
2. Add file chooser portal tests (filters, save/open, directory mode).
3. Automate version/tag/release pipeline with preflight checks (branch sync, tag uniqueness, smoke pass).

## Recommended Next Implementation Batch

1. FileOpsService orchestration (progress + conflict + undo skeleton).
2. Replace blocking directory duplicate implementation.
3. Wire `millerColumnWidth` and `showFileExtensions` (highest-visibility settings gap).
4. Replace archive shell commands with job-based async workflow + conflict handling.
