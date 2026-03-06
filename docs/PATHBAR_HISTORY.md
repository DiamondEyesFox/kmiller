# KMiller Pathbar / Breadcrumb History

## Scope

This document reconstructs the evolution of KMiller's pathbar and breadcrumb behavior across:

- every locally available git tag in the active repo
- the current dirty working tree
- older Claude/Codex logs and release-backup diffs when they disagree with the tag history

The goal is to answer:

- when `KUrlNavigator` first appeared
- when breadcrumb syncing worked or regressed
- which changes were part of the long-running design
- which changes appear to be later duct tape

## Sources Used

- active repo: [`/home/diamondeyesfox/kmiller-5.24-hotfix`](/home/diamondeyesfox/kmiller-5.24-hotfix)
- older backup repos under [`/mnt/steamssd/Documents/Development/kmiller`](/mnt/steamssd/Documents/Development/kmiller)
- Claude offloaded logs, especially:
  - [Jan 16, 2026 session](/mnt/steamssd/Vault/DEF%20Master%20Vault/Outside%20Vault%20Data/Vault%20Offloads/LLM%20Logs/Claude%20Code%20Logs/2026/01-January/16/11_Session_2026-01-16_11-23pm_f8aceca9.md)
  - [Jan 17, 2026 session](/mnt/steamssd/Vault/DEF%20Master%20Vault/Outside%20Vault%20Data/Vault%20Offloads/LLM%20Logs/Claude%20Code%20Logs/2026/01-January/17/01_Session_2026-01-17_12-10am_f8aceca9.md)
- Codex offloaded logs, especially:
  - [Feb 14, 2026 session](/mnt/steamssd/Vault/DEF%20Master%20Vault/Outside%20Vault%20Data/Vault%20Offloads/LLM%20Logs/Codex/codex-2026-02-14_15-50-21_019c5e22.md)
- release snapshots:
  - [releases/kmiller-5.25/source](/home/diamondeyesfox/releases/kmiller-5.25/source)
  - [pre-parity backup diff](/home/diamondeyesfox/releases/kmiller-backups/pre-parity-20260214-194301/git-working.diff)
  - [pre-ql-style backup diff](/home/diamondeyesfox/releases/kmiller-backups/pre-ql-style-20260214-203703/git-working.diff)

## Executive Summary

The pathbar history splits into four distinct eras:

1. No `KUrlNavigator` yet.
2. `KUrlNavigator` exists as a plain pathbar, but without explicit breadcrumb-mode forcing.
3. Breadcrumb syncing is attempted from the wrong signal (`selectionChanged`) and later reverted after breaking navigation.
4. A safer sync model appears using `MillerView::navigatedTo`, and later an additional stronger lockout is layered on top with:
   - `setUrlEditable(false)`
   - `setShowFullPath(true)`
   - `editableStateChanged` forcing edit mode back off

Important conclusion:

- The oldest clearly bad regression was the old `selectionChanged`-driven breadcrumb sync.
- The current dirty tree's most suspicious behavior is not `KUrlNavigator` itself, but the stronger "never stay editable" lockout via `editableStateChanged`, even though that behavior can now be traced back at least to the Feb. 14 working `5.25.2` line.
- There is a discrepancy between the current git tags and older Claude/release-backup evidence: some breadcrumb-mode work existed in logs and backup diffs but is not present in the current tagged `v5.25.x` code.

## Version-By-Version History

### Phase 0: No KUrlNavigator Yet

These versions do not contain `KUrlNavigator` in `Pane.cpp`:

- `v0.4.2-alpha`
- `v0.4.3`
- `v0.5.0`

Interpretation:

- the app did not yet have the KDE-style path navigator
- any "pathbar" behavior here would have been from a different widget or not present yet

### Phase 1: Plain KUrlNavigator Pathbar

These versions all contain `KUrlNavigator`, but do not explicitly force breadcrumb mode with `setUrlEditable(false)`:

- `v0.4.3-final`
- `v0.4.4`
- `v0.4.7`
- `v0.5.2`
- `v0.5.3`
- `v5.3`
- `v5.4`
- `v5.8`
- `v5.9`
- `v5.10`
- `v5.11`
- `v5.12`
- `v5.13`
- `v5.14`
- `v5.15`
- `v5.16`

Typical state in this era:

- `nav = new KUrlNavigator(this);`
- `connect(nav, &KUrlNavigator::urlChanged, this, &Pane::onNavigatorUrlChanged);`
- no `editableStateChanged` hook
- no `QSignalBlocker`-based sync helper

Interpretation:

- `KUrlNavigator` is present and wired for user-initiated navigation
- breadcrumb display mode is not being strongly managed in code
- this is the stable "plain navigator" era

### Phase 2: Miller Columns Grow Their Own Navigation Signal

Beginning in:

- `v5.17`
- and continuing through `v5.18`, `v5.19`, `v5.22`, `v5.23`, `v5.24`, `v5.25`, `v5.25.1`, `v5.25.2`

New behavior appears:

- `MillerView::navigatedTo` exists
- `Pane` connects to it
- `MainWindow` also begins styling `KUrlNavigator`

But in the tagged code, this still looks relatively light:

- `navigatedTo` mostly forwards to `urlChanged`
- there is still no:
  - `setUrlEditable(false)`
  - `setShowFullPath(true)`
  - `editableStateChanged`
  - `QSignalBlocker`

Interpretation:

- the project now has a signal appropriate for breadcrumb syncing
- but the tags do not yet show the more opinionated breadcrumb-mode enforcement

### Phase 3: The Old Broken Breadcrumb Regression

Separate from the clean tag timeline, git history shows a broken experiment:

- commit `0f11cfa` `KMiller v5.6: Enhanced Navigation and Context Menu System`
- later reverted by:
  - commit `0d88068` `KMiller v5.6 BROKEN - Revert pathbar changes`

What went wrong:

- breadcrumb updates were being driven from `MillerView::selectionChanged`
- that interfered with normal directory navigation

This is the first clearly documented pathbar regression.

Key lesson from that era:

- `selectionChanged` is the wrong source of truth for breadcrumb navigation

### Phase 4: Claude's Mid-January Breadcrumb Work

Older Claude logs show pathbar work that does not cleanly line up with the current git tags.

The Jan 16-17 sessions describe:

- `KUrlNavigator` needing `KFilePlacesModel`
- `setUrlEditable(false)` being added to force breadcrumb mode
- the navigator showing `/` only because Miller column navigation was not updating it
- fixing that by syncing `nav->setLocationUrl(url)` from Miller navigation

Those logs explicitly say the pathbar was then working as clickable breadcrumbs.

This suggests that an important breadcrumb-oriented working state existed in a live worktree or install, even if that exact state is not preserved in the current `v5.25.x` tags in this repo.

### Phase 4.5: Feb. 14 Working `5.25.2` Line

The Feb. 14 Codex session and release-backup diffs make one thing much clearer:

- by the working `5.25.2` line, the app already had:
  - `setUrlEditable(false)`
  - `setShowFullPath(true)`
  - `editableStateChanged`
- that means the breadcrumb lockout was not invented during today's review session

What remains unusual is that this state is visible in:

- the Feb. 14 Codex transcript
- the Feb. 14 release backup diffs
- the reported `5.25.2` binary behavior

but not in the current repo's `v5.25.2` tag contents.

Interpretation:

- the lockout behavior belongs to the real Feb. 14 `5.25.2` working line
- but the currently checked-in tag history is incomplete as a record of that work

### Phase 5: Current Dirty Tree

The current uncommitted tree adds all of the following:

- `nav->setUrlEditable(false);`
- `nav->setShowFullPath(true);`
- `editableStateChanged` hook that forces edit mode back off
- `syncNavigatorLocation()` using `QSignalBlocker`
- `MillerView::navigatedTo` updating `currentRoot` and the navigator
- `focusView()` after breadcrumb navigation

Current code references:

- [Pane.cpp](/home/diamondeyesfox/kmiller-5.24-hotfix/src/Pane.cpp:210)
- [Pane.cpp](/home/diamondeyesfox/kmiller-5.24-hotfix/src/Pane.cpp:358)
- [Pane.cpp](/home/diamondeyesfox/kmiller-5.24-hotfix/src/Pane.cpp:385)
- [Pane.cpp](/home/diamondeyesfox/kmiller-5.24-hotfix/src/Pane.cpp:501)

Interpretation:

- this is the most sophisticated pathbar state seen in code
- it also contains the strongest "do not allow edit mode" behavior
- that strongest lockout is the part most likely to be duct tape

## Key Discrepancy: Tags vs Logs vs Backup Diffs

There is an important mismatch:

- the tagged `v5.25` / `v5.25.1` / `v5.25.2` code in the current repo does **not** contain:
  - `setUrlEditable(false)`
  - `setShowFullPath(true)`
  - `editableStateChanged`
- but:
  - Claude logs from January discuss `setUrlEditable(false)` explicitly
  - release-backup diffs from Feb 14 show:
    - `setUrlEditable(false)`
    - `setShowFullPath(true)`
    - `editableStateChanged`

This means one of the following is true:

1. A working tree with those changes existed outside the currently tagged history.
2. The release backup diffs captured local changes that were never committed/tagged.
3. The current active clone is not the only authoritative history of the pathbar work.

This discrepancy is why transcript evidence and backup diffs matter here, not just tags.

## What Was Stable vs Risky

### Stable / Defensible Ideas

- using `KUrlNavigator` at all
- wiring `urlChanged` to `onNavigatorUrlChanged`
- updating breadcrumbs from Miller navigation rather than from random selection
- a dedicated sync helper like `syncNavigatorLocation()`
- focus handoff back to the active view after breadcrumb clicks

### Historically Risky Ideas

- driving breadcrumb changes from `selectionChanged`
- allowing view-selection noise to masquerade as directory navigation

### Currently Suspect

- `editableStateChanged` immediately forcing the bar out of edit mode again

This is the strongest candidate for "duct tape" because:

- it is stricter than the older breadcrumb-mode intent
- it removes the user's ability to type/paste paths
- it has not yet been justified by a clearly documented reproduced bug in the current code history

## Repo Copies and Why the History Is Confusing

The code lineage is split across multiple places:

- active repo:
  - [`/home/diamondeyesfox/kmiller-5.24-hotfix`](/home/diamondeyesfox/kmiller-5.24-hotfix)
- symlink alias:
  - [`/home/diamondeyesfox/kmiller`](/home/diamondeyesfox/kmiller)
- older archive tree:
  - [`/mnt/steamssd/Documents/Development/kmiller`](/mnt/steamssd/Documents/Development/kmiller)

The `Documents/Development/kmiller` tree mostly contains:

- `v0.4.3` era repos and backups
- not the current clean `5.25.x` history

This is one reason the pathbar story appears inconsistent across source snapshots.

## Recommended Interpretation for Future Work

When making future pathbar changes:

1. Treat the old `selectionChanged` breadcrumb sync as a known anti-pattern.
2. Keep the safer `navigatedTo`-driven sync model.
3. Treat `setUrlEditable(false)` as historically intentional breadcrumb-mode behavior, not automatically a bug.
4. Treat the newer `editableStateChanged` force-disable hook as the first thing to question in regression triage.

## Appendix: Tag Inventory

Locally available tags:

- `v0.4.2-alpha`
- `v0.4.3`
- `v0.4.3-final`
- `v0.4.4`
- `v0.4.7`
- `v0.5.0`
- `v0.5.2`
- `v0.5.3`
- `v5.3`
- `v5.4`
- `v5.8`
- `v5.9`
- `v5.10`
- `v5.11`
- `v5.12`
- `v5.13`
- `v5.14`
- `v5.15`
- `v5.16`
- `v5.17`
- `v5.18`
- `v5.19`
- `v5.22`
- `v5.23`
- `v5.24`
- `v5.25`
- `v5.25.1`
- `v5.25.2`
