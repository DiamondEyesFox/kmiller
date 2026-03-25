# KMiller Pre-0.3.0 Lineage

Last updated: 2026-03-06

## Purpose

This document captures the phase between the first generated KMiller prototype and the first explicit `0.3.0` version mention.

It is based on the raw ChatGPT export conversation:
- title: `Finder alternatives for Linux`
- id: `68957385-6578-832d-9162-7e565fff2c47`

## Short Answer

Between the genesis prototype and `0.3.0`, KMiller goes through a real pre-SemVer lineage:

1. genesis skeleton
2. Quick Look prototype
3. media-capable Quick Look prototype
4. dynamic-columns prototype
5. full-project packaging phase
6. `KMiller2`
7. `KMiller3`
8. then explicit numeric versions begin with `0.3.0`

## Phase Timeline

### 1. Genesis skeleton

First surviving literal naming moment:
- 2025-08-07 22:51:18 CDT

What it was:
- project name in code: `KMillerFM`
- executable name: `kmiller`
- three fixed Miller-style columns
- `QListView` + `QFileSystemModel`

What survives:
- raw source blocks in the genesis conversation

### 2. Quick Look prototype

Immediately after the skeleton, the next phase adds:
- image preview
- text preview
- PDF preview
- floating preview dialog
- `Space` / `Escape`

What survives:
- raw source blocks in the genesis conversation

### 3. Media-capable Quick Look prototype

Next, the same conversation adds:
- audio playback
- video playback
- `QtMultimedia`

This is the point where the app starts feeling recognizably like KMiller, not just a column browser.

What survives:
- raw source blocks in the genesis conversation

### 4. Dynamic-columns prototype

Next phase:
- removes the fixed 3-column limitation
- starts moving toward true Finder-style expanding Miller behavior

What survives:
- raw source blocks in the genesis conversation

### 5. Full-project packaging phase

After the iterative code blocks, the conversation shifts into:
- “give me the whole program”
- full-project folder layouts
- zips
- base64 packaging
- ready-to-build project delivery

This is where the project stops being “a code snippet” and starts becoming “a handoffable app.”

What survives:
- raw source blocks
- build commands
- packaging instructions

### 6. KMiller2

Earliest clear `KMiller2` phase:
- 2025-08-07 23:04:23 CDT

Associated features:
- icons/list/compact views plus Miller view
- places sidebar
- path bar
- tabs
- search filter
- sorting
- Quick Look

Interpretation:
- this is the first “closer to a normal file manager” milestone

What survives:
- build/package references in the conversation
- code blocks and packaging instructions

### 7. KMiller3

Earliest clear `KMiller3` phase:
- 2025-08-07 23:08:00 CDT

Associated features:
- KIO / KDirModel push
- breadcrumb path bar
- split view
- zoom slider
- undo/redo
- trash
- Open With
- thumbnails
- Miller Columns + Quick Look

Interpretation:
- this is the first major KDE-native ambition jump
- it is the last major named pre-SemVer phase

What survives:
- repeated package/build blocks
- later rebuild instructions
- backups like `kmiller3.zip` and `kmiller3_quicklook.zip`

### 8. Transition into numeric versions

First explicit `0.3.0` mention:
- 2025-08-29 19:53:14 CDT

After that, the conversation contains explicit mentions of:
- `0.3.0`
- `0.4.0`
- `0.4.1`
- `0.4.2`
- `0.4.3`
- `0.4.4`
- `0.4.5`
- `0.4.6`
- `0.4.7`
- `0.5.0`
- `0.5.1`

## Important Conclusion

The gap between genesis and `0.3.0` is not empty.

It contains a meaningful pre-SemVer lineage:
- the first generated skeleton
- the first true Quick Look identity
- the shift to full-project packaging
- the `KMiller2` phase
- the `KMiller3` phase

So when reconstructing early history, the right order is:

1. genesis skeleton
2. Quick Look prototype
3. media prototype
4. dynamic columns prototype
5. packaged full project
6. `KMiller2`
7. `KMiller3`
8. `0.3.0+`

## Were There Versions Between Genesis And KMiller2?

Yes, but not as distinct surviving **named** versions.

After checking the raw conversation messages and the extracted code blocks, the phase between the first packaged project and `KMiller2` looks like this:

- `msg 22`: first packaged full project
- `msg 24-41`: repo/zip/base64 delivery workflow
- `msg 46-47`: redesign spec for “everything expected in a normal file manager”
- `msg 49-50`: first explicit updated build that becomes `KMiller2`

What survives in that gap:
- `kmiller/`
- `kmiller.zip`
- `kmiller.b64`

What does **not** survive as a distinct name in that gap:
- no `KMiller1`
- no `KMiller1.5`
- no other clearly labeled intermediate project name

So the most accurate reading is:
- there were multiple intermediate iterations
- but they appear to have remained plain **KMiller** until the first explicit `KMiller2` label

In other words:
- **more iterations**: yes
- **more surviving named versions**: no clear evidence so far

## Artifact Pointers

Useful sources for this phase:
- raw ChatGPT export conversation
- `/home/diamondeyesfox/kmiller_genesis_codeblocks.txt`
- backup artifacts:
  - `Documents/Development/kmiller.backup-20260306-053920/Broken Kmiller Builds and Backups/kmiller3.zip`
  - `Documents/Development/kmiller.backup-20260306-053920/Broken Kmiller Builds and Backups/kmiller3_quicklook.zip`

## Recommended Next Step

If we want a real archaeology map, the next task is:
- dedupe the 447 raw code blocks into unique source snapshots
- then assign each unique snapshot to one of the phases above
