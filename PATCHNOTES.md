# KMiller File Manager - Release Notes

## Version 5.25.4
**Released: April 2026**

### Daily-Driver Parity Improvements
- Added a release-readiness pass focused on reducing Dolphin/Thunar fallback moments.
- Centralized pane navigation history through `PaneNavigationState`.
- Added `Show in Parent Folder` for single local selections.
- Improved recursive search wording, tooltip, and active visual state.
- Standardized Open With dialog presentation and custom command prompting.
- Added a GIO MIME-association fallback so Open With can populate from the same shared handler data Thunar sees when KDE's service cache is stale.
- Moved Open With app discovery and app launching into `OpenWithService`, keeping the pane UI stable while reducing `Pane.cpp` responsibility.

### Settings Truthfulness
- Miller column width setting now applies to existing and newly-created Miller columns.
- Follow symbolic links now affects directory symlink navigation in Pane and Miller views.
- Hid the unimplemented file-extension setting until display behavior is real.

### File Operation Trust
- Paste/move refreshes destination and source parent folders.
- Duplicate now routes through `FileOpsService::copyAs()`.
- Duplicate success uses status-bar feedback instead of a modal.
- Copy, move, trash, permanent delete, and new-folder actions report status-bar success/error feedback.
- Inline rename cleanup no longer blanket-disconnects delegate `closeEditor` handlers.

### QA
- Added `--path <folder>` for opening KMiller against disposable fixtures.
- Added `--qa-fixture <folder>` for noninteractive file-operation checks.
- Added `--qa-ui-logic <folder>` for initial-path and symlink-follow checks.
- Added `--qa-open-with <file>` for noninteractive Open With app-discovery checks.
- Added `--qa-archive <folder>` for noninteractive archive create/extract/conflict checks.
- Expanded archive QA with hostile `../`, nested traversal, and absolute-path entries; extraction must keep them contained inside the target folder.
- Expanded fixture operations QA to cover rename and permanent delete on disposable files.
- Added `tools/qa_headless.sh` for fixture operations, UI logic checks, and Xvfb GUI smoke.
- Added `tools/kmiller_manual_qa_harness.py`, a PyQt6 floating manual QA coach for Cody's disposable test folder.
- Added `tools/run_manual_qa_harness.sh` as a convenience launcher for the guided manual QA harness.

---

## Version 5.25 (Current)
**Released: December 2025**

### Improvements
✨ **Native Archive Backend**: Common archive create/extract flows now use `KArchive` instead of shelling out to external tools
- Added built-in support for `.zip`, `.7z`, `.tar`, `.tar.gz`, `.tar.bz2`, and `.tar.xz`
- Archive operations now run off the UI thread with a real progress dialog instead of blocking process wrappers
- Compression now lets you choose the archive format explicitly instead of forcing `.zip`
- Archive context menus now offer `Extract Here`, `Extract to New Folder`, and `Extract To...`
- Extraction now preflights built-in archive contents and asks before replacing existing files
- Search now shows recursive results for the current folder tree instead of only filtering the open directory
- Properties now calculate folder sizes in the background for single folders and multi-selection summaries
- `rar` remains extract-only and uses `7z` or `unrar` as a narrow fallback when available

### Bug Fixes
🐛 **FileChooser URI Return**: Fixed file picker not returning selected file URIs to applications
- Dialog was returning folder URL instead of selected file URLs
- Now correctly returns selected file(s) via `getSelectedUrls()`

---

## Version 5.24
**Released: December 2025**

### Improvements
✨ **Full Pane in File Picker**: FileChooser dialog now uses complete Pane widget instead of stripped-down MillerView
✨ **Places Sidebar**: Added KFilePlacesView sidebar for quick navigation to common locations
✨ **Preview Enabled**: File preview pane enabled by default in picker dialogs
✨ **Auto-Focus**: Pane receives focus automatically when dialog opens

---

## Version 5.23
**Released: December 2025**

### Major Features
✨ **FileChooser Portal**: kmiller can now be used as the system file picker via xdg-desktop-portal
✨ **Portal Mode**: Run with `--portal` flag to act as D-Bus FileChooser backend
✨ **Miller Columns File Picker**: Full Miller columns UI in file dialogs (Open, Save, Save As)

### Technical Details
- Implements org.freedesktop.impl.portal.FileChooser D-Bus interface
- Supports file filters, multiple selection, directory mode
- Auto-activates via D-Bus when configured as preferred portal
- New files: FileChooserPortal.cpp/h, FileChooserDialog.cpp/h
- Portal registration: kmiller.portal, D-Bus service file

### Installation
To use kmiller as your system file picker:
```bash
# Edit ~/.config/xdg-desktop-portal/portals.conf
org.freedesktop.impl.portal.FileChooser=kmiller

# Restart portal service
systemctl --user restart xdg-desktop-portal
```

---

## Version 5.22
**Released: December 2025**

### Features
- Properties dialog with permissions editing
- Settings dialog with theme/view preferences
- Enhanced QuickLook with navigation
- Improved context menus
- Archive compress/extract support

---

## Version 5.19
**Released: December 2025**

### Features
- Multi-tab support
- Improved drag & drop
- Status bar with selection info
- Zoom slider for icon sizes

---

## Version 5.18
**Released: December 2025**

### Features
- Complete file manager with Miller columns
- Preview pane (images, PDF, text)
- Multi-view modes (Icons, Details, Compact, Miller)
- Full file operations (copy, cut, paste, delete, rename, duplicate)
- Linux desktop integration
- Thumbnail caching

---

## Earlier Versions (0.4.x - 0.5.x)
Development versions establishing core functionality.

---

## System Requirements
- **OS**: Linux
- **Dependencies**: Qt6 (Widgets, Multimedia, DBus), KDE Frameworks 6, Poppler-Qt6
- **Optional**: `7z` or `unrar` for `rar` extraction fallback

## Installation
KMiller uses versioned installation in `/opt/kmiller/versions/` with symlinks for easy rollback.

---
*Generated with [Claude Code](https://claude.ai/code)*
