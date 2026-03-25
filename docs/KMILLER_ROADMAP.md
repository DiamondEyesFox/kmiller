# KMiller Roadmap

Last updated: 2026-03-06

## Goal

Make KMiller a solid, genuinely usable daily-driver file manager that can honestly compete with Dolphin and Thunar while keeping its own identity:
- Finder-like feel
- true Miller columns
- strong Quick Look
- KDE-native integration

This roadmap is not just about adding features.
It is about reaching:
- reliability
- consistency
- maintainability
- workflow confidence

## Product Standard

KMiller should eventually be good enough that a normal user can choose it as their main file manager because it is:
- fast enough
- stable enough
- visually coherent enough
- behaviorally predictable enough
- capable enough for everyday file work

That means it must compete in three categories:
1. **Core file-manager competence**
2. **Finder-like differentiated UX**
3. **Project maturity and trustworthiness**

## Current Position

KMiller already has:
- tabs
- multiple view modes
- Miller columns
- breadcrumb path bar
- Quick Look
- preview pane
- context menus
- Open With
- file operations
- properties
- settings
- a portal/file-chooser direction
- release/install/version discipline

What it lacks is mostly:
- deeper polish
- consistency across views
- mature operations UX
- maintainable internals
- stronger test coverage

## Strategic Priorities

### Priority 1: Daily-Driver Reliability
This is the non-negotiable foundation.

Targets:
- no obvious regressions in navigation, selection, path bar, Quick Look, or file operations
- no visually broken dialogs or hidden child windows
- no stale-state bugs between path bar, history, selection, preview, and Quick Look
- no surprising view-specific behavior differences unless intentional

Success criteria:
- a week of normal personal use without major workflow-breaking bugs
- release-gate checklist passes consistently

### Priority 2: Core Operations Maturity
KMiller must feel trustworthy with real files.

Targets:
- better copy/move/trash/delete UX
- clearer conflict handling
- visible operation progress
- better failure reporting
- more coherent duplicate/compress/extract behavior

Success criteria:
- common file operations feel predictable and safe
- long-running actions are visible and understandable

### Priority 3: Finder-Like UX Leadership
This is where KMiller should stand out.

Targets:
- best-in-class Miller columns on Linux
- excellent Quick Look
- strong preview pane
- thoughtful breadcrumb/path interactions
- polished rename/open/selection behavior

Success criteria:
- the app feels intentional and distinct, not like Dolphin with a Miller mode bolted on

### Priority 4: Architecture and Maintainability
The app has to stay editable as it grows.

Targets:
- split oversized modules
- reduce duplicated interaction logic
- separate UI wiring from file-operation logic
- reduce fragile coupling between path bar, navigation history, and selection state

Success criteria:
- major interaction changes no longer require risky surgery in one giant file

### Priority 5: Test and Release Confidence
Competing with Dolphin/Thunar requires trust.

Targets:
- add repeatable regression checks for the trickiest behavior
- add stronger smoke/integration coverage
- release notes and stable release rituals stay clean

Success criteria:
- fewer “works in source, broken in installed app” situations
- faster confidence when shipping changes

## Roadmap Phases

## Phase 1: Stabilize The Core

### Goals
- eliminate the current class of brittle regressions
- make existing features feel solid
- reduce “one fix breaks another” behavior

### Main work
- refactor [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp) into smaller subsystems
- separate navigation/history/pathbar state from preview/Quick Look state
- unify selection/open/rename/context-menu semantics across all views
- harden dialog presentation and focus behavior
- make settings that exist in UI actually take effect consistently

### Deliverables
- extracted modules or helper classes for:
  - navigation state
  - context-menu / file actions
  - preview handling
  - Open With handling
- fewer behavior branches duplicated between classic views and Miller view
- stable dialog behavior for all child windows

### Exit criteria
- path bar, Go to Folder, Quick Look, Open With, and context menus stop regressing each other
- all current major visual parity checks pass

## Phase 2: Make File Operations Trustworthy

### Goals
- make KMiller feel safe for real work
- improve the “competent file manager” side of the product

### Main work
- deepen [FileOpsService.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileOpsService.cpp) into a true operations layer
- add operation progress and better user feedback
- handle collisions/overwrites more thoughtfully
- standardize copy/move/trash/delete/duplicate/archive behavior
- improve undo/redo strategy or, at minimum, operation history groundwork

