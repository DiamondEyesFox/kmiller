# KMiller Genesis Conversation Iterations

Source conversation:
- Title: `Finder alternatives for Linux`
- Conversation ID: `68957385-6578-832d-9162-7e565fff2c47`
- Export source: ChatGPT raw export split file `conversations-024.json`
- Total chronological messages with text: `380`

## Notes
- Early KMiller did not use consistent version numbers.
- To make the conversation usable as history, this document assigns synthetic archaeology version labels to the pre-SemVer phases.
- These labels are for reconstruction and discussion only; they are not claimed as original release names unless explicitly stated in the conversation.
- Once the conversation begins using explicit versions like `0.3.0`, `0.4.0`, etc., those real version numbers are used.

## Iteration Map

### `proto-genesis-0`
- Messages: `1-10`
- Time: `2025-08-07 22:48:48` to `22:51:18`
- Original naming state:
  - concept only until msg 10
  - code names: `KMillerFM` project, `kmiller` executable
- What happens:
  - user says to create the Finder-like file manager
  - KDE is chosen as the platform
  - first starter skeleton is generated
- Core shape:
  - fixed 3-column Miller-style file browser
  - Qt/KDE-flavored prototype
  - not yet a full file manager
- Main files present in the first code drop:
  - `CMakeLists.txt`
  - `mainwindow.h`
  - `mainwindow.cpp`
  - `main.cpp`

### `proto-quicklook-1`
- Messages: `11-12`
- Time: `2025-08-07 22:51:38`
- What changes:
  - first Quick Look support is added
- Feature jump:
  - images
  - text
  - PDFs
  - floating preview dialog
  - `Space` / `Escape`
- Significance:
  - this is the first moment KMiller starts to feel like the app you wanted rather than just a Miller demo

### `proto-media-2`
- Messages: `13-14`
- Time: `2025-08-07 22:51:59`
- What changes:
  - Quick Look expands to audio/video playback
- Significance:
  - this is the first truly ambitious Finder-style Quick Look phase

### `proto-dynamic-columns-3`
- Messages: `15-16`
- Time: `2025-08-07 22:52:28`
- What changes:
  - fixed 3-column layout is replaced by dynamic columns
- Significance:
  - this is the first version aiming at true Finder-style Miller behavior rather than a static mockup

### `proto-packaged-kmiller-4`
- Messages: `17-41`
- Time: `2025-08-07 22:53:21` to `22:58:04`
- What changes:
  - project shifts from snippets to a full packaged project
  - assistant creates zip/base64/repo-style packaging
- Naming state:
  - plain `KMiller` becomes the practical product identity here
- Artifacts described:
  - `kmiller/`
  - `kmiller.zip`
  - `kmiller.b64`
- Significance:
  - this is the first downloadable KMiller era

### `proto-kmiller-mainline-5`
- Messages: `42-48`
- Time: `2025-08-07 23:01:52` to `23:04:22`
- What changes:
  - user asks for a normal daily-driver file manager, not only Miller view
  - requirements expand toward full parity with mainstream file managers
- Features requested:
  - normal view modes
  - path bar
  - tabs
  - expected file manager behavior
- Significance:
  - this is the architecture/spec pivot that leads directly to KMiller2

### `KMiller2`
- Messages: `49-52`
- Time: `2025-08-07 23:04:23` to `23:06:44`
- First explicit named milestone after plain packaged KMiller
- What changes:
  - updated downloadable build with broader real-FM features
- Features described:
  - icons/list/compact + Miller view
  - places sidebar
  - path bar
  - tabs
  - search filter
  - sorting
  - Quick Look retained
- Significance:
  - first clear full file manager plus Miller Columns milestone

### `KMiller3`
- Messages: `53-55`
- Time: `2025-08-07 23:07:59` to `23:10:59`
- What changes:
  - KDE integration ambition jumps sharply
- Features described:
  - KIO / KDirModel
  - breadcrumb path bar
  - split view
  - zoom slider
  - undo/redo
  - trash
  - Open With
  - thumbnails
  - Miller Columns + Quick Look
- Significance:
  - first clearly KDE-native, Finder-aspirational major build

### `KMiller3-rehydration`
- Messages: `56-97`
- Time: `2025-08-28 19:47:16` to `20:26:49`
- What changes:
  - old download link expires
  - KMiller3 is rebuilt/reprovided
  - user attempts first real local build on Arch/KDE
  - many dependency and KF6 API fixes are applied
