# KMiller Genesis Source Map

Last updated: 2026-03-06

## Purpose

This document maps the most important early KMiller source milestones to:
- raw ChatGPT conversation message numbers
- extracted code block numbers from `kmiller_genesis_codeblocks.txt`

Primary raw artifact:
- `/home/diamondeyesfox/kmiller_genesis_codeblocks.txt`

Primary conversation:
- raw export conversation `Finder alternatives for Linux`
- id: `68957385-6578-832d-9162-7e565fff2c47`

## High-Level Milestones

### 1. Genesis skeleton

Conversation message:
- `msg 10`

Description:
- first generated KMiller code
- fixed 3-column Miller prototype
- project name `KMillerFM`
- executable `kmiller`

Key extracted blocks:
- `BLOCK 1` — first `CMakeLists.txt`
- `BLOCK 2` — first `mainwindow.h`
- `BLOCK 3` — first `mainwindow.cpp`
- `BLOCK 4` — first `main.cpp`
- `BLOCK 5` — first build command

## 2. Quick Look prototype

Conversation message:
- `msg 12`

Description:
- adds image/text/PDF Quick Look
- first floating preview dialog

Key extracted blocks:
- `BLOCK 6` — Quick Look header update
- `BLOCK 7` — Quick Look implementation
- `BLOCK 8` — Quick Look build command

## 3. Media-capable Quick Look prototype

Conversation message:
- `msg 14`

Description:
- adds audio/video playback via `QtMultimedia`
- first “full Quick Look clone” attempt

Key extracted blocks:
- `BLOCK 9` — media-enabled `CMakeLists.txt`
- `BLOCK 10` — media-enabled header
- `BLOCK 11` — media-enabled implementation
- `BLOCK 12` — media-enabled build/install command

## 4. Dynamic-columns prototype

Conversation message:
- `msg 16`

Description:
- removes the fixed 3-column limitation
- pushes toward true Finder-style expanding columns

Key extracted blocks:
- `BLOCK 13` — dynamic-columns header
- `BLOCK 14` — dynamic-columns implementation

## 5. First packaged full project

Conversation message:
- `msg 22`

Description:
- first whole-project handoff
- dynamic Miller columns + Quick Look
- packaged as a complete project folder instead of just snippets

Key extracted blocks:
- `BLOCK 16` — project tree layout
- `BLOCK 17` — packaged project `CMakeLists.txt`
- `BLOCK 18` — packaged header
- `BLOCK 19` — packaged implementation
- `BLOCK 20` — packaged `main.cpp`
- `BLOCK 21` — packaged build command

Related packaging/setup:
- `BLOCK 22` — `git init`
- `BLOCK 23` — build command
- `BLOCKS 24-31` — base64/zip delivery flow

## 6. KMiller2 phase

Earliest conversation messages:
- `msg 49`
- `msg 50`
- `msg 52`

Description:
- “more full-featured” file manager phase
- introduces the idea of a more normal daily-driver FM around Miller + Quick Look

Features referenced in the conversation:
- icons/list/compact + Miller view
- places sidebar
- path bar
- tabs
- search filter
- sorting
- Quick Look

Key extracted blocks:
- `BLOCK 34` — KMiller2 build/install command
- `BLOCK 35` — KMiller2 build/install command variant

## 7. KMiller3 phase

Earliest conversation messages:
- `msg 54`
- `msg 55`

Description:
- KDE-native ambition jump
- first strong KIO / KDirModel phase

Features referenced in the conversation:
- KIO / KDirModel
- breadcrumb path bar
- split view
- zoom slider
- undo/redo
- trash
- Open With
- thumbnails
- Miller Columns + Quick Look

Key extracted blocks:
- `BLOCK 36` — initial KMiller3 build command
- `BLOCK 37` — KMiller3 build command variant
- `BLOCK 82` — KMiller3 `CMakeLists.txt`

Later KMiller3 repair/build flow blocks:
- `BLOCKS 38-81`
- these include repeated rebuild, unzip, dependency, and patch commands

## 8. First explicit SemVer phase (`0.3.0`)

Earliest conversation message:
- `msg 186`

Description:
- versioned install system becomes explicit
- `/opt/kmiller/versions/<version>/bin/kmiller`
- updater/packaging flow starts hardening

Key extracted blocks:
- `BLOCK 150` — first `0.3.0` versioned `CMakeLists` fragment
- `BLOCK 151` — updater script
- `BLOCK 152` — install updater command
- `BLOCK 153` — first `0.4.0` build/update path from the same phase
- `BLOCKS 155-162` — packaging / desktop file / installer pieces

## 9. First explicit `0.4.0` phase

Earliest conversation message:
- `msg 190`

Description:
- first clean tagged/build-oriented `0.4.0` workflow in the conversation

Key extracted blocks:
- `BLOCK 167` — early versioned `CMakeLists`
- `BLOCK 169` — clean `CMakeLists.txt`
- `BLOCK 170` — `git tag -a v0.4.0`
- `BLOCK 174` — desktop entry
- `BLOCK 175` — large clean `CMakeLists.txt` in the next message (`msg 192`)

## Important Observation

The raw code-block dump contains:
- `447` total fenced code blocks
- about `390` unique blocks by body hash

So this is not a neat sequence of fully distinct versions.
It is:
- a mix of source snapshots
- repairs
- rebuild commands
- packaging snippets
- desktop files
- updater scripts

That means the best recovery strategy is:
1. use this map to identify milestone blocks
2. extract those blocks into curated per-phase docs
3. reconstruct the source snapshots phase-by-phase

## Recommended Next Docs

The next most useful archaeology deliverables would be:
1. `KMILLER_GENESIS_SKELETON.md`
2. `KMILLER_FIRST_QUICKLOOK.md`
3. `KMILLER2_RECOVERY.md`
4. `KMILLER3_RECOVERY.md`
