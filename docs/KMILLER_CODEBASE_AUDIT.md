# KMiller Codebase Audit

Last updated: 2026-03-06

## Scope

This audit evaluates the current KMiller codebase as it exists in the canonical repo:
- [/mnt/steamssd/Documents/Development/kmiller](/mnt/steamssd/Documents/Development/kmiller)

It focuses on:
- what is genuinely good
- what is weak or risky
- what is ugly from a maintenance perspective
- what is still missing for full-fledged file-manager parity

Grounding:
- current code inspection across `src/`
- current `CMakeLists.txt`
- successful current build from the canonical repo

## Executive Summary

KMiller is already a real file manager, not a toy.

It has:
- a meaningful architecture
- real view modes
- a working Miller view
- Quick Look
- preview pane
- copy/move/trash/delete flows
- Open With
- settings
- properties
- a file chooser / portal direction
- release/install discipline that is far beyond “hello world” quality

But it is not yet at full parity with a polished mainstream file manager.

The main gap is not “missing basic features.”
The main gap is:
- maturity
- cohesion
- behavioral consistency
- deeper platform integration
- maintainability of the growing interaction layer

## Good

### 1. The app has real breadth
Core functionality is spread across sensible modules:
- [MainWindow.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MainWindow.cpp)
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)
- [QuickLookDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/QuickLookDialog.cpp)
- [FileOpsService.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileOpsService.cpp)
- [PropertiesDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/PropertiesDialog.cpp)
- [SettingsDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/SettingsDialog.cpp)
- [FileChooserDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserDialog.cpp)
- [FileChooserPortal.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserPortal.cpp)

That is a real application surface, not one giant experiment file.

### 2. The architectural center is understandable
The split is sensible:
- `MainWindow` manages tabs, global menus, settings load/save, places sidebar, and top-level UX
- `Pane` owns one file-browsing workspace and most per-tab behavior
- `MillerView` specializes Miller interactions
- `QuickLookDialog` owns preview presentation
- `FileOpsService` begins separating file actions from UI wiring

That is a healthy shape.

### 3. Finder-specific interaction intent is clear
The code has strong behavioral intent, especially in:
- slow second-click rename behavior
- Miller left/right semantics
- right-click selection correction
- preview pane and Quick Look interplay
- breadcrumb-first path bar behavior

Files where this is especially visible:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

This is one of KMiller’s biggest strengths: it is trying to behave like something specific, not just “a KDE app with files.”

### 4. Release/install thinking is unusually mature for the project stage
Current build/install flow in [CMakeLists.txt](/mnt/steamssd/Documents/Development/kmiller/CMakeLists.txt) includes:
- version detection
- versioned installs under `/opt/kmiller/versions/<version>`
- stable launcher symlink strategy
- desktop entry installation
- source snapshot installation
- lightweight smoke test hook

That is a real release mindset.

### 5. Quick Look is a serious subsystem
[QuickLookDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/QuickLookDialog.cpp) is not a stub.
It supports:
- images
- text
- PDFs
- audio/video
- folder summaries
- media controls
- metadata
- async directory size work

That is a strong differentiator.

### 6. The file chooser / portal direction is strategically smart
Having:
- [FileChooserDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserDialog.cpp)
- [FileChooserPortal.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserPortal.cpp)

means KMiller is not only trying to be a file manager, but also an ecosystem component. That is a good long-term product idea.

## Bad

### 1. Too much behavior still lives inside `Pane.cpp`
[Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp) is doing a lot:
- navigation
- preview pane
- context menus
- file ops wiring
- clipboard semantics
- archive compression/extraction
- Open With dialog logic
- rename handling
- thumbnail logic
- sorting
- history
- pathbar sync
- selection logic

That makes it powerful, but also brittle.

This is the single biggest maintainability issue.

### 2. Settings are broader in UI than in actual enforcement
[SettingsDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/SettingsDialog.cpp) exposes settings like:
- default terminal
- confirm delete
- move to trash
- follow symlinks
- thumbnail behavior
- file extension display
- Miller column width

But the codebase does not appear to enforce all of these consistently across the app.

So the settings surface is ahead of the implementation surface.

### 3. File operations abstraction is still thin
[FileOpsService.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileOpsService.cpp) is a good start, but right now it is mostly a convenience wrapper.
It does not yet centralize:
- job progress UX
- undo/redo semantics
- collision handling strategy
- consistent error handling
- notifications
- copy/move policy

That means file-operation behavior is still scattered in practice.

