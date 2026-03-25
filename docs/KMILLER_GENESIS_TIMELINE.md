# KMiller Genesis Timeline

Last updated: 2026-03-06

## Purpose

This document reconstructs the earliest known history of KMiller using:
- local git history
- historical installed binaries under `/opt/kmiller`
- backup source trees
- exported ChatGPT logs from 2025
- the raw ChatGPT export archive under `~/Downloads/Chatgpt Exports/`

It is meant to answer three questions:
1. When did KMiller first exist as an idea/project?
2. When did it become a working application?
3. What is the best candidate for KMiller's "birthday"?

## Short Answer

KMiller almost certainly existed before the first currently available git commit.

Best known milestone candidates:
- First known literal `kmiller` naming moment in raw ChatGPT export: **August 7, 2025, 10:51:18 PM CDT**
- First known user genesis prompt in raw ChatGPT export: **August 7, 2025, 10:48:48 PM CDT**
- First git repo bootstrap commit: **August 30, 2025, 6:37:24 PM CDT**
- First working prototype commit: **August 30, 2025, 6:58:57 PM CDT**
- First tagged release (`v0.4.2-alpha`): **August 30, 2025, 7:06:34 PM CDT**

If one date must be chosen as KMiller's practical birthday, the strongest candidate is:
- **August 30, 2025, 6:58:57 PM CDT**
- commit: `295448f`
- reason: this is the first surviving commit that explicitly says it is the "first working prototype of kmiller"

If one date must be chosen as KMiller's idea/naming birthday, the strongest candidate is:
- **August 7, 2025, 10:48:48 PM CDT**
- raw ChatGPT conversation title: `Finder alternatives for Linux`
- reason: this is the first surviving user prompt asking ChatGPT to create the app, immediately followed by the assistant coining `KMillerFM` / `kmiller`

## Raw ChatGPT Export Breakthrough

The standard month-folder markdown exports were not the whole story. The decisive source turned out to be the raw ChatGPT export zip:

- `~/Downloads/Chatgpt Exports/a6902e2fc401942889c85ad001cf3a2877ac73034f3a4f28be33980249dbfb1e-2026-02-25-23-59-59-9d0d8dbb4c704d88a01930860568b3e0.zip`

Important note:
- this export does **not** use a single `conversations.json`
- it is split into `conversations-000.json` through `conversations-047.json`
- KMiller first appears in `conversations-024.json`

Relevant conversation:
- title: `Finder alternatives for Linux`
- id: `68957385-6578-832d-9162-7e565fff2c47`
- create time: `2025-08-08T03:48:48.764397+00:00`

In US/Central, that is:
- **2025-08-07 10:48:48 PM CDT**

## Earliest Surviving Genesis Prompt

### 2025-08-07 22:48:48 CDT
Source:
- raw ChatGPT export conversation `Finder alternatives for Linux`

Earliest user message in that surviving thread:

> remember how i wanted a finder like file manager with true miller columns and quicklook?

This is immediately followed by:
- a short exchange about Linux alternatives
- the user saying:

> just a yes or no wouldve sufficed, because im going to ask you to create it

- then:

> create it

This is the strongest surviving genesis prompt we have found so far.

## Earliest Literal `kmiller` Naming Moment

### 2025-08-07 22:51:18 CDT
Source:
- same raw ChatGPT export conversation

First surviving literal `kmiller` usage appears in the assistant's generated skeleton code:

- `project(KMillerFM LANGUAGES CXX)`
- `add_executable(kmiller ...)`

This strongly suggests:
- the project name was coined in this conversation
- or at minimum, this is the earliest surviving record of the name being used

At present, this is earlier than every month-folder markdown export mention.

## Earliest Known Evidence Before Git

### 2025-08-29 11:49:48 CDT
Source:
- `Outside Vault Data/Vault Offloads/LLM Logs/ChatGPT Logs/2025/08/29/2025-08-29 11-49-48 - Latest kmiller build_md.md`

Key clue:
- user asks for the "latest kmiller build"
- the conversation references an earlier expired session/canvas where KMiller had already been vibecoded

What this proves:
- KMiller existed before this exported chat
- by late morning on **August 29, 2025**, the project already had at least one earlier build/session
- this is no longer the earliest known mention; it is now the earliest surviving **month-folder markdown export** mention

What it does not prove:
- exact first creation time
- exact initial version number

## Early Versioned-Install Era

### 2025-08-30 01:00:48 CDT
Source:
- `Outside Vault Data/Vault Offloads/LLM Logs/ChatGPT Logs/2025/08/30/2025-08-30 01-00-48 - Versioned installs recap_md.md`

