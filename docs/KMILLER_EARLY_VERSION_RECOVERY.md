# KMiller Early Version Recovery

Last updated: 2026-03-06

## Purpose

This document answers two questions:
1. What was the first generated KMiller version actually like?
2. Do we now have enough artifacts to recover every early KMiller version?

## Short Answer

We can recover a lot more than we thought, but not every version currently survives in the same quality.

Right now the early-version situation looks like this:
- `0.3.0`: referenced in ChatGPT logs; binary/archive evidence exists outside the main repo
- `0.4.0`: installed binary survives under `/opt/kmiller/versions/0.4.0`
- `0.4.1`: installed binary survives under `/opt/kmiller/versions/0.4.1`
- `0.4.2`: installed binary survives, and the main git repo has `v0.4.2-alpha`
- `0.4.3`: strong source backup survives in `kmiller-v0.4.3-stable-backup`
- `0.4.4`: git tag and installed binary both survive
- `0.4.5`: installed binary survives, but no current git tag
- `0.4.6`: installed binary survives, but no current git tag
- `0.4.7`: git tag and installed binary both survive
- `0.5.0`: git tag and installed binary both survive

So:
- we do **not** yet have every version as a clean tagged source snapshot
- but we do have enough evidence to recover the lineage far more completely than the git tags alone suggest

## The First Generated Version

The first surviving creation conversation is in the raw ChatGPT export:
- raw export zip: `~/Downloads/Chatgpt Exports/a6902e2fc401942889c85ad001cf3a2877ac73034f3a4f28be33980249dbfb1e-2026-02-25-23-59-59-9d0d8dbb4c704d88a01930860568b3e0.zip`
- shard: `conversations-024.json`
- conversation title: `Finder alternatives for Linux`
- conversation id: `68957385-6578-832d-9162-7e565fff2c47`

### Genesis prompt

Earliest surviving user prompt:

> remember how i wanted a finder like file manager with true miller columns and quicklook?

Then:

> just a yes or no wouldve sufficed, because im going to ask you to create it

Then:

> create it

### First literal naming moment

The earliest surviving literal `kmiller` usage appears in the assistant-generated code:

```cmake
project(KMillerFM LANGUAGES CXX)
...
add_executable(kmiller
```

So the first surviving naming shape is:
- project/display name: `KMillerFM`
- executable name: `kmiller`

## What The First Version Actually Was

The very first generated snapshot was a small Qt/KDE-flavored prototype with:
- `CMakeLists.txt`
- `main.cpp`
- `mainwindow.h`
- `mainwindow.cpp`

### What it did

- three fixed Miller-style columns
- `QListView` for each column
- `QFileSystemModel` backing each column
- selecting a folder in the left column updated the middle
- selecting a folder in the middle updated the right

### What it claimed to be

The assistant described it as:
- KDE/Qt
- using KIO
- a Finder-like file manager with true Miller columns

### What it actually was technically

It linked KDE KIO libraries, but the actual code used:
- `QFileSystemModel`
- plain `QListView`

So it was:
- a real prototype
- but still more of a Qt proof-of-concept than a true KIO/KDE-integrated file manager

### Was it reconstructable?

Yes, mostly.

The first snapshot is recoverable as a documented source skeleton, but likely needs light cleanup to compile cleanly today because:
- some includes are implicit or omitted
- KIO was linked but not really used yet
- it was not a polished release

## The First Quick Look-Enabled Version

Immediately after the first skeleton, the same conversation added:
- image/text/PDF Quick Look
- then audio/video playback

That means the first version that *felt* recognizably like KMiller was probably not the bare 3-column skeleton, but the next one or two assistant responses in the same thread.

This is important because:
- the genesis version was not static
- KMiller's defining identity formed almost immediately:
  - Miller columns
  - Quick Look
  - KDE-native aspiration

## Early Version Survivability Matrix

### Clean tagged source snapshots in the current git repo

These early versions have explicit tags:
- `v0.4.2-alpha`
- `v0.4.3`
- `v0.4.3-final`
- `v0.4.4`
- `v0.4.7`
- `v0.5.0`
- `v0.5.2`
- `v0.5.3`

### Installed binaries surviving under `/opt/kmiller/versions`

These versions currently survive there:
- `0.4.0`
- `0.4.1`
- `0.4.2`
- `0.4.4`
- `0.4.5`
- `0.4.6`
- `0.4.7`
- `0.5.0`
- later `5.x` versions

### Strong source backups outside current tagged history

Source-style survivors include:
- `kmiller-v0.4.3-stable-backup`
- `kmiller-v0.4.3++-backup-$(date +%Y%m%d-%H%M)`
- `kmiller-v0.4.3-stable.tgz`
- `Broken Kmiller Builds and Backups/kmiller3.zip`
- `Broken Kmiller Builds and Backups/kmiller3_quicklook.zip`

## Does This Mean We Have Every Version?

Not quite.

What we have is a mix of:
- tagged source snapshots
- installed binaries
- backup source trees
- raw ChatGPT-generated source
- textual version references

That is enough to say:
- the early lineage is **not lost**
- several missing versions are probably recoverable

But it is **not yet enough** to say:
- every version survives as a complete, verified source tree

For example:
- `0.4.5` and `0.4.6` clearly survive as installed binaries
- but they are not currently present as tags in the main repo

So the right language is:
- we likely have evidence for every early version
- we do **not yet** have every early version as a clean source snapshot

## Current Best Recovery Categories

### Fully represented

- `0.4.2-alpha`
- `0.4.3`
- `0.4.4`
- `0.4.7`
- `0.5.0`

These have strong git and/or source backup presence.

### Binary survives, source not yet cleanly mapped

- `0.4.0`
- `0.4.1`
- `0.4.5`
- `0.4.6`

These are prime archaeology targets.

### Textually referenced, likely reconstructable

- `0.3.0`

This one appears in early ChatGPT/versioned-install records and older `/opt` backup evidence, but it is not yet represented as a clean git-tagged source state in the current repo.

## Recommendation

The right next archaeology pass is not “rebuild everything immediately.”

It is:
1. extract the first generated KMiller skeleton and Quick Look variant into docs
2. inventory all early version references from raw ChatGPT export
3. compare those against:
   - git tags
   - `/opt/kmiller/versions`
   - backup folders
4. classify each version as:
   - full source survives
   - binary survives
   - partially reconstructable
   - text-only reference

That will tell us exactly which versions can be faithfully restored and which ones require reconstruction.
