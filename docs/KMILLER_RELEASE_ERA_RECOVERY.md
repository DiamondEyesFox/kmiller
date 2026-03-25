# KMiller Release-Era Recovery

This document bridges the recovered proto-KMiller snapshots into the first release-engineering era. Unlike the earlier proto snapshots, these milestones often survive as patch flows, install scripts, and one-shot repair blocks rather than one neat full-project payload.

## Recovered Release-Era Artifacts

### `0.3.0-versioned-install-architecture`
Path:
- [0.3.0-versioned-install-architecture](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.3.0-versioned-install-architecture)

Primary source messages:
- `msg 186`
- `msg 188`

What survives:
- early versioned-install `CMakeLists` snippets
- first `kmiller-update.sh` updater
- Qt Installer Framework packaging layout
- `config.xml`, `package.xml`, `installscript.qs`, `make_installer.sh`
- sample build/install flow into `/opt/kmiller/versions/<ver>/bin/kmiller`

Meaning:
- first durable versioned-install architecture
- first installer/updater split

### `0.4.0-auto-versioning`
Path:
- [0.4.0-auto-versioning](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.4.0-auto-versioning)

Primary source messages:
- `msg 190`
- `msg 192`

What survives:
- corrected install-path snippet
- auto-versioning `CMakeLists`
- updated updater script
- command examples for `0.4.0`, `0.4.1`, and `0.4.2`
- full clean release-oriented `CMakeLists.txt`

Meaning:
- first mature auto-versioning/install-layout layer
- `.desktop` path correction and git/env/CLI version detection

### `0.4.2-rescue-stable`
Path:
- [0.4.2-rescue-stable](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.4.2-rescue-stable)

Primary source messages:
- `msg 298`
- `msg 301`
- `msg 303`
- `msg 330`

What survives:
- full recovered `src/Pane.h` and `src/Pane.cpp`
- rebuild/install rescue scripts
- git-init / backup / tag flow
- titlebar/version patches

Meaning:
- first clearly remembered stabilization checkpoint
- `v0.4.2` becomes the known-good anchor in the conversation

### `0.4.6-self-archiving`
Path:
- [0.4.6-self-archiving](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.4.6-self-archiving)

Primary source message:
- `msg 334`

What survives:
- `tools/build_release.sh`
- full patch/build block
- textual description of the self-archiving install tree

Meaning:
- install tree starts preserving source alongside binaries in `/opt/kmiller/versions/<ver>/`

### `0.4.7-release-experiment`
Path:
- [0.4.7-release-experiment](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.4.7-release-experiment)

Primary source message:
- `msg 344`

What survives:
- one-shot repair/build script
- version-title / compile-definition adjustments
- Miller context-menu repair attempt

Meaning:
- late-night release push phase
- survives more as an operational script than a clean source tree

### `0.5.0-patchnotes-release`
Path:
- [0.5.0-patchnotes-release](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.5.0-patchnotes-release)

Primary source messages:
- `msg 220`
- `msg 224`

What survives:
- first `make_patchnotes.sh`
- first `build_release.sh` patch-notes workflow
- quiet/gentler follow-up wrapper variant

Meaning:
- KMiller starts behaving like a release process, not just a versioned install
- patch notes become part of the release artifact story

### `0.5.1-patchnotes-design`
Path:
- [0.5.1-patchnotes-design](/mnt/steamssd/Documents/Development/kmiller/docs/recovered-src/0.5.1-patchnotes-design)

Primary source message:
- `msg 248`

What survives:
- target install layout for per-version patch notes
- sample summary / change bullets / diff block
- release command example

Meaning:
- patch notes get promoted from helper script output into a designed per-version deliverable

## Interpretation

The release era is materially different from the proto-KMiller era:
- proto era survives as whole project payloads
- release era survives mostly as install architecture, build tools, and targeted file overwrites

These artifacts are trustworthy reconstruction aids, but not every directory here is a standalone runnable full project.

## Best Current Bridge

The reconstructed historical line now looks like:

1. proto-genesis-0
2. proto-quicklook-1
3. proto-media-2
4. proto-dynamic-columns-3
5. proto-packaged-kmiller-4
6. KMiller2
7. KMiller3
8. KMiller3-plus
9. KMiller-source-tree-12file
10. 0.3.0-versioned-install-architecture
11. 0.4.0-auto-versioning
12. 0.4.2-rescue-stable
13. 0.4.6-self-archiving
14. 0.4.7-release-experiment
15. 0.5.0-patchnotes-release
16. 0.5.1-patchnotes-design

That gives us a continuous reconstructed story from the genesis prompt into the first recognizable release line and early release-process era.
