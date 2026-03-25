# KMiller Early File Groupings

This document groups the early Aug 7 to Aug 30, 2025 genesis conversation by
major file families rather than by raw chat order.

It is meant to answer:
- what the main files were in each early iteration
- when files first appeared
- how the project split and reorganized itself over time

References:
- [KMILLER_CONVERSATION_ITERATIONS.md](KMILLER_CONVERSATION_ITERATIONS.md)
- [KMILLER_GENESIS_SOURCE_MAP.md](KMILLER_GENESIS_SOURCE_MAP.md)
- raw extraction dump: `/home/diamondeyesfox/kmiller_genesis_codeblocks.txt`

## Reading Guide
- Pre-SemVer labels like `proto-genesis-0` are synthetic archaeology labels.
- Message and block references refer to the raw Aug 7 genesis conversation.
- Early stages were not always consistent about filename casing:
  - `mainwindow.cpp` / `mainwindow.h`
  - later: `MainWindow.cpp` / `MainWindow.h`

## 1. Build And Packaging Files

### `CMakeLists.txt`
First appears:
- `proto-genesis-0`
- `msg 10`, `BLOCK 1`

Evolution:
1. `proto-genesis-0`
   - `project(KMillerFM LANGUAGES CXX)`
   - tiny Qt/KDE skeleton
   - links `Qt6::Widgets`, `KF6::KIOCore`, `KF6::KIOWidgets`
2. `proto-media-2`
   - adds `Qt6::Multimedia`
   - adds `Poppler::Qt6`
3. `proto-packaged-kmiller-4`
   - packaged full-project version still uses `KMillerFM`
   - now reflects dynamic columns + media Quick Look
4. `KMiller2`
   - described inside the Python zip-builder message
   - project becomes `KMiller2`
   - layout expands into `src/` plus more files
5. `KMiller3`
   - project becomes `KMiller3`
   - KIO/KDirModel-heavy build
6. `0.3.0`
   - first explicit semantic version:
     - `project(KMiller VERSION 0.3.0 LANGUAGES CXX)`
   - versioned install architecture begins
   - `/opt/kmiller/versions/<ver>` enters the project structure

### `README.md`
First clearly appears:
- packaged phases around `proto-packaged-kmiller-4`
- later explicitly embedded in `KMiller2` and `KMiller3` zip-builder messages

What it tracks:
- feature list at each stage
- build instructions
- the user-facing identity of the app

Practical meaning:
- README is one of the best surviving artifacts for understanding what each
  iteration thought it was trying to be

### Packaging / Install Scripts
Appear later:
- `KMiller2` / `KMiller3` zip-builder Python messages
- `0.3.0+` versioned install era

Examples:
- zip-builder scripts in `msg 49`, `msg 54`
- versioned install snippets in `msg 186`
- updater scripts in later `0.3.0` / `0.4.x` messages

## 2. Entry Point

### `main.cpp`
First appears:
- `proto-genesis-0`
- `msg 10`, `BLOCK 4`

Role:
- always thin
- creates `QApplication`
- constructs the main window
- enters `app.exec()`

Evolution:
1. `proto-genesis-0`
   - very small bootstrap
2. `proto-packaged-kmiller-4`
   - still simple, but launches a larger app
3. `KMiller2` and `KMiller3`
   - still an entry-point file, but now fronting a much more modular app
4. `0.4.x`
   - later conversations discuss adding version reporting and About-dialog
     behavior, but those are downstream concerns, not genesis-file changes

Takeaway:
- `main.cpp` is not where the early architectural change happened
- it stayed mostly a stable bootstrap layer while the rest of the app evolved

## 3. Main Window Shell

### `mainwindow.h` / `mainwindow.cpp`
First appears:
- `proto-genesis-0`
- `msg 10`, `BLOCKS 2-3`

This is the original center of the app.

