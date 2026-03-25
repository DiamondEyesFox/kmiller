# KMiller Feature Timeline

Last updated: 2026-03-06

## Purpose

This document answers a simple historical question:
- what got added when?

It condenses the larger archaeology docs into a feature-and-milestone timeline, starting from the genesis conversation and continuing into the early release era.

Related docs:
- [KMILLER_GENESIS_TIMELINE.md](KMILLER_GENESIS_TIMELINE.md)
- [KMILLER_CONVERSATION_ITERATIONS.md](KMILLER_CONVERSATION_ITERATIONS.md)
- [KMILLER_RELEASE_ERA_RECOVERY.md](KMILLER_RELEASE_ERA_RECOVERY.md)
- [KMILLER_RECOVERABILITY_MATRIX.md](KMILLER_RECOVERABILITY_MATRIX.md)

## 2025-07: Concept Prehistory

### 2025-07-05 to 2025-07-21
What exists:
- dissatisfaction with existing Linux file managers
- explicit desire for:
  - Finder-like UX
  - true Miller columns
  - Quick Look / spacebar preview
  - polished macOS-style workflow

Meaning:
- KMiller does not exist yet by name
- but the exact problem statement is already clear

## 2025-08-07: Genesis and Proto-KMiller

### 2025-08-07 22:48 CDT
Added:
- the actual project genesis prompt
- "create it"

Meaning:
- this is the moment the idea becomes a software project

### 2025-08-07 22:51 CDT — `proto-genesis-0`
Added:
- first generated project skeleton
- first literal `kmiller` executable naming
- basic fixed 3-column Miller layout
- KDE/Qt starting point