### Deliverables
- centralized file-operation logic
- visible progress / errors / completion feedback
- coherent archive and duplicate workflows

### Exit criteria
- file operations feel as trustworthy as the rest of the UI vision
- user can move, copy, duplicate, trash, and extract without anxiety

## Phase 3: Push Finder-Like UX From Good To Great

### Goals
- make KMiller feel like the best Linux home for this workflow
- strengthen its differentiated identity

### Main work
- deepen Quick Look polish
- improve preview pane fidelity and consistency
- improve breadcrumb/path interactions and discoverability
- tune Miller keyboard and mouse behavior until it feels truly effortless
- polish theming and visual coherence across dialogs, panes, and preview surfaces

### Deliverables
- cleaner Quick Look behavior for all major file types
- more polished preview-pane and breadcrumb UX
- fewer visual inconsistencies between app subsystems

### Exit criteria
- KMiller is not merely “feature complete,” it is notably pleasant to use

## Phase 4: Close Mainstream File-Manager Gaps

### Goals
- reduce the remaining reasons someone would fall back to Dolphin or Thunar

### Main work
- improve search beyond simple filtering
- improve properties and metadata depth
- improve Open With / app association handling
- improve mounted-volume and external-location handling
- improve terminal/external-tool integration
- harden file chooser / portal behavior

### Deliverables
- richer search behavior
- stronger properties dialog
- more mature Open With/default-app workflow
- better ecosystem integration

### Exit criteria
- most mainstream everyday tasks no longer feel like “the part KMiller isn’t good at yet”

## Phase 5: Release Quality And Trust

### Goals
- make releases feel boring in the best way
- ensure the project can grow without archaeology becoming necessary again

### Main work
- add interaction regression tests for path bar, Quick Look, and Miller navigation
- add operation-focused smoke/integration tests
- keep release docs, patch notes, and versioned installs clean
- formalize release checklist and acceptance criteria

### Deliverables
- stronger tests
- reliable release checklist
- clearer branch/tag/release discipline

### Exit criteria
- shipping a release feels routine, not risky

## Theme Of Work By Area

### Core code health
Top priority files:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)
- [MainWindow.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MainWindow.cpp)

### Operations maturity
Top priority files:
- [FileOpsService.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileOpsService.cpp)
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [PropertiesDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/PropertiesDialog.cpp)

### Finder-like UX leadership
Top priority files:
- [QuickLookDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/QuickLookDialog.cpp)
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

### Ecosystem integration
Top priority files:
- [FileChooserDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserDialog.cpp)
- [FileChooserPortal.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserPortal.cpp)
- [CMakeLists.txt](/mnt/steamssd/Documents/Development/kmiller/CMakeLists.txt)

## What “Competitive With Dolphin And Thunar” Means

KMiller does **not** need to copy those apps exactly.
It needs to match them where they are strong enough that users trust them:
- reliability
- predictable file operations
- stable navigation
- clear status and feedback
- broad enough compatibility for normal workflows

Then it should beat them where KMiller is special:
- Miller columns
- Quick Look
- preview-first workflow
- Finder-like interaction feel

That is the winning strategy.

## Immediate Next Milestones

### Milestone A: Stability Sweep
- refactor the riskiest state coupling in `Pane`
- preserve current working fixes
- eliminate known regressions between path bar, Quick Look, and dialogs

### Milestone B: Operations Pass
- improve progress/error/collision handling
- make copy/move/trash/delete feel more mature

### Milestone C: Open With / Properties / Search Pass
- harden the practical “daily use” rough edges

### Milestone D: Regression Safety Net
- add tests for the behaviors that keep breaking

## Recommended Order

1. stabilize core interactions
2. strengthen file operations
3. polish Finder-like differentiators
4. close everyday parity gaps
5. harden release/testing workflow

## Bottom Line

KMiller does not need a total reinvention.

It already has the hard thing:
- a real identity
- a real architecture
- a real user experience goal

The path from here is about making it:
- more coherent
- more trustworthy
- more maintainable
- more boringly reliable in the places where file managers must be boringly reliable

If we do that, KMiller can absolutely become a legitimate competitor rather than just a charming personal project.