Evolution:
1. `proto-genesis-0`
   - `MainWindow` owns everything
   - 3 hardcoded columns
   - uses `QFileSystemModel`
2. `proto-quicklook-1`
   - Quick Look UI and key handling are added directly into `MainWindow`
3. `proto-media-2`
   - multimedia support still lives directly in `MainWindow`
4. `proto-dynamic-columns-3`
   - dynamic Miller logic is still directly inside `MainWindow`
5. `proto-packaged-kmiller-4`
   - `MainWindow` is still the entire app shell and much of the logic
6. `KMiller2`
   - `MainWindow` remains central, but starts delegating to:
     - `TabPage`
     - `MillerView`
     - `QuickLookDialog`
7. `KMiller3`
   - `MainWindow` becomes a more proper application shell
   - deeper per-pane logic starts moving into `Pane`

Takeaway:
- the earliest KMiller architecture is “everything in MainWindow”
- the long-term trajectory is “MainWindow becomes orchestration, not implementation”

### `MainWindow.h` / `MainWindow.cpp` (capitalized phase)
Clearly present by:
- `KMiller2`
- `KMiller3`

This is more than a naming cleanup:
- it marks the point where KMiller starts looking like a conventional C++ app
  rather than a one-file prototype with accreted features

## 4. View Core And Navigation Logic

### Phase A: hardcoded columns in `mainwindow.cpp`
Used in:
- `proto-genesis-0`
- `proto-quicklook-1`
- `proto-media-2`

Behavior:
- left selection updates middle
- middle selection updates right
- fixed-width/fixed-count Miller-style structure

### Phase B: dynamic columns still inside `mainwindow.cpp`
Used in:
- `proto-dynamic-columns-3`
- `proto-packaged-kmiller-4`

Behavior:
- list of `QListView*`
- list of `QFileSystemModel*`
- selection removes columns to the right and adds a new column

Meaning:
- this is the first true Miller-columns implementation
- but it is still architecturally monolithic

### Phase C: `MillerView`
Clearly present by:
- `KMiller2`
- also strongly present in the later `KMiller-source-tree-12file` dump

Role:
- Miller logic starts getting its own widget/class
- this is a major sign that the app is becoming maintainable

Typical responsibilities:
- per-column list management
- Miller keyboard behavior
- context-menu forwarding
- selection-driven column spawning

### Phase D: `Pane`
Clearly present by:
- `KMiller3`
- later source-tree delivery and fix messages

Role:
- separates:
  - outer file-manager shell
  - per-view/per-folder pane state
  - view switching
  - path bar / toolbar / context menu / Quick Look triggers

Meaning:
- `Pane` is the biggest architectural maturity jump in the whole early history

## 5. Quick Look System

### Phase A: inline Quick Look inside `MainWindow`
Used in:
- `proto-quicklook-1`
- `proto-media-2`
- `proto-dynamic-columns-3`
- `proto-packaged-kmiller-4`

Files involved:
- `mainwindow.h`
- `mainwindow.cpp`

Capabilities:
- images
- text
- PDF
- then audio/video
- preview dialog members live directly in `MainWindow`

### Phase B: dedicated `QuickLookDialog`
Clearly present by:
- `KMiller2`
- definitely present in `KMiller3`
- explicitly present in the 12-file source-tree delivery

Why it matters:
- Quick Look becomes its own subsystem instead of being a feature bolted onto
  the main window
- this is a major step toward the later recognizable KMiller identity

Likely responsibilities:
- file display
- image rendering
- text preview
- PDF preview
- media playback
- close behavior / dialog presentation

Takeaway:
- Quick Look starts as an inline feature
- quickly becomes a first-class component
- that mirrors how important it was to the project identity from the start

## 6. Tabs And Multi-Pane Expansion

### `TabPage`
First clearly appears:
- `KMiller2`

Meaning:
- this is the first sign the app is trying to behave like a full daily-driver
  file manager, not only a Miller demo

