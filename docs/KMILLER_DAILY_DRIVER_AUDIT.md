# KMiller Daily Driver Stability / Parity Audit

Last updated: 2026-03-06

## Goal

This audit answers a narrower question than the broader codebase audit:

Can KMiller be trusted as a real daily-driver file manager, and what still blocks it from competing credibly with Dolphin and Thunar?

This document focuses on:
- stability and trustworthiness
- daily-use parity gaps
- behavioral consistency
- the difference between cosmetic issues and true release blockers

Grounding:
- current code inspection in the canonical repo
- successful current build from the canonical repo
- live parity debugging done during this session
- comparison against mainstream file-manager expectations

Canonical repo:
- [/mnt/steamssd/Documents/Development/kmiller](/mnt/steamssd/Documents/Development/kmiller)

## Executive Summary

KMiller is much closer to a daily-driver than a casual glance would suggest.

It already has:
- a real navigation model
- multiple view modes
- a working Miller-columns experience
- a preview pane
- a strong Quick Look subsystem
- tabs
- file operations
- properties
- settings
- Open With
- a file chooser direction
- release/install discipline

That is enough to make it feel like real software.

But daily-driver quality is not just about having features. It is mostly about:
- whether core actions feel trustworthy every time
- whether different views behave consistently
- whether regressions appear when unrelated UI features change
- whether file operations and dialogs behave predictably under normal use

Right now, KMiller's main weakness is not missing identity. It is reliability cohesion.

The app feels most at risk where:
- shared pane state is too tightly coupled
- view-specific behavior is duplicated
- operational behavior is UI-driven instead of policy-driven
- important dialogs and interaction flows can regress during unrelated work

So the path to daily-driver quality is not "add everything." It is:
- stabilize the existing core
- unify behavior across views
- harden file operations and recovery paths
- reduce fragile coupling in navigation/state handling

## What Already Feels Daily-Driver Capable

### 1. Core browsing shape is real
KMiller already provides the baseline browsing experience expected from a modern file manager:
- folder navigation
- multiple simultaneous views
- places sidebar
- status bar
- tabs
- current-folder awareness
- item selection and activation

Relevant files:
- [MainWindow.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MainWindow.cpp)
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

### 2. The Miller-columns experience is real, not decorative
This matters because KMiller's strongest case against Dolphin/Thunar is not "generic Linux file manager, but another one." It is:
- true Finder-style Miller columns
- coupled with preview-first workflows

Current code clearly supports that as a first-class experience, not a novelty mode.

### 3. Quick Look is already a standout feature
[QuickLookDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/QuickLookDialog.cpp) is one of the strongest parts of the app.

It supports:
- images
- text
- PDFs
- folders
- audio/video
- metadata and controls

That is already beyond what many Linux file managers feel like in practice.

### 4. Release/install discipline is stronger than average for the stage
[CMakeLists.txt](/mnt/steamssd/Documents/Development/kmiller/CMakeLists.txt) shows unusually mature instincts:
- versioning
- versioned installs
- stable launcher strategy
- source snapshot installation
- smoke-test hooks

That matters a lot for a daily-driver app because rollback and release trust are part of product quality.

### 5. File chooser and portal ambitions are strategically correct
Even if they are not fully mature yet, these components show the right long-term product direction:
- [FileChooserDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserDialog.cpp)
- [FileChooserPortal.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserPortal.cpp)

A file manager that can also become an ecosystem picker/provider is a strong platform move.

## Daily-Driver Release Blockers

These are the issues that most directly block KMiller from feeling trustworthy enough to recommend as a primary file manager.

### 1. Navigation/state coupling is too fragile
This is the biggest practical stability risk.

Recent live debugging already proved that a seemingly sensible navigation-sync change could break Quick Look. That tells us the current navigation model still has too much hidden coupling between:
- pane root state
- history
- breadcrumb/pathbar state
- selection state
- preview/Quick Look behavior

Primary hotspot:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)

Daily-driver implication:
- unrelated fixes can accidentally break core interactions
- regressions are too easy in the most frequently used part of the app

This is a release blocker because navigation consistency is table stakes.

### 2. Too much critical behavior is concentrated in `Pane.cpp`
[Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp) currently owns too much of the trust surface:
- navigation
- selection
- history
- preview pane
- context menus
- rename timing
- clipboard flows
- trash/delete behavior
- archive actions
- Open With
- sort and status updates

Daily-driver implication:
- changes are harder to reason about
- regressions can appear in surprising places
- testing burden grows too quickly

This is not just a code-style problem. It directly affects reliability.

### 3. View consistency is not yet strong enough
KMiller supports multiple views, but behavior still appears partially duplicated across:
- classic views
- Miller view

That affects things like:
- selection behavior
- rename timing
- Quick Look trigger behavior
- context-menu assumptions
- opening files/folders

Relevant files:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

Daily-driver implication:
- users cannot fully trust that the same action behaves the same way in every mode
- parity bugs become more likely every time one path is fixed and the other is not

This is a real blocker because inconsistency erodes confidence very quickly.

### 4. File operations are functional but not yet confidence-inspiring
KMiller can do the operations, but the surrounding user experience is not yet at the level of a mature primary file manager.

[FileOpsService.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileOpsService.cpp) is still a thin wrapper, not a strong operations subsystem.

Gaps include:
- no robust undo/redo model
- limited centralized conflict/collision policy
- limited job/progress UX
- limited operation-history visibility
- incomplete "trust signals" around long-running work

Daily-driver implication:
- users can perform actions, but they do not yet get the same operational reassurance they would expect from Dolphin