Main files:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.h`
- `mainwindow.cpp`

### 2025-08-07 22:51 to 22:52 CDT — `proto-quicklook-1`
Added:
- first Quick Look
- image preview
- text preview
- PDF preview
- `Space` / `Escape`

Meaning:
- this is the first moment KMiller feels like your desired app instead of a file-list demo

### 2025-08-07 22:52 CDT — `proto-media-2`
Added:
- audio/video playback in Quick Look

Meaning:
- Quick Look ambition jumps immediately beyond a basic preview popup

### 2025-08-07 22:52 CDT — `proto-dynamic-columns-3`
Added:
- dynamic columns instead of a fixed 3-column proof of concept

Meaning:
- first true attempt at real Finder-style Miller behavior

### 2025-08-07 22:53 to 22:58 CDT — `proto-packaged-kmiller-4`
Added:
- first packaged full-project form
- zip/base64/shareable project payloads
- practical product identity as plain KMiller

Meaning:
- first downloadable KMiller phase

### 2025-08-07 23:01 to 23:04 CDT — `proto-kmiller-mainline-5`
Added:
- spec pivot from “Miller demo” to “real daily-driver file manager”
- mainstream file manager expectations become explicit:
  - path bar
  - tabs
  - normal view modes
  - broader usability parity

Meaning:
- this is the design pivot that leads to KMiller2

### 2025-08-07 23:04 CDT — `KMiller2`
Added:
- icons/list/compact plus Miller view
- places sidebar
- path bar
- tabs
- search filter
- sorting
- dedicated `QuickLookDialog`

Meaning:
- first clear “full file manager plus Miller Columns” milestone

### 2025-08-07 23:08 CDT — `KMiller3`
Added:
- KIO / KDirModel direction
- breadcrumb path bar
- split view
- zoom slider
- undo/redo
- trash
- Open With
- thumbnails
- pane architecture

Meaning:
- first strongly KDE-native, Finder-aspirational architecture jump

## 2025-08-28 to 2025-08-29: Rehydration and Local Hardening

### `KMiller3-rehydration`
Added / changed:
- project is re-provided after expired payloads
- real local build attempts on Arch/KDE
- dependency fixes for:
  - KF6
  - Poppler
  - KUrlNavigator
- compile-surgery phase begins

Meaning:
- first real “get it running on the actual machine” phase

### `KMiller3-plus`
Added:
- menu bar
- preferences/settings pass
- arrow-key behavior tuning
- toolbar slimming / polish

### `KMiller3-quicklook-wired`
Added:
- Quick Look explicitly exposed as a user feature
- `Space`
- menu action
- right-click Quick Look action

### `KMiller3-millerfix`
Added / changed:
- right-click context menu work
- double-click should open, not rename
- Miller arrow-key behavior tuning
- interaction model fixes

Meaning:
- this is the first serious Finder-parity interaction tuning pass

### `KMiller-source-tree-12file`
Added:
- full source tree delivered in text chunks
- durable source delivery after expiring zips became annoying

Meaning:
- first durable source-tree handoff phase

### `KMiller-local-install-presemver`
Added / changed:
- rapid local bug-fix loop for:
  - incomplete `Pane.cpp`
  - missing includes
  - Miller navigation
  - Quick Look
  - crash guards
  - rename/open behavior

Meaning:
- first real local hardening before formal versioning

## 2025-08-29: Release-Era Begins

### `0.3.0`
Added:
- versioned installs under `/opt/kmiller/versions/<version>`
- updater concept and script
- installer/updater architecture

Meaning:
- KMiller becomes a release-managed application, not just a build folder

### `0.4.0`
Added:
- corrected install layout
- `.desktop` integration cleanup
- stable launcher symlink strategy
- maturing auto-versioning flow

Meaning:
- first recognizable install-and-launch management layer

### `0.4.x-rightclick-history-loop`
Focus:
- right-click crash
- Miller weirdness
- launcher freshness confusion
- behavior drift between source and installed app

Meaning:
- first strong sign that release machinery and app behavior are evolving faster than the process can cleanly carry

### `0.4.x-patchnotes-attempt`
Added:
- early patch-notes thinking
- archive/latest notes concepts
- attempts to make releases self-describing

### `0.4.0-revert-crisis`
Added / changed:
- rollback patterns
- “known good” mentality becomes important
- emphasis on keeping stable snapshots

### `0.4.2-rescue-stable`
Added / changed:
- rescue rebuild
- repaired `Pane` behavior
- path bar / double-click / Miller default stabilization
- git-init / snapshot / tag guidance

Meaning:
- first clearly remembered stable anchor of the early release era

### `0.4.2-preview-multiselect`
Added:
- preview-pane / selection interaction refinements
- multi-select era tuning begins

### `0.4.6-self-archiving`
Added:
- self-archiving install tree
- source snapshot stored alongside installed version
- `build_release.sh` grows up

Meaning:
- early release reproducibility starts becoming intentional

### `0.4.7-build-release-experiment`
Added / changed:
- repair-and-build automation for a late-night release push
- version title wiring via compile definitions
- context-menu API repair attempts

Meaning:
- one of the earliest “push this into a releasable state tonight” iterations

## 2025-08-29 to 2025-08-30: Patch Notes and Process Maturity

### `0.5.0-patchnotes-release`
Added:
- `make_patchnotes.sh`
- `build_release.sh` patch-notes flow
- release wrapper around build, install, notes, and tagging

Meaning:
- KMiller starts to behave like a project with a release process, not just versioned binaries

### `0.5.1-patchnotes-design`
Added:
- per-version patchnotes layout under installed versions
- structured release-note design examples

Meaning:
- patch notes become part of the product/release architecture itself

## Surviving Source/Binary Anchors

### Exact source survivors
- recovered source trees for the proto/KMiller2/KMiller3 eras
- exact installed source snapshot at:
  - `/opt/kmiller/versions/0.5.0`

### Binary-only early survivors
- installed binaries remain for:
  - `0.4.0`
  - `0.4.1`
  - `0.4.2`
  - `0.4.4`
  - `0.4.5`
  - `0.4.6`
  - `0.4.7`

## Big Picture

The historical arc is:
1. desire for a better Finder-like Linux file manager
2. genesis prototype
3. Quick Look and media ambition immediately added
4. real file-manager features layered on
5. KDE-native architecture jump
6. local build and interaction hardening
7. release/install/version infrastructure
8. stability/rescue workflow
9. release tooling and patch notes

That is an unusually fast and ambitious evolution for a first-ever software project.
