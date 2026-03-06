# KMiller Visual Parity Checklist

This is a practical live-testing list for pushing KMiller toward standard desktop file manager parity while preserving its Finder-like identity.

## Core Navigation
- Breadcrumb path bar:
  - Click each segment to navigate to that exact ancestor.
  - Verify current folder segment updates after every navigation method.
  - Verify long paths do not clip in a confusing way.
- Typed path entry:
  - `Ctrl+L` opens `Go to Folder`.
  - `Ctrl+Shift+G` opens `Go to Folder`.
  - `Ctrl+L` toggles the dialog closed.
  - `Escape` closes the dialog.
  - Typing a valid path navigates there.
  - Typing an invalid path shows a clear error.
- Back / Forward / Up:
  - Toolbar buttons.
  - Go menu items.
  - Keyboard shortcuts.
  - Verify history is preserved after breadcrumb navigation and `Go to Folder` navigation.
- Places sidebar:
  - Single-click common locations.
  - Mounts and removable devices.
  - Recent locations / recent files.

## Selection And Activation
- Single click selects the expected item.
- Double click or `Open` opens files / enters folders as expected.
- `Ctrl+Down` / `Open Selection` behavior matches menu wording.
- Selection highlight is visually clear in all themes.
- Selection count and status bar update correctly.

## Context Menus
- File context menu:
  - labels make sense for single vs multi-select
  - actions are enabled/disabled correctly
- Folder context menu:
  - `Paste Into` / `Move Into` appears when appropriate
- Empty-space context menu:
  - offers only relevant folder-level actions
- Right-click should not desync focus from the selected item

## Open With
- `Open With...` appears only for single selection.
- Application list populates with sensible apps for common file types:
  - text
  - image
  - audio
  - video
  - PDF
  - unknown file
- Double-clicking an app launches it.
- `Open` button launches the selected app.
- Custom command flow:
  - plain command with no placeholders appends the file path
  - `%f` inserts local path
  - `%F` inserts local path
  - `%u` inserts URL string
  - `%U` inserts URL string
  - invalid command shows a clear error
- Dialog visuals:
  - file name is obvious
  - app list is readable
  - separators / fallback apps are not confusing
  - dialog size feels appropriate

## File Operations
- Cut / Copy / Paste
- Duplicate
- Rename inline
- Move to Trash
- Delete permanently
- New Folder
- Properties
- Compress / Extract
- Visual confirmation after each operation:
  - view refreshes
  - item appears/disappears/moves correctly
  - status bar updates

## Tabs And Multi-Location Work
- New tab opens sensible starting location.
- Close tab / close other tabs.
- Tab title updates after navigation.
- Right-click tab strip behavior.
- Opening folders in another tab if supported.

## View Modes
- Icons
- Details
- Compact
- Miller Columns
- Verify:
  - switching preserves current folder
  - selection remains sane
  - sorting works in each view
  - preview pane and zoom remain coherent

## Miller Columns Specific
- Opening folders grows/shifts columns correctly.
- Focus stays in the active column.
- Preview pane updates to current selection.
- Empty columns are not left behind after navigation.
- Right-click and keyboard navigation do not break column focus.

## Search And Filter
- Search field accepts input and filters correctly.
- Clearing search restores the full list.
- Search doesn't strand focus or selection in a bad state.

## Quick Look / Preview
- Space opens Quick Look.
- Preview pane reflects the selected file.
- Images, text, folders, media, and unsupported types all behave sensibly.
- Quick Look close / next / previous navigation works.

## Theme And Visual Polish
- Dark theme readability.
- Light theme readability.
- Finder theme readability.
- Dialogs should feel visually consistent with the selected theme.
- Focus rings, hover states, and disabled states should be readable.
- No ghost surfaces, clipped dialogs, or stray compositor artifacts mistaken for app UI.

## Windowing And Session Behavior
- Normal launch vs scratchpad launch.
- Scratchpad summon / dismiss behavior.
- Child dialogs stay on the correct workspace and monitor.
- Modal dialogs do not render offscreen or invisible.

## Errors And Recovery
- Invalid path warnings.
- Missing external app warnings.
- Failed file operation warnings.
- UI remains responsive after error dialogs.

## Release Gate
Before tagging a release, verify at minimum:
- Path bar + `Go to Folder`
- Open With
- Rename
- Copy / paste / move / duplicate / delete / trash
- New folder
- Tabs
- Quick Look
- Theme readability
- Scratchpad launch behavior
