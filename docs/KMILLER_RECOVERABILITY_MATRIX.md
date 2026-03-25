# KMiller Recoverability Matrix

This document answers a practical archaeology question: which historical KMiller phases survive as exact source trees, which survive as partial overlays, and which survive only as binaries or textual evidence.

## Categories

- **Exact source tree survives**: a concrete source snapshot exists on disk and can be inspected directly.
- **Recovered source tree**: reconstructed from conversation payloads into a coherent source directory.
- **Overlay / patch state**: survives as scripts, file overwrites, install tooling, or partial source patches.
- **Binary only**: installed binary survives, but no matching source tree has yet been found.
- **Text evidence only**: version or phase is documented in chat/logs, but no concrete source or binary artifact has yet been matched.

## Early Proto Lineage

| Phase | Status | Evidence | Notes |
|---|---|---|---|
| `proto-genesis-0` | Recovered source tree | `docs/recovered-src/proto-genesis-0` | First generated KMiller skeleton |
| `proto-quicklook-1` | Recovered source tree | `docs/recovered-src/proto-quicklook-1` | First Quick Look layer |
| `proto-media-2` | Recovered source tree | `docs/recovered-src/proto-media-2` | Media-capable Quick Look |
| `proto-dynamic-columns-3` | Recovered source tree | `docs/recovered-src/proto-dynamic-columns-3` | First dynamic columns layer |
| `proto-packaged-kmiller-4` | Recovered source tree | `docs/recovered-src/proto-packaged-kmiller-4` | First packaged plain KMiller payload |
| `proto-kmiller-mainline-5` | Text evidence only | conversation chronology | Iteration/spec phase; not yet a neat code payload |
| `KMiller2` | Recovered source tree | `docs/recovered-src/KMiller2` | First larger daily-driver architecture |
| `KMiller3` | Recovered source tree | `docs/recovered-src/KMiller3` | First major KDE-native architecture jump |
| `KMiller3-plus` | Recovered source tree | `docs/recovered-src/KMiller3-plus` | Menu/settings polish layer |
| `KMiller-source-tree-12file` | Recovered source tree | `docs/recovered-src/KMiller-source-tree-12file` | First durable chunked source delivery |

## Release-Era Bridge

| Phase | Status | Evidence | Notes |
|---|---|---|---|
| `0.3.0-versioned-install-architecture` | Overlay / patch state | `docs/recovered-src/0.3.0-versioned-install-architecture` | Versioned installs, updater, IFW packaging |
| `0.4.0-auto-versioning` | Overlay / patch state | `docs/recovered-src/0.4.0-auto-versioning` | Auto-versioning and corrected install layout |
| `0.4.2-rescue-stable` | Overlay + recovered files | `docs/recovered-src/0.4.2-rescue-stable` | Includes recovered `Pane.h/.cpp` plus rescue/build flows |
| `0.4.6-self-archiving` | Overlay / patch state | `docs/recovered-src/0.4.6-self-archiving` | Self-archiving install tree tooling |
| `0.4.7-release-experiment` | Overlay / patch state | `docs/recovered-src/0.4.7-release-experiment` | One-shot repair/build script state |
| `0.5.0-patchnotes-release` | Overlay / release-process state | `docs/recovered-src/0.5.0-patchnotes-release` | First patch-notes-centered release tooling |
| `0.5.1-patchnotes-design` | Text/design + partial artifacts | `docs/recovered-src/0.5.1-patchnotes-design` | Per-version patchnotes design phase |

## Installed Binary Archive

### Binary-only survivors
These versions are currently confirmed in `/opt/kmiller/versions`, but only as installed binaries:

- `0.4.0`
- `0.4.1`
- `0.4.2`
- `0.4.4`
- `0.4.5`
- `0.4.6`
- `0.4.7`

Path root:
- [/opt/kmiller/versions](/opt/kmiller/versions)

### Exact source-tree survivor

- `0.5.0` survives as an installed source tree and `CMakeLists.txt` in addition to the binary:
  - [/opt/kmiller/versions/0.5.0](/opt/kmiller/versions/0.5.0)

This makes `0.5.0` the earliest currently confirmed **exact installed source snapshot** in the release era.

## Important Distinction: `KMiller-source-tree-12file` vs `0.5.0`

These are not the same state.

Comparison result:
- `0.5.0` differs from the recovered `KMiller-source-tree-12file` across key files including:
  - `MainWindow.cpp/.h`
  - `MillerView.cpp/.h`
  - `Pane.cpp/.h`
  - `QuickLookDialog.cpp/.h`
  - `main.cpp`

Interpretation:
- `KMiller-source-tree-12file` is an earlier recovered source delivery.
- `0.5.0` is a later installed-source snapshot with additional release-era polish, version-title wiring, preview-pane work, patch-notes integration, and release-tooling alignment.

So the historical bridge is:
- recovered proto/source-delivery arc up through `KMiller-source-tree-12file`
- overlay/patch recovery through `0.3.0`–`0.4.7`
- exact installed source again at `0.5.0`

## Best Current Reconstruction Confidence

### Highest confidence
- `proto-genesis-0`
- `proto-quicklook-1`
- `proto-media-2`
- `proto-dynamic-columns-3`
- `proto-packaged-kmiller-4`
- `KMiller2`
- `KMiller3`
- `KMiller3-plus`
- `KMiller-source-tree-12file`
- `0.5.0`

### Medium confidence
- `0.3.0-versioned-install-architecture`
- `0.4.0-auto-versioning`
- `0.4.2-rescue-stable`
- `0.4.6-self-archiving`
- `0.4.7-release-experiment`
- `0.5.0-patchnotes-release`

### Lower confidence / still partial
- `proto-kmiller-mainline-5`
- `0.5.1-patchnotes-design`
- any exact source state for `0.4.0`, `0.4.1`, `0.4.2`, `0.4.4`, `0.4.5`, `0.4.6`, `0.4.7`

## Practical Conclusion

We now have enough material to say:

- the proto-KMiller era is strongly recoverable as source
- the early release era is strongly recoverable as tooling/overlay history
- `0.5.0` is the first exact release-era source anchor currently confirmed on disk

That means early KMiller is no longer “mostly lost”; it is now a partially reconstructed but increasingly well-grounded historical codebase.