- Technical themes:
  - package naming on Arch
  - KF6 API mismatches
  - `KUrlNavigator` signal differences
  - Poppler API changes
- Significance:
  - this is the first strong real-machine porting and compile surgery phase

### `KMiller3-plus`
- Messages: `98-105`
- Time: `2025-08-28 20:32:02` to `20:40:12`
- What changes:
  - first user-driven UX polish wave on the running app
- Requested features:
  - menu bar
  - preferences/settings menu
  - arrow-key behavior adjustments
  - less bulky toolbar feel
- Significance:
  - first live usability tuning after a successful local build

### `KMiller3-quicklook-wired`
- Messages: `101-106`
- Time: `2025-08-28 20:36:07` to `20:43:34`
- What changes:
  - Quick Look is explicitly wired to the app instead of only assumed to exist internally
- Features added:
  - `Space`
  - menu item
  - right-click context menu action
- Significance:
  - this is the first iteration where Quick Look is treated like a fully exposed user feature in the UI

### `KMiller3-millerfix`
- Messages: `107-126`
- Time: `2025-08-28 20:53:25` to `2025-08-29 06:49:28`
- What changes:
  - interaction model fixes for actual use
- Main fixes pursued:
  - right-click context menu
  - double-click should open instead of rename
  - arrow-key behavior for Miller view only
  - Miller selection/navigation behavior
- Naming state:
  - assistant explicitly calls the package `KMiller3 MillerFix`
- Significance:
  - first serious attempt to lock Finder-like interaction details

### `KMiller-source-tree-12file`
- Messages: `127-146`
- Time: `2025-08-29 06:51:52` to `07:11:17`
- What changes:
  - because expiring zip links are unreliable, the project is delivered as a full source tree in chunks
- Important state:
  - app becomes 12 files total in the user’s mental model
  - user renames the project back toward plain `KMiller`
- Significance:
  - this is the first durable textual source-tree delivery phase

### `KMiller-local-install-presemver`
- Messages: `147-185`
- Time: `2025-08-29 19:05:41` to `19:52:57`
- What changes:
  - user builds and runs the plain local source tree as `KMiller`
  - rapid bug-fix loop for Miller behavior and Quick Look
- Main fix themes:
  - incomplete `Pane.cpp`
  - missing includes
  - right/enter navigation
  - single-click behavior
  - Quick Look in Miller
  - crash guard when clicking files
  - double-click rename regressions
  - startup crash from recent Miller patches
- Significance:
  - this is the first real local app hardening phase before explicit semantic versioning starts

### `0.3.0`
- Messages: `186-189`
- Time: `2025-08-29 19:53:14` to `20:06:56`
- First explicit SemVer milestone in the conversation
- What changes:
  - versioned installs system is introduced
  - `/opt/kmiller/versions/<version>` becomes part of the architecture
- Significance:
  - this is the first real release-management iteration, not just feature work

### `0.4.0`
- Messages: `190-198`
- Time: `2025-08-29 20:10:30` to `20:21:43`
- What changes:
  - versioned install flow is working
  - launcher and desktop integration become first-class concerns
- New concerns:
  - `.desktop` location
  - `/usr/local/bin/kmiller`
  - active-version symlink strategy
- Significance:
  - first installable version with recognizable release/launcher management

### `0.4.x-rightclick-history-loop`
- Messages: `199-217`
- Time: `2025-08-29 20:26:37` to `21:07:12`
- What changes:
  - right-click crash and growing-Miller issues become major focus
  - install/launcher freshness becomes a repeated concern
- Main problems:
  - right-click crash
  - previous-column mouse behavior
  - launcher freshness confusion
- Significance:
  - first strong sign that the release/install system and the code evolution were drifting apart

### `0.4.x-patchnotes-attempt`
- Messages: `217-257`
- Time: `2025-08-29 21:07:06` to `22:29:23`
- What changes:
  - patch notes become a major architectural goal
  - both git-based and non-git patch note systems are attempted
- Main themes:
  - natural-language patch notes
  - archive/latest symlinks
  - generator scripts
  - Yakuake/terminal crashes from large scripts
- Significance:
  - first release-engineering and documentation-system phase

