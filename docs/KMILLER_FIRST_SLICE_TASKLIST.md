# KMiller First Slice Task List

Last updated: 2026-03-06

## Goal

Execute the first high-leverage stability slice from the roadmap and implementation checklist:

- reduce brittle coupling in pane navigation/state handling
- make dialog behavior more standardized
- improve cross-view consistency for the most trust-critical interactions

This slice is intentionally narrow. It is not a general feature sprint.

## Why This Slice First

The current audits and source review all point to the same conclusion:

- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp) is the main architectural and reliability hotspot
- navigation/history/pathbar logic is still too entangled with UI behavior
- dialog behavior is partly standardized and partly ad hoc
- regressions are too easy in the exact workflows that define trust

The best immediate return is to reduce fragility before adding breadth.

## Concrete Work Order

### Step 1: Extract pane navigation/history state

Objective:
- make one place responsible for navigation history mutation and traversal

Target files:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [Pane.h](/mnt/steamssd/Documents/Development/kmiller/src/Pane.h)
- new helper:
  - `PaneNavigationState.h`
  - `PaneNavigationState.cpp`

Implementation target:
- move history bookkeeping out of inline `Pane` methods
- make `setRoot`, `goBack`, and `goForward` use one helper
- reduce the chance of future pathbar/history regressions

Success check:
- navigation still behaves the same
- `goBack` / `goForward` / direct navigation all build on one state model

### Step 2: Centralize pane location application

Objective:
- separate “decide what the current location should be” from “apply that location to the views/model”

Target files:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [Pane.h](/mnt/steamssd/Documents/Development/kmiller/src/Pane.h)

Implementation target:
- add one helper for applying a location to:
  - `currentRoot`
  - `KDirLister`
  - `MillerView`
  - navigator sync
  - `urlChanged`
- make `setRoot`, `goBack`, and `goForward` route through it

Success check:
- no duplicated location-application block remains in those paths

### Step 3: Standardize dialog parenting/presentation helpers

Objective:
- stop hand-rolling modal dialog setup differently in each path

Target files:
- [MainWindow.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MainWindow.cpp)
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)

Implementation target:
- introduce small reusable helpers for:
  - modal dialog flags
  - centering on parent window
  - focus/raise/show behavior

First candidates:
- `Go to Folder`
- `Open With`

Success check:
- both flows use the same presentation path

### Step 4: Define canonical core interaction parity

Objective:
- reduce drift between classic views and Miller view

Target files:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

Initial interaction rules to normalize:
- selection
- open
- Quick Look trigger
- rename trigger
- right-click correction

This step may begin in this slice but does not need to fully finish before the first checkpoint.

## Out Of Scope For This Slice

- deeper file-ops redesign
- portal maturity work
- search expansion
- properties dialog depth work
- big visual restyling

## Immediate First Refactor

Start here:

1. Create `PaneNavigationState`
2. Move history logic into it
3. Add a single `applyLocation(...)` helper in `Pane`
4. Replace duplicated `currentRoot + openUrl + miller + nav sync` blocks

That is the strongest first move because it directly attacks the regression class that already caused trouble.

