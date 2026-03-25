# KMiller Phased Implementation Checklist

Last updated: 2026-03-06

## Purpose

This checklist turns:
- [KMILLER_ROADMAP.md](/mnt/steamssd/Documents/Development/kmiller/docs/KMILLER_ROADMAP.md)
- [KMILLER_DAILY_DRIVER_AUDIT.md](/mnt/steamssd/Documents/Development/kmiller/docs/KMILLER_DAILY_DRIVER_AUDIT.md)

into a concrete execution plan.

The order is intentional:
- first make KMiller trustworthy
- then make it consistent
- then make it mature
- then make it excellent

This is not a feature wish list.
It is a practical path toward a daily-driver file manager that can genuinely compete with Dolphin and Thunar while preserving KMiller's Finder-like identity.

## How To Use This Checklist

- Treat unchecked items in Phase 1 as more urgent than almost everything else.
- Do not add broad new features while Phase 1 release blockers are still unstable.
- When a task spans multiple files, prefer extracting a subsystem rather than adding more logic to `Pane.cpp`.
- After each phase, run the visual/release gate checks before moving on.

## Phase 1: Daily-Driver Stability Foundation

### Objective
Eliminate the current class of regressions where core browsing, Quick Look, pathbar, or dialogs can break each other.

### 1. Navigation/state ownership
- [ ] Define one clear source of truth for pane navigation state.
- [ ] Separate breadcrumb display sync from history/root mutation.
- [ ] Audit every place that mutates pane root/history/selection state.
- [ ] Remove hidden coupling between navigation, preview pane, and Quick Look.
- [ ] Add a lightweight navigation-state helper or subsystem extracted from [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp).

Primary files:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [Pane.h](/mnt/steamssd/Documents/Development/kmiller/src/Pane.h)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

### 2. Dialog reliability
- [ ] Audit all modal and child dialogs for visibility, centering, focus, and workspace behavior.
- [ ] Standardize dialog creation patterns so `Go to Folder`, `Open With`, properties, and chooser dialogs behave consistently.
- [ ] Eliminate offscreen, invisible, or compositor-confusing dialog presentation paths.
- [ ] Verify all dialogs close and restore focus predictably.

Primary files:
- [MainWindow.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MainWindow.cpp)
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [PropertiesDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/PropertiesDialog.cpp)
- [FileChooserDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserDialog.cpp)

### 3. View behavior consistency
- [ ] Write down the canonical behavior rules for select/open/rename/right-click/Quick Look.
- [ ] Audit classic views against Miller view for each rule.
- [ ] Remove duplicated behavior where one helper can serve both view families.
- [ ] Make sure right-click selection correction behaves identically across views.
- [ ] Make sure Quick Look triggers behave identically across views.
- [ ] Make sure rename timing/slow-second-click behavior behaves identically across views.

Primary files:
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

### 4. Settings trust sweep
- [ ] Inventory every exposed user setting.
- [ ] Verify each setting actually changes runtime behavior where promised.
- [ ] Remove or hide any setting that is not truly supported yet.
- [ ] Standardize settings load/apply/save behavior across tabs and panes.
- [ ] Add a quick manual regression list for hidden files, thumbnails, file extensions, trash behavior, and column width.

Primary files:
- [SettingsDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/SettingsDialog.cpp)
- [MainWindow.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MainWindow.cpp)
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)
- [MillerView.cpp](/mnt/steamssd/Documents/Development/kmiller/src/MillerView.cpp)

### 5. Phase 1 release gate
- [ ] Pathbar and breadcrumb navigation do not regress Quick Look.
- [ ] `Go to Folder` works from both shortcuts and renders correctly.
- [ ] `Open With` renders correctly and is usable.
- [ ] Quick Look works from keyboard and context menu in all relevant views.
- [ ] No major dialog opens invisibly, offscreen, or on the wrong surface.

## Phase 2: File Operations Trust

### Objective
Make KMiller feel safe and trustworthy with real files, not just visually appealing.

### 1. Strengthen the operations layer
- [ ] Expand [FileOpsService.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileOpsService.cpp) beyond thin wrappers.
- [ ] Centralize copy/move/trash/delete/duplicate policy.
- [ ] Centralize error handling and user feedback for file operations.
- [ ] Reduce direct KIO operation wiring inside [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp).

### 2. Progress and feedback
- [ ] Add visible progress for long-running operations.
- [ ] Add clearer success/failure feedback after operations.
- [ ] Make cancellations and partial failures understandable.
- [ ] Standardize status bar / dialog / message feedback patterns.

### 3. Collision and overwrite behavior
- [ ] Audit copy/move collision behavior.
- [ ] Define overwrite/rename/skip expectations.
- [ ] Add clear conflict handling UI if current defaults are not sufficient.

### 4. Recovery and trust features
- [ ] Decide on near-term undo/redo strategy.
- [ ] If true undo is too large for now, add operation-history groundwork.
- [ ] Audit trash vs permanent delete behavior against settings.
- [ ] Verify duplicate/compress/extract all produce predictable results.

### 5. Phase 2 release gate
- [ ] Copy, move, duplicate, trash, delete, compress, and extract all feel predictable.
- [ ] Long operations provide visible trust signals.
- [ ] Failure paths do not leave stale or confusing UI state.

## Phase 3: Finder-Like Interaction Polish

### Objective
Make KMiller not just usable, but uniquely pleasant.