### 4. Properties dialog is only partially mature
[PropertiesDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/PropertiesDialog.cpp) works, but it shows unfinished depth:
- directory size still has a TODO
- bulk rename is TODO-ish / intentionally incomplete
- ownership/permission editing is basic
- metadata breadth is limited compared with a mature file manager

It exists, but it is not yet a deeply polished properties system.

### 5. Portal/file chooser integration is promising but still thin
The portal layer is structurally good, but it still looks like an early implementation rather than a hardened desktop-portal backend.
Potential weak points include:
- limited option interpretation
- no deeper parent-window integration
- likely missing edge-case behavior for complex portal callers

So this is a good feature, but not yet “production-grade desktop portal replacement.”

## Ugly

### 1. Interaction logic is duplicated across classic views and Miller view
There is a lot of parallel behavior between:
- classic views (`iconView`, `detailsView`, `compactView`)
- `MillerView`

Examples include:
- selection logic
- rename timing behavior
- context-menu handling
- Quick Look triggering
- open semantics

That duplication makes parity bugs more likely, because one view can drift from the others.

### 2. UI logic and domain logic are entangled
A lot of operational code is still closely tied to widget code.
For example in [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp), UI presentation and file-operation decisions are deeply interleaved.

This makes it harder to:
- test behavior independently
- reuse logic cleanly
- reason about regressions

### 3. Pathbar/history/selection interactions are fragile
Recent work already showed this clearly: breadcrumb sync and Quick Look could break each other.
That tells us the current navigation model is not fully factored yet.

The app works, but some interactions are still coupled in ways that make regressions easy.

### 4. Theme/system polish is partly centralized, partly ad hoc
There is some good top-level theming, but a lot of widget styling is still embedded locally in code:
- preview widgets
- Miller view scrollbars/selection
- Quick Look styling
- zoom slider styling
- file chooser sidebar styling

That works, but it makes global visual consistency harder to maintain.

## Missing For Full File-Manager Parity

### 1. Robust undo/redo for file operations
A mainstream file manager should have a much more coherent undo/redo story for:
- move
- copy
- rename
- trash
- delete recovery where possible

There are traces of this ambition historically, but it is not yet a mature current subsystem.

### 2. Job progress and background operations UX
Full parity wants:
- visible job progress
- cancellable long-running operations
- transfer queue / operations panel
- conflict prompts
- resumable-feeling feedback

Right now file operations are functional, but not yet presented like a polished operations system.

### 3. Rich search
Current code includes filtering, but a full file manager wants more:
- true recursive search
- metadata-aware search
- saved searches
- content search where appropriate
- better search-state visibility

### 4. Better archive handling
Current archive support appears pragmatic, but parity would want:
- richer format support
- clearer extraction UX
- archive browsing behavior or better preview support
- stronger dependency/error flow

### 5. Better properties / metadata / permission editing
Parity would want:
- async directory size
- richer metadata panels
- better ownership/group editing
- more complete multi-file properties semantics
- safer bulk property editing

### 6. Stronger Open With / association management
`Open With` exists, but full parity would want:
- more polished app ranking
- better defaults handling
- set default app flows
- clearer custom command UX
- multi-select Open With behavior design

### 7. Better external integration
A polished file manager usually benefits from stronger integration with:
- terminal launching
- trash management
- recent files/places
- mounted volumes/devices
- cloud/remote locations
- desktop portal expectations

KMiller has some of this directionally, but not yet fully.

### 8. Stronger test coverage
Current build/test discipline is improving, but parity-level confidence wants more than a smoke test.
Missing maturity areas include:
- interaction regression tests
- file operation behavior tests
- settings persistence tests
- view parity checks
- Quick Look edge-case tests

## Overall Verdict

### Good
- strong identity
- real architecture
- real features
- serious Quick Look
- meaningful Finder-inspired behavior
- better release thinking than many projects at this stage

### Bad
- oversized `Pane.cpp`
- settings/implementation mismatch
- thin file-ops abstraction
- immature properties/portal depth

### Ugly
- duplicated interaction logic
- UI and domain logic mixed together
- fragile navigation/state coupling
- ad hoc styling in too many places

### Missing
- robust operations UX
- deeper search
- stronger archive handling
- richer properties
- mature Open With/defaults flow
- stronger integration and tests

## Final Assessment

KMiller is already beyond “cool prototype.”
It is a real file manager with a distinctive product identity.

What keeps it from full-fledged parity is not lack of ambition or even lack of breadth.
It is mostly:
- uneven maturity
- too much behavior concentrated in a few files
- not enough systems-level polishing around operations, search, properties, and consistency

That is a very fixable kind of gap.
