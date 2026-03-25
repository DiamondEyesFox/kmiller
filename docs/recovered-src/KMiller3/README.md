# KMiller3 — Dolphin-level KDE File Manager + Miller Columns + Quick Look

This build adds **KIO/KDirModel** for full KDE integration (SMB/SFTP/trash), a **breadcrumb path bar**, **split view**, **zoom slider**, **undo/redo**, **trash** actions, **Open With...**, and **KDE thumbnailers** in icon view — alongside **Miller Columns** and **Quick Look**.

## Features
- **Views**: Icons, Details, Compact, **Miller Columns**
- **Quick Look**: Space/ESC; images, PDFs, text, audio/video
- **KIO**: open `smb:/`, `sftp:/`, `trash:/`, network shares
- **Breadcrumb path bar** (`KUrlNavigator`) + editable
- **Split view** (two panes; toggle via toolbar)
- **Tabs per window** (Ctrl+T/Ctrl+W)
- **Undo/Redo** via `KIO::FileUndoManager`
- **Trash**: Move to Trash, Open Trash (trash:/), Empty Trash (simple)
- **Open With…** via `KIO::FileItemActions`
- **KDE thumbnailers** for icon view via `KIO::PreviewJob`
- **Zoom slider** for icon sizes
- **Sorting**; show hidden; inline rename; drag & drop

> Note: Archive integration uses your system "ark" if installed (via OpenUrlJob). Advanced grouping, batch rename UI, multi-page PDF controls can be added.

## Build (Arch/KDE)
```bash
sudo pacman -S --needed qt6-base qt6-multimedia kio kio-extras poppler-qt6   kf6-i18n kf6-config cmake make gcc ffmpeg unzip
unzip kmiller3.zip -d .
cd kmiller3
mkdir build && cd build
cmake ..
make
./kmiller3
```

## Usage
- **View switcher** in toolbar; **zoom** slider for icons.
- **Split** via toolbar button; panes navigate independently.
- **Sidebar** (Places) shows devices, network, trash; remote URLs open.
- **Context menu**: Open, Open With…, Rename (F2), Move to Trash, Delete, Properties.
- **Quick Look**: select file and press Space; ESC closes.

## License
MIT (no warranty).
