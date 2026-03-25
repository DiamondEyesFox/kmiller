# KMiller2 — KDE File Manager with Icons/List/Compact + Miller Columns + Quick Look

A daily-driver, KDE-native file manager with multiple view modes (**Icons**, **Details**, **Compact**, **Miller Columns**) and **Quick Look** previews (Space to open, ESC to close).

## Features in this build
- **View switcher**: Icons / Details / Compact / Miller Columns
- **Quick Look** (Space/ESC): Images, PDFs, Text, Audio/Video
- **Tabs** (Ctrl+T/Ctrl+W), basic session remembering per tab path
- **Places sidebar** (Home, Devices, Trash, Network) via KDE `KFilePlacesView`
- **Path bar** (editable line) + Back/Forward/Up
- **Search filter** in-folder (live filtering for Icons/Details/Compact)
- **Sorting** (Name, Size, Type, Date; ascending/descending; folders first)
- **Drag & drop**, rename (F2), delete to trash (Del), show hidden (Ctrl+H)

> Note: Split view, undo/redo journal, Baloo-indexed search, and plugin hooks can be added next.

## Build & Run (Arch/KDE)
```bash
sudo pacman -S --needed qt6-base qt6-multimedia kio poppler-qt6 cmake make gcc ffmpeg unzip
mkdir -p ~/src && cd ~/src
unzip kmiller2.zip -d .
cd kmiller2
mkdir build && cd build
cmake ..
make
./kmiller2
```

## Usage
- **Sidebar**: click Places to jump to locations (mounted devices, network, trash).
- **Navigate**: double-click or Enter to open; Back/Forward/Up in toolbar.
- **Search**: type in the search box to filter current folder (except Miller view).
- **View modes**: pick from the combo in the toolbar.
- **Quick Look**: select a file, press **Space**; **ESC** closes. Arrow keys move selection while preview is open.
- **Shortcuts**: F2 rename, Ctrl+L edit path, Ctrl+T/W tab new/close, Ctrl+H show hidden.

## License
MIT (no warranty).
