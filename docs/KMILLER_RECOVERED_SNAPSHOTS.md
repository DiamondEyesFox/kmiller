# KMiller Recovered Snapshots

This document tracks the concrete early KMiller source snapshots that have been
materialized from the raw Aug 7 to Aug 30, 2025 ChatGPT genesis conversation.

Recovered source trees live here:
- [recovered-src](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src)

Primary manifest:
- [MANIFEST.md](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/MANIFEST.md)

Related docs:
- [KMILLER_CONVERSATION_ITERATIONS.md](KMILLER_CONVERSATION_ITERATIONS.md)
- [KMILLER_EARLY_FILE_GROUPINGS.md](KMILLER_EARLY_FILE_GROUPINGS.md)
- [KMILLER_GENESIS_SOURCE_MAP.md](KMILLER_GENESIS_SOURCE_MAP.md)

## What Was Materialized

### `proto-genesis-0`
Path:
- [proto-genesis-0](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/proto-genesis-0)

Recovered from:
- `msg 10`
- code blocks `1-4`

Contents:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.cpp`
- `mainwindow.h`

Meaning:
- the first generated KMiller skeleton
- fixed 3-column Miller prototype
- pre-Quick Look

### `proto-quicklook-1`
Path:
- [proto-quicklook-1](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/proto-quicklook-1)

Recovered from:
- inherited from `proto-genesis-0`
- updated with `msg 12` file changes

Contents:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.cpp`
- `mainwindow.h`

Meaning:
- first Quick Look source layer
- still a fixed 3-column prototype
- adds image/text/PDF preview into the monolithic `MainWindow` build

### `proto-media-2`
Path:
- [proto-media-2](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/proto-media-2)

Recovered from:
- inherited from `proto-quicklook-1`
- updated with `msg 14` file changes

Contents:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.cpp`
- `mainwindow.h`

Meaning:
- first media-capable Quick Look source layer
- adds `QtMultimedia`
- extends inline Quick Look to audio/video playback

### `proto-dynamic-columns-3`
Path:
- [proto-dynamic-columns-3](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/proto-dynamic-columns-3)

Recovered from:
- inherited from `proto-media-2`
- updated with `msg 16` file changes

Contents:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.cpp`
- `mainwindow.h`

Meaning:
- first true dynamic-column Miller implementation
- still monolithic
- last important phase before the packaged plain-KMiller full project

### `proto-packaged-kmiller-4`
Path:
- [proto-packaged-kmiller-4](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/proto-packaged-kmiller-4)

Recovered from:
- `msg 22`
- packaged full-project code blocks