This is a release blocker because file operations are the trust core of a file manager.

### 5. Settings trust is weaker than settings surface
[SettingsDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/SettingsDialog.cpp) exposes a healthy-looking settings set, but not all of those settings appear to be enforced consistently app-wide.

Examples include:
- trash vs permanent delete behavior
- thumbnail behavior
- file extension visibility
- follow symlinks behavior
- terminal/default-app style settings

Daily-driver implication:
- the app can feel configurable without being fully dependable
- users may stop trusting preferences after a few mismatches

This is a blocker because daily-driver apps must keep their promises.

## Important Parity Gaps

These do not all block basic use immediately, but they are the main areas where KMiller still falls short of Dolphin/Thunar-class maturity.

### 1. Search is still filter-first, not strong search
Current search behavior is closer to inline filtering than a mature file-manager search system.

Relevant areas:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

What is missing for stronger parity:
- deeper recursive search options
- content/metadata-aware search integration
- saved/visible search state
- clearer search result mode

### 2. Properties are useful, but not yet comprehensive
[PropertiesDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/PropertiesDialog.cpp) is real, but still shallow compared with polished mainstream file managers.

Current gaps:
- directory size still unfinished in spirit
- metadata depth is limited
- permission editing is basic
- ownership/link-target/details could be richer
- batch metadata workflows are not mature

### 3. `Open With` is functional but not yet mature enough
Recent work fixed important dialog visibility and placeholder handling, which is good progress.

But for Dolphin/Thunar parity, `Open With` needs stronger confidence in:
- app list quality
- default-app management
- fallback behavior
- custom-command ergonomics
- consistency across file types

Relevant code:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)

### 4. Portal/file chooser support is promising, not yet battle-tested
[FileChooserDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserDialog.cpp) and [FileChooserPortal.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserPortal.cpp) are strategically smart, but they still read as early implementations.

For daily-driver confidence, these need:
- stronger option handling
- edge-case resilience
- parent-window/focus correctness
- more caller compatibility

### 5. Progress, conflict, and recovery UX need to feel more "grown up"
Mature file managers are not just action launchers. They are recovery environments.

KMiller still needs a stronger story around:
- conflict resolution
- job progress
- cancel/retry flows
- background operation visibility
- undo/recovery confidence

## Bad But Not Yet Blocking

These matter, but they are less urgent than the release blockers above.

### 1. Theme/style logic is still scattered
There is good polish work, but some style behavior is still inline and localized rather than systematized.

This affects long-term visual consistency more than immediate trust.

### 2. Properties, chooser, and portal depth can come after stability
These are valuable, but they do not need to be solved before the navigation/operations trust model is stronger.

### 3. More advanced features can wait
Examples:
- richer archive management
- deeper metadata editing
- more elaborate background job UI
- advanced bookmarks/recent workflows

These are worth doing, but they are second-order if basic reliability is not fully locked in.

## Good Surprises

These are areas where KMiller is already stronger than many first impressions would suggest.

### 1. Quick Look is a serious differentiator
This is not marketing fluff. It is real product substance.

### 2. The app already has a coherent product identity
KMiller does not feel like a random feature pile. It still knows what it is trying to be.

### 3. Release maturity instincts are strong
Versioned installs, stable launcher discipline, and rollback thinking are all daily-driver-positive traits.

### 4. The codebase is fixable
Even the ugly parts are not hopeless. They are concentrated enough that a cleanup roadmap is realistic.

## Release Gate For "Daily Driver" Status

KMiller should not call itself a true daily-driver competitor until these pass reliably:

### Core trust gate
- navigation does not regress Quick Look or preview behavior
- back/forward/up/pathbar behavior is coherent
- selection behavior is consistent across all view modes
- dialogs render correctly and predictably

### Operations gate
- copy/move/trash/delete/duplicate/rename are consistent and clearly recoverable
- conflict handling is understandable
- long operations provide visible trust signals
- error handling leaves the app responsive and state-consistent

### View-consistency gate
- Miller and classic views behave equivalently for core interactions
- context menus and keyboard flows match expectations in every view
- Quick Look and preview pane remain reliable across all views

### Preferences gate
- exposed settings are actually honored consistently
- hidden-files, extension visibility, trash/delete, and thumbnail settings all behave predictably

### Polish gate
- no invisible/offscreen dialogs
- no ghost surfaces or compositor confusion
- no major keyboard/mouse regressions in daily flows

## Recommended Immediate Priorities

### Priority 1: Stabilize pane navigation/state ownership
Goal:
- make one subsystem the clear source of truth for navigation state
- stop unrelated UI sync work from breaking preview/Quick Look

### Priority 2: Unify interaction semantics across views
Goal:
- one behavioral model for selection, open, rename, Quick Look, and context-menu handling
- reduce drift between Miller and classic views

### Priority 3: Strengthen file operations trust
Goal:
- make file operations feel safe and comprehensible
- centralize more operation policy in `FileOpsService` or a successor subsystem

### Priority 4: Audit settings enforcement end-to-end
Goal:
- every setting in the UI should actually mean something everywhere it claims to apply

### Priority 5: Keep parity work tied to release gates
Goal:
- treat daily-driver readiness as a testable quality bar, not a feeling

## Bottom Line

KMiller is not missing the soul of a daily-driver file manager.
It already has that.

What it still needs is the boring strength that makes people trust a file manager every day:
- predictable state
- consistent interactions
- dependable operations
- fewer regression-prone couplings

That is good news.

It means KMiller does not need to reinvent itself to compete with Dolphin and Thunar.
It needs to harden, unify, and simplify the parts it already has.