### `0.4.0-revert-crisis`
- Messages: `258-302`
- Time: `2025-08-29 22:29:26` to `23:27:21`
- What changes:
  - multiple attempted rollbacks to good 0.4.x builds
  - user discovers the good snapshot may already have been overwritten
  - installed version vs source snapshot confusion becomes severe
- Key explicit versions discussed:
  - `0.4.0`
  - `0.4.1`
  - `0.4.2`
- Important turning point:
  - by msg 302, the user reports the latest rescue version worked and fixed everything
- Significance:
  - this is the first full version-identity crisis era

### `0.4.2-rescue-stable`
- Messages: `300-307`
- Time: `2025-08-29 23:23:26` to `23:43:27`
- Naming nuance:
  - `0.4.2-rescue` is mentioned, but CMake rejects it as invalid SemVer
  - functionally this era stabilizes around plain `0.4.2`
- What changes:
  - user confirms a rescue build fixed the big regressions
  - discussion turns to backups and preservation
- Significance:
  - first clearly re-stabilized daily-driver milestone after the rollback chaos

### `0.4.2-preview-multiselect`
- Messages: `306-319`
- Time: `2025-08-29 23:43:23` to `2025-08-30 00:07:47`
- What changes:
  - preview pane attempt
  - Finder-like multiselect behavior effort
  - Quick Look gets close button / Space toggle requests
- Main themes:
  - preview pane
  - Ctrl-click / Shift-click / Ctrl-A
  - Finder-style selection semantics
  - Quick Look close behavior
- Significance:
  - this is the first parity-expansion wave after a stable base is regained

### `0.4.7-build-release-experiment`
- Messages: `320-374`
- Time: `2025-08-30 00:14:37` to `01:38:29`
- What changes:
  - self-archiving install tree is attempted
  - build/release scripts are introduced
  - `0.4.7` becomes the target for a more formalized release process
- Main themes:
  - install sources alongside binaries
  - preserve sources per version under `/opt/kmiller/versions/<ver>`
  - `tools/build_release.sh`
  - large one-shot repair scripts
  - repeated `Pane.*` / `MainWindow.cpp` surgery
  - detached/background builds to avoid terminal crashes
- Why it matters:
  - this is the first attempt to turn KMiller into a repeatable release workflow, not just a patched local app
- Outcome:
  - the attempt becomes unstable and repeatedly collapses into repair loops

### `0.4.2-fallback-night-end`
- Messages: `374-376`
- Time: `2025-08-30 01:38:29` to `01:43:10`
- What changes:
  - conversation explicitly gives up on the unstable path and returns everything to `0.4.2`
- Significance:
  - this is the call it a night, restore the good version checkpoint

### `0.4.2-about-dialog-followup`
- Messages: `377-380`
- Time: `2025-08-30 18:29:54` to `18:32:31`
- What changes:
  - user wants a reliable version display
  - `--version` behavior is unsatisfactory
  - About dialog is proposed as the clean solution
- Significance:
  - this is the next planned iteration, but the conversation ends before a later explicit milestone is established

## High-Level Evolution
1. Desire becomes code: the app begins as a direct answer to `create it`.
2. Quick Look appears almost immediately: preview is part of KMiller’s identity from the first few minutes.
3. Normal file manager parity becomes the next big jump: this produces `KMiller2` and `KMiller3`.
4. Real local builds begin later: the conversation shifts from designing to surviving actual KF6/Arch compilation.
5. Behavior tuning dominates: Miller navigation, right-click, double-click, and Quick Look behavior become the core product work.
6. Release engineering arrives abruptly: versioned installs, patch notes, and self-archiving builds appear before the project tooling is truly stable.
7. `0.4.2` becomes the first remembered stable anchor: the conversation repeatedly falls back to it.

## Most Important Practical Milestones
- First creation prompt: `msg 1-5`
- First literal `kmiller` code naming: `msg 10`
- First downloadable full project: `msg 22`
- First real file manager expansion: `KMiller2` in `msg 49-52`
- First KDE-native major leap: `KMiller3` in `msg 54-55`
- First explicit SemVer: `0.3.0` in `msg 186`
- First working release-management architecture: `0.4.0` in `msg 190-198`
- First stable remembered rescue point: `0.4.2` / `0.4.2-rescue`
