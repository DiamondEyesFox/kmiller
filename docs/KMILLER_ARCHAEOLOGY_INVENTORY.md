# KMiller Archaeology Inventory

Date: 2026-03-06
Repo: `/mnt/steamssd/Documents/Development/kmiller`
Current tag: `v5.25.3`

## Summary

KMiller's early history is not lost.

The current git repo does not contain a complete tagged record of the earliest versions, but the filesystem still preserves multiple historical layers:

- current git history and tags
- installed historical binaries in `/opt/kmiller/versions`
- older development backups under `Documents/Development`
- archived release/binary copies under `~/releases`
- vault documentation and LLM log references

Most important finding:

- pre-`v0.4.2-alpha` is partially preserved
- there is an untagged repo bootstrap commit before `v0.4.2-alpha`
- real historical binaries exist for `0.4.0` and `0.4.1`
- a `0.3.0` binary exists inside an older `/opt/kmiller` backup tree

## Current Canonical Locations

### Active source repo
- `/mnt/steamssd/Documents/Development/kmiller`
- local branch: `main`
- current release tag: `v5.25.3`

### Convenience symlinks
- `/home/diamondeyesfox/kmiller` -> `Documents/Development/kmiller`
- `/home/diamondeyesfox/kmiller-5.24-hotfix` -> `Documents/Development/kmiller`

### Stable launcher path
- `/home/diamondeyesfox/.local/bin/kmiller`

## Git History Findings

### Earliest roots in current repo
`git rev-list --max-parents=0 --all` shows three roots:

- `6dd00bc` - start of current `v5.25+` line
- `2f1bdff` - `v0.5.0` root line
- `422ed18` - untagged `Initial commit`

### Earliest known commits in current repo log
- `422ed18` - `Initial commit`
- `295448f` - `KMiller v0.4.2 (Alpha Release)`
- `05f1442` - tagged `v0.4.2-alpha`

### Meaning
The current repo does preserve the start of the project, but the very first commit is only a tiny bootstrap commit, not a usable application release.

## Installed Historical Binaries

### Current `/opt/kmiller/versions`
Verified executable binaries exist for:

- `/opt/kmiller/versions/0.4.0/bin/kmiller`
- `/opt/kmiller/versions/0.4.1/bin/kmiller`
- `/opt/kmiller/versions/0.4.2/bin/kmiller`
- `/opt/kmiller/versions/0.4.4/bin/kmiller`
- `/opt/kmiller/versions/0.4.5/bin/kmiller`
- `/opt/kmiller/versions/0.4.6/bin/kmiller`
- `/opt/kmiller/versions/0.4.7/bin/kmiller`
- `/opt/kmiller/versions/0.5.0/bin/kmiller`
- `/opt/kmiller/versions/5.18/bin/kmiller`
- `/opt/kmiller/versions/5.19/bin/kmiller`
- `/opt/kmiller/versions/5.23/bin/kmiller`
- `/opt/kmiller/versions/5.24/bin/kmiller`

Also present:
- `/opt/kmiller/versions/0.4.2-rescue`
- `/opt/kmiller/stable/bin/kmiller`

### Older `/opt` backup tree
An older backup tree still contains:

- `/opt/kmiller.__backup.1756525609/versions/0.3.0/bin/kmiller`
- `/opt/kmiller.__backup.1756525609/versions/0.4.0/bin/kmiller`
- `/opt/kmiller.__backup.1756525609/versions/0.4.1/bin/kmiller`

This is the strongest evidence currently found for a surviving `0.3.0` binary.

## Development Backup Trees

Preserved backup container:
- `/mnt/steamssd/Documents/Development/kmiller.backup-20260306-053920`

Important subtrees:
- `kmiller-v0.4.3-stable-backup`
- `kmiller-v0.4.3++-backup-$(date +%Y%m%d-%H%M)`
- `kmiller copy`
- `Broken Kmiller Builds and Backups`

Notable artifacts:
- `kmiller3.zip`
- `kmiller3_quicklook.zip`
- `kmiller-v0.4.3-stable.tgz`
- several broken/mixed kmiller build directories

These backups appear to preserve the `0.4.x` line more explicitly than the modern repo structure does.

## Other Live/Recent Artifacts

### Side-by-side worktree used during archaeology/debugging
- `/home/diamondeyesfox/kmiller-5.25.2-worktree`

### Release bundle area
- `/home/diamondeyesfox/releases/kmiller-5.25`
- `/home/diamondeyesfox/releases/kmiller-5.25.tar.gz`
- `/home/diamondeyesfox/releases/kmiller-backups/...`

### System/user command paths
- `/usr/bin/kmiller`
- `/usr/local/bin/kmiller`
- `/home/diamondeyesfox/bin/kmiller`
- `/home/diamondeyesfox/Tools/bin/kmiller`

These should eventually be audited so there is one clearly authoritative launcher path.

## Vault Documentation and Archaeology Leads

Active vault project area:
- `/mnt/steamssd/Vault/DEF Master Vault/Master Vault/My Dashboard/Projects Dashboard/Creative/Active/KMiller`

Key docs already present there:
- `KMiller/KMiller Version History.md`
- `KMiller/KMiller v0.4.2 Backup.md`
- `KMiller/KMiller v0.4.3 Backup.md`
- `KMiller/KMiller v0.4.7 Backup.md`
- `KMiller/KMiller v0.5.0 Backup.md`
- `KMiller/KMiller v0.5.2 Backup.md`
- `KMiller/KMiller v0.5.3 Backup.md`

Chat/LLM archaeology also exists in:
- `Outside Vault Data/Vault Offloads/LLM Logs/ChatGPT Logs/2025/08/...`
- old ALICE session logs for August 2025
- `.smart-env` mirrored vault references

## Early-History Conclusion

### What is definitely preserved
- repo bootstrap commit before `v0.4.2-alpha`
- `v0.4.2-alpha` and later git history
- historical binaries for `0.4.0`, `0.4.1`, and `0.4.2+`
- a `0.3.0` binary in an `/opt` backup tree
- multiple backup source trees and archives for early versions

### What is not yet proven
- whether `0.3.0` source is still available in a clean repo form
- whether anything earlier than `0.3.0` ever existed as a real KMiller build
- exact project birthday / genesis moment

## Best Next Archaeology Steps

1. Inspect Claude Code history for the first creation/build prompts.
2. Inspect the August 2025 ChatGPT logs in the vault for first mentions and upload/build events.
3. Compare `/opt/kmiller/versions/0.4.0` and `0.4.1` against backup source trees.
4. Search backup archives for `0.3.0` source, not just the binary.
5. Build a formal timeline doc with:
   - first mention
   - first build
   - first runnable version
   - first GitHub push
   - release/tag milestones