### 1. Miller-columns polish
- [ ] Tune keyboard navigation until it feels effortless.
- [ ] Audit focus movement between columns after open/back/up/navigation.
- [ ] Verify empty-column cleanup and active-column correctness.
- [ ] Make status/selection reporting feel coherent in Miller mode.

### 2. Quick Look and preview refinement
- [ ] Audit Quick Look behavior for images, text, PDFs, folders, audio, and video.
- [ ] Improve preview-pane fidelity and consistency with Quick Look.
- [ ] Make preview and Quick Look feel like one coherent preview system.
- [ ] Reduce visual/style drift between the two systems.

Primary files:
- [QuickLookDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/QuickLookDialog.cpp)
- [Pane.cpp](/mnt/steamssd/Documents/Development/kmiller/src/Pane.cpp)

### 3. Path and rename ergonomics
- [ ] Refine breadcrumb click behavior and long-path handling.
- [ ] Confirm `Ctrl+L` / `Ctrl+Shift+G` workflows are clean and discoverable.
- [ ] Audit inline rename from mouse and keyboard paths.
- [ ] Ensure rename cancellation/focus restoration behaves cleanly.

### 4. Visual coherence
- [ ] Reduce ad hoc inline styling where practical.
- [ ] Make dialogs, preview surfaces, and panes feel like one app.
- [ ] Audit dark/light/Finder theme readability.

### 5. Phase 3 release gate
- [ ] KMiller feels intentional and polished rather than merely feature-rich.
- [ ] Miller-columns and Quick Look are clearly stronger than mainstream Linux alternatives.

## Phase 4: Mainstream Parity Gaps

### Objective
Close the remaining reasons a user would still need Dolphin or Thunar for everyday work.

### 1. Search maturity
- [ ] Define the target search model: filter, recursive search, or both.
- [ ] Improve current filter UX.
- [ ] Add deeper search capabilities as appropriate.
- [ ] Make search state visible and easy to clear.

### 2. Properties maturity
- [ ] Finish directory-size behavior properly.
- [ ] Improve permissions/ownership depth where feasible.
- [ ] Improve metadata presentation and file detail coverage.
- [ ] Decide whether batch rename belongs here or in a separate workflow.

Primary files:
- [PropertiesDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/PropertiesDialog.cpp)

### 3. Open With maturity
- [ ] Improve app list quality and ordering.
- [ ] Add clearer default-app handling.
- [ ] Make custom-command UX less fragile.
- [ ] Validate behavior across text, image, media, PDF, unknown, and desktop-file cases.

### 4. Chooser and portal maturity
- [ ] Harden [FileChooserDialog.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserDialog.cpp) for real-world picker use.
- [ ] Improve option parsing and caller compatibility in [FileChooserPortal.cpp](/mnt/steamssd/Documents/Development/kmiller/src/FileChooserPortal.cpp).
- [ ] Verify focus/parent-window integration under real calling scenarios.

### 5. Ecosystem integration
- [ ] Improve terminal/external-tool handoff.
- [ ] Audit mounts, removable devices, and external locations.
- [ ] Review bookmarks, places, and recent-location workflows.

### 6. Phase 4 release gate
- [ ] Most everyday "I need Dolphin/Thunar for this" moments are gone.
- [ ] Remaining gaps are secondary rather than workflow-blocking.

## Phase 5: Release Confidence And Maintainability

### Objective
Make KMiller sustainable to ship and evolve.

### 1. Regression coverage
- [ ] Create a manual regression suite for pathbar, Quick Look, Open With, rename, and file operations.
- [ ] Add smoke/integration checks for the most failure-prone flows.
- [ ] Add regression coverage specifically around navigation/state coupling.

### 2. Release discipline
- [ ] Keep versioned installs and stable launcher behavior clean.
- [ ] Formalize release gate docs for tagging and shipping.
- [ ] Keep patch notes and release notes short, clear, and reliable.

### 3. Architecture follow-through
- [ ] Finish extracting new subsystems instead of leaving half-refactored glue.
- [ ] Document ownership of major subsystems.
- [ ] Make future parity work easier, not harder.

### 4. Phase 5 release gate
- [ ] Shipping a release feels routine.
- [ ] Confidence comes from repeatable checks, not memory or luck.

## Suggested First Execution Slice

If we start immediately, the best first slice is:

### Slice A: Stop regressions from cross-breaking core workflows
- [ ] Extract or isolate pane navigation state.
- [ ] Audit and simplify pathbar/history/Quick Look coupling.
- [ ] Standardize dialog creation/presentation.
- [ ] Verify Quick Look, Open With, and Go to Folder all survive navigation changes.

### Slice B: Unify core interactions across views
- [ ] Define canonical selection/open/rename/right-click rules.
- [ ] Port both classic and Miller behavior to shared helpers where possible.
- [ ] Retest the visual parity checklist.

### Slice C: Strengthen operation trust
- [ ] Move more logic into the operations layer.
- [ ] Improve feedback and collision handling.
- [ ] Verify trash/delete/settings consistency.

## Success Definition

KMiller is ready to compete as a daily-driver when:
- users can trust navigation, Quick Look, and file operations every day
- view switching does not change fundamental behavior unexpectedly
- settings actually do what they say
- dialogs and workflow surfaces behave predictably
- the app still feels uniquely like KMiller, not like a generic file manager clone
