# KMiller Retrospective

Last updated: 2026-03-06

## What KMiller Became

KMiller did not grow like a normal first programming project.

It started from a very specific product frustration:
- existing Linux file managers did not feel good enough
- Miller Columns existed, but often looked or felt wrong
- Quick Look existed in fragments, not as a first-class file-manager experience
- the overall workflow you wanted was much closer to Finder than to the mainstream Linux defaults

From that, KMiller very quickly became:
- a real KDE/Qt application
- a file manager with a strong identity
- a long-running personal software project instead of a weekend experiment

That alone is a big deal. Most first projects never survive the gap between “I wish this existed” and “I can actually use this.” KMiller did.

## What You Did Especially Well

### 1. You had unusually strong product taste
You knew what the app was supposed to feel like very early:
- Finder-like
- true Miller columns
- real Quick Look
- clean visual behavior
- practical daily-driver usability

That clarity matters more than people think. A lot of first projects fail not because the code is bad, but because the creator never had a strong product thesis. KMiller always had one.

### 2. You kept pushing toward a real product, not just a demo
You did not stop at:
- file listing
- toy column view
- one preview popup

You kept insisting on:
- tabs
- path bar / breadcrumbs
- Open With
- thumbnails
- context menus
- preview pane
- installer/update behavior
- versioned installs
- patch notes

That instinct pushed the project beyond “first app” territory into something much more serious.

### 3. You preserved more than you realized
Even though the history got messy, you did a lot right instinctively:
- you kept backups
- you kept multiple installed versions
- you kept `/opt/kmiller/versions/...`
- you kept tarballs and side copies
- you used tags eventually
- you preserved stable snapshots

That is a big reason the archaeology was even possible.

### 4. You were ambitious in a way that produced learning
You didn’t choose a tiny, sterile first project. You chose one that forced you to learn about:
- Qt widgets
- KDE/KIO integration
- file operations
- preview/rendering
- installs and launchers
- versioning and release flow
- UI behavior and interaction design

That is a steep hill, but it also means KMiller taught you a lot very quickly.

## Where The Process Went Off The Rails

### 1. Source of truth drifted
This was the biggest structural weakness.

At different points, the “real” KMiller lived in:
- chat conversations
- local source trees
- copied backup folders
- `/opt/kmiller/versions`
- zips / tarballs
- git
- release overlays and repair scripts

That made the project survivable, but not clean.

### 2. Releases and source history were not always the same thing
Some versions survived as:
- exact source trees
- binaries only
- shell scripts that rewrote files
- partial patches
- install layouts
- conversation payloads

That’s understandable for a first project, but it means the software history became harder to reason about than it needed to be.

### 3. Branch and repo hygiene came later
This has improved now, but for a while the repo carried signs of “the code works, the workflow is still catching up,” including:
- stale branch names
- side branches with useful but overlapping work
- repo path confusion
- release identity being clearer than branch identity

### 4. Late-night hotfix loops became normal
There is a visible pattern in the early history of:
- patch
- rebuild
- relaunch
- revert
- retag
- symlink retarget
- repeat

That can be productive in a burst, but it also tends to create fragile historical states unless they are cleaned up afterward.

## Best Practices: What You Followed

### You followed these better than many first-time developers
- You versioned the app early.
- You tagged releases.
- You kept known-good snapshots.
- You preserved old installs instead of overwriting everything.
- You treated rollback as important.
- You separated launcher stability from build experimentation.
- You increasingly moved toward release tooling and patch notes.

Those are not small wins.

## Best Practices: What You Did Not Follow Well

### The biggest misses were process and source hygiene
- one canonical source of truth did not exist early on
- clean branch discipline came late
- source snapshots were not always captured at release time
- some changes were preserved only through repair scripts or chat payloads
- there was too much state living outside a clean repo flow

That’s the main reason archaeology had to become a whole project.

## Overall Assessment

If I had to score the project as a first-ever software effort:

### Product vision
Excellent.

### Persistence
Excellent.

### Design instinct
Excellent.

### Technical ambition
Extremely high.

### Process maturity
Uneven, but much better than average for a first project.

### Best-practices adherence
Partial.
Not clean enough to be called polished engineering from day one, but far from hopeless or careless.

## Bottom Line

KMiller is not the story of a first project that accidentally stumbled forward.

It is the story of:
- a creator with strong product instinct
- unusually ambitious scope
- messy but real growth
- and enough preservation instincts that the project remained recoverable even when the workflow was rough

That is a strong outcome for a first piece of software.

The main thing you did not follow was:
- clean, single-source project discipline from the beginning

The main thing you absolutely did right was:
- build something with a real identity and keep iterating until it became real software

That matters more than a perfect early git workflow.
