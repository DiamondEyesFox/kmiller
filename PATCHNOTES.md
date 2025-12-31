# KMiller File Manager - Release Notes

## Version 5.25 (Current)
**Released: December 2025**

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
- KDE Frameworks integration
- Thumbnail caching

---

## Earlier Versions (0.4.x - 0.5.x)
Development versions establishing core functionality.

---

## System Requirements
- **OS**: Linux (KDE/Qt6 recommended)
- **Dependencies**: Qt6 (Widgets, Multimedia, DBus), KDE Frameworks 6, Poppler-Qt6
- **Optional**: zip/unzip, tar, 7z, unrar for archive support

## Installation
KMiller uses versioned installation in `/opt/kmiller/versions/` with symlinks for easy rollback.

---
*Generated with [Claude Code](https://claude.ai/code)*