Role:
- tab content container
- likely couples one pane/view stack with file-manager-level UI

Relationship to later architecture:
- `TabPage` is part of the middle evolutionary layer
- later, `Pane` becomes the more durable architectural unit

## 7. Thumbnails, Settings, And Other Support Files

### `ThumbCache.h` / `ThumbCache.cpp`
Clearly present by:
- `KMiller3`
- explicitly listed in the later source-tree delivery

Role:
- thumbnail caching helper
- another sign of a real file-manager architecture

### `SettingsDialog.h` / `SettingsDialog.cpp`
Clearly present by:
- later `KMiller3` source-tree delivery
- `msg 138`

Role:
- preferences dialog stub
- proof that the app was already growing into a “real app shell with settings”

## 8. File Families By Major Phase

### `proto-genesis-0`
Files:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.h`
- `mainwindow.cpp`

### `proto-quicklook-1`
Files:
- same 4 files
- Quick Look added inline to `MainWindow`

### `proto-media-2`
Files:
- same 4 files
- media support added inline to `MainWindow`

### `proto-dynamic-columns-3`
Files:
- same 4 files
- dynamic columns replace hardcoded columns

### `proto-packaged-kmiller-4`
Files:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.h`
- `mainwindow.cpp`
- plus packaging/readme structure

### `KMiller2`
Known file families:
- `CMakeLists.txt`
- `README.md`
- `src/main.cpp`
- `src/MainWindow.h`
- `src/MainWindow.cpp`
- `src/TabPage.h`
- `src/TabPage.cpp`
- `src/MillerView.h`
- `src/MillerView.cpp`
- `src/QuickLookDialog.h`
- `src/QuickLookDialog.cpp`

### `KMiller3`
Known file families:
- `CMakeLists.txt`
- `README.md`
- `src/main.cpp`
- `src/MainWindow.h`
- `src/MainWindow.cpp`
- `src/Pane.h`
- `src/Pane.cpp`
- `src/MillerView.h`
- `src/MillerView.cpp`
- `src/QuickLookDialog.h`
- `src/QuickLookDialog.cpp`
- `src/ThumbCache.h`
- `src/ThumbCache.cpp`
- later also:
  - `src/SettingsDialog.h`
  - `src/SettingsDialog.cpp`

### `KMiller-source-tree-12file`
By the later chunked delivery phase, the app is presented as:
- `CMakeLists.txt`
- `src/main.cpp`
- `src/MainWindow.h/.cpp`
- `src/Pane.h/.cpp`
- `src/MillerView.h/.cpp`
- `src/QuickLookDialog.h/.cpp`
- `src/ThumbCache.h/.cpp`
- `src/SettingsDialog.h/.cpp`

## 9. The Most Important Architectural Story

If you ignore naming noise and look only at file structure, the early KMiller
history is:

1. **single-window prototype**
   - everything in `mainwindow.*`
2. **single-window prototype with Quick Look**
   - still monolithic
3. **single-window true Miller prototype**
   - dynamic columns, still monolithic
4. **first packaged product**
   - same architecture, easier to ship
5. **real file manager expansion**
   - `KMiller2`
   - separate `MillerView`, `QuickLookDialog`, tabs
6. **real KDE-native architecture**
   - `KMiller3`
   - `Pane`, `ThumbCache`, settings dialog, KIO-centric design

That is the most useful way to think about the early versions:
- not just by labels
- but by which file families existed and what responsibilities they held

## 10. Best Reconstruction Targets

If we want to recover the early project cleanly, the best source snapshots to
extract are:

1. `proto-genesis-0`
   - minimal origin snapshot
2. `proto-packaged-kmiller-4`
   - first downloadable “plain KMiller”
3. `KMiller2`
   - first full file-manager architecture jump
4. `KMiller3`
   - first mature KDE-native architecture jump

Those four snapshots would capture nearly the entire early structural evolution
of KMiller.