Contents:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.cpp`
- `mainwindow.h`

Meaning:
- the first downloadable plain-KMiller project snapshot
- still monolithic, but already includes dynamic columns + inline Quick Look/media behavior

### `KMiller2`
Path:
- [KMiller2](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/KMiller2)

Recovered from:
- `msg 49`
- assistant’s Python zip-builder payload

Contents:
- `CMakeLists.txt`
- `README.md`
- `src/main.cpp`
- `src/MainWindow.h/.cpp`
- `src/TabPage.h/.cpp`
- `src/MillerView.h/.cpp`
- `src/QuickLookDialog.h/.cpp`

Meaning:
- first strong “daily-driver file manager” expansion
- tabs, path bar, view modes, search, and dedicated Quick Look class

### `KMiller3`
Path:
- [KMiller3](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/KMiller3)

Recovered from:
- `msg 54`
- assistant’s Python zip-builder payload

Contents:
- `CMakeLists.txt`
- `README.md`
- `src/main.cpp`
- `src/MainWindow.h/.cpp`
- `src/Pane.h/.cpp`
- `src/MillerView.h/.cpp`
- `src/QuickLookDialog.h/.cpp`
- `src/ThumbCache.h/.cpp`

Meaning:
- first major KDE-native architecture jump
- KIO/KDirModel
- breadcrumb path bar
- pane architecture
- thumbnails
- Open With

### `KMiller3-plus`
Path:
- [KMiller3-plus](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/KMiller3-plus)

Recovered from:
- `msg 99`
- assistant’s Python package-builder payload

Contents:
- `CMakeLists.txt`
- `src/main.cpp`
- `src/MainWindow.h/.cpp`
- `src/Pane.h/.cpp`
- `src/MillerView.h/.cpp`
- `src/QuickLookDialog.h/.cpp`
- `src/ThumbCache.h/.cpp`
- `src/SettingsDialog.h/.cpp`

Meaning:
- first menu/settings-focused polish layer
- introduces `SettingsDialog`
- represents the “menu bar, preferences, arrow-nav tuning, slimmer toolbar” phase

### `KMiller-source-tree-12file`
Path:
- [KMiller-source-tree-12file](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/KMiller-source-tree-12file)

Recovered from:
- chunked file delivery across `msg 130`, `132`, `134`, `136`, `138`

Contents:
- `CMakeLists.txt`
- `src/main.cpp`
- `src/MainWindow.h/.cpp`
- `src/Pane.h/.cpp`
- `src/MillerView.h/.cpp`
- `src/QuickLookDialog.h/.cpp`
- `src/ThumbCache.h/.cpp`
- `src/SettingsDialog.h/.cpp`

Meaning:
- first durable plain-source “no more expired downloads” delivery
- very important because it reflects what was actually handed to you in text, file by file

### `0.3.0-versioned-install-architecture`
Path:
- [0.3.0-versioned-install-architecture](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.3.0-versioned-install-architecture)

Meaning:
- first durable versioned-install / updater / IFW packaging layer

### `0.4.0-auto-versioning`
Path:
- [0.4.0-auto-versioning](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.4.0-auto-versioning)

Meaning:
- first mature auto-versioning and corrected install-path release layer

### `0.4.2-rescue-stable`
Path:
- [0.4.2-rescue-stable](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.4.2-rescue-stable)

Meaning:
- first clearly documented stability anchor in the chat-era release line

### `0.4.6-self-archiving`
Path:
- [0.4.6-self-archiving](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.4.6-self-archiving)

Meaning:
- self-archiving install tree / build-release tooling layer

### `0.4.7-release-experiment`
Path:
- [0.4.7-release-experiment](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.4.7-release-experiment)

Meaning:
- repair/build script state for the first late-night release experiment

## What Is Still Partial

Not yet materialized as full recovered source trees:
- `proto-kmiller-mainline-5`
- the `KMiller3-millerfix` transitional state
- most of the `0.3.0` to `0.5.1` release-engineering era

Why:
- some of those phases are represented as patch/repair flows rather than one
  clean full-project payload
- some were delivered as incremental file batches instead of a single builder script

## Confidence Level

### High-confidence recoveries
- `proto-genesis-0`
- `proto-quicklook-1`
- `proto-media-2`
- `proto-dynamic-columns-3`
- `proto-packaged-kmiller-4`
- `KMiller2`
- `KMiller3`
- `KMiller3-plus`
- `KMiller-source-tree-12file`

Why:
- each came from a coherent code-drop or Python builder payload
- each produced a consistent file manifest

### Medium-confidence historical phases
- most of the pre-`0.3.0` in-between iterations
- many `0.4.x` rescue/build-script states

Why:
- they are historically real, but often survive as:
  - partial patches
  - build commands
  - file replacements
  - repair instructions
  - not always one clean complete snapshot

## Best Next Recoveries

If we continue the archaeology, the highest-value remaining recoveries are:

1. `proto-kmiller-mainline-5`
   - first “make it a normal daily-driver file manager” specification pivot
2. `KMiller3-millerfix`
   - important behavior-tuning layer before the 12-file source-tree delivery
3. `0.3.0` / `0.4.0`
   - first versioned-install architecture snapshots
4. `0.4.2-rescue-stable`
   - first remembered stable anchor
5. `0.4.7-build-release-experiment`
   - first self-archiving release-workflow attempt

## Practical Takeaway

We now have real, inspectable source trees for the most important early
structural milestones of KMiller:

1. first skeleton
2. first Quick Look
3. first media Quick Look
4. first dynamic columns
5. first packaged plain-KMiller
6. first “real file manager” expansion
7. first KDE-native major architecture
8. first menu/settings polish layer
9. first durable chunked plain-source delivery

That is enough to reconstruct the early evolution of the project with much more
confidence than we had before.