Important evidence:
- project path referenced as `~/Downloads/kmiller/`
- build path referenced as `~/Downloads/kmiller/build/`
- versioned-install design discussed for `/opt/kmiller/versions/<version>/`
- examples reference versions including `0.3.0`, `0.4.0`, and `0.4.1`

What this proves:
- by early **August 30, 2025**, KMiller had already advanced through multiple version numbers
- the versioned install/archive strategy was already being actively designed
- `/opt/kmiller` is not an accident or later invention; it is part of the early project story

## Git History: First Surviving Repo Milestones

### 2025-08-30 18:37:24 CDT
Commit:
- `422ed18` `Initial commit`

What survives:
- only `README.md`

Meaning:
- this is the earliest surviving git commit in the current repo
- it is best treated as the repo bootstrap, not the first meaningful application snapshot

### 2025-08-30 18:58:57 CDT
Commit:
- `295448f` `KMiller v0.4.2 (Alpha Release)`

Commit message says:
- "first working prototype of kmiller"
- "Proof of concept: includes basic Miller Columns view and Quick Look preview."

Why this matters:
- this is the first surviving commit that clearly represents a working application
- it already includes the two defining concepts of KMiller:
  - Miller Columns
  - Quick Look

### 2025-08-30 19:06:34 CDT
Commit/tag:
- `05f1442` tagged `v0.4.2-alpha`

Meaning:
- first surviving tagged KMiller release
- this is the earliest clean release marker in current git history

## Evidence That Pre-0.4.2 Versions Survive

The current repo's tagged history begins at `v0.4.2-alpha`, but older KMiller versions do survive elsewhere.

### Installed version archive
Under `/opt/kmiller/versions/` we have surviving installed binaries for:
- `0.4.0`
- `0.4.1`
- `0.4.2`
- later versions

In an older `/opt` backup tree we also found:
- `0.3.0`

This means pre-`0.4.2` KMiller was real and is not lost to time.

### Backups and archives
Additional evidence exists in:
- `Documents/Development/kmiller.backup-*`
- KMiller backup/source archives
- Vault version-history and backup notes

## Best Birthday Candidates

### 1. Idea / genesis birthday
**2025-08-07 22:48:48 CDT**

Pros:
- earliest surviving user prompt that clearly initiates KMiller creation
- captures the actual desire:
  - Finder-like
  - true Miller columns
  - Quick Look
- immediately leads into assistant-generated project code

Cons:
- the literal name `kmiller` appears a couple minutes later in the assistant response, not in the first user sentence

### 2. Naming birthday
**2025-08-07 22:51:18 CDT**

Pros:
- earliest surviving literal `kmiller` appearance
- likely the naming moment

Cons:
- assistant-originated naming, not necessarily user-originated

### 3. Repo birthday
**2025-08-30 18:37:24 CDT**

Commit:
- `422ed18`

Pros:
- first surviving git commit

Cons:
- only bootstrap README state; not yet a meaningful app snapshot

### 4. Working prototype birthday
**2025-08-30 18:58:57 CDT**

Commit:
- `295448f`

Pros:
- strongest practical birthday candidate
- explicitly described as the first working prototype
- already contains the core identity of KMiller

Cons:
- does not capture any earlier pre-git or lost-session prototype work

### 5. First public/release birthday
**2025-08-30 19:06:34 CDT**

Tag:
- `v0.4.2-alpha`

Pros:
- first clear release milestone
- easiest date to communicate publicly

Cons:
- not the earliest working form

## Recommendation

Use this internally:
- **KMiller's surviving origin story now clearly begins on August 7, 2025 in the raw ChatGPT export, and predates the current git repo by over three weeks.**

Use this as the idea/genesis date:
- **August 7, 2025 at 10:48:48 PM CDT**

Use this as the naming date:
- **August 7, 2025 at 10:51:18 PM CDT**

Use this as the practical birthday:
- **August 30, 2025 at 6:58:57 PM CDT**
- first working prototype commit `295448f`

Use this as the first release date:
- **August 30, 2025 at 7:06:34 PM CDT**
- first tag `v0.4.2-alpha`

## Open Questions

Still worth investigating later:
1. Can any older ChatGPT export prior to the February 2026 zip push the genesis earlier than August 7, 2025?
2. Can the `/opt/kmiller` install archives or backup tarballs be matched back to precise source snapshots?
3. Was `0.3.0` the first runnable build, or were there even earlier unnamed prototypes?
