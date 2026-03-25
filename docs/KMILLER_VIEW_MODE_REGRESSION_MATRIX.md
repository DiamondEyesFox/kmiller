# KMiller View Mode Regression Matrix

Last updated: 2026-03-06

## Purpose

This is the manual regression matrix for validating the four KMiller view modes after interaction refactors.

It exists to catch the exact class of regressions that are easy to miss in `Pane.cpp`:

- selection target drift
- rename/open inconsistencies
- context-menu target mismatch
- preview / Quick Look desync
- accidental flattening of Miller-specific behavior

## How To Use This Matrix

- Run each test in the named view mode.
- Mark each test as `PASS`, `FAIL`, `N/A`, or `INTENTIONAL DIFFERENCE`.
- For failures, note:
  - exact action
  - expected behavior
  - observed behavior
  - whether it reproduces in one view or multiple views

## Shared Expectations

These are expected across all non-Miller classic views unless explicitly noted:

- `Space` triggers Quick Look for the active item.
- `Ctrl+Enter` opens the active item.
- `Return` renames the active item.
- `F2` renames the active item.
- right-clicking an unselected item should switch selection to that item before opening the menu.
- context-menu actions should target the visible active selection.

## Miller Intentional Differences

These are expected differences and should not be treated as regressions:

- `Right` enters a directory column instead of opening files.
- `Left` collapses back one column.
- single-click on a directory navigates deeper in-column.
- `Return` renames the selected item.
- `Ctrl+Enter` opens or enters the selected item.
- selection/focus are column-based rather than a single classic-view list state.

---

## Icons View Tests (24)

1. Switch to Icons view and verify the current folder remains unchanged.
2. Single-click a file and verify the expected item becomes selected.
3. Single-click a folder and verify it becomes selected without opening immediately.
4. Double-click a folder and verify the pane enters that folder.
5. Double-click a file and verify it opens with the default app.
6. Press `Space` on a selected file and verify Quick Look opens the same item.
7. Press `Space` on a selected folder and verify Quick Look/preview behaves sensibly.
8. Press `Ctrl+Enter` on a selected folder and verify it opens/enters the folder.
9. Press `Ctrl+Enter` on a selected file and verify it opens the file.
10. Press `Return` on a selected file and verify inline rename begins.
11. Press `F2` on a selected folder and verify inline rename begins.
12. Slow second-click a selected item and verify rename begins.
13. Type printable text after selecting an item and verify slow-click rename is cancelled.
14. Right-click an unselected file and verify selection moves to that file before the menu opens.
15. Right-click selected empty space and verify the folder-level menu appears.
16. Use `Open` from the context menu on a folder and verify navigation occurs.
17. Use `Open With...` on a single file and verify the dialog targets the correct file.
18. Multi-select two files and verify `Open With...` is disabled or absent.
19. Toggle preview pane on and verify it seeds from the active item.
20. Change selection with preview visible and verify the preview updates.
21. Copy and paste a file in the same folder and verify the view refreshes correctly.
22. Create a new folder and verify it appears and can be renamed.
23. Move an item to trash and verify the selection and status update sanely.
24. Leave Icons view and return to it; verify the current folder is preserved.

## Details View Tests (24)

1. Switch to Details view and verify the current folder remains unchanged.
2. Click a row and verify the correct row becomes current and selected.
3. Use Up/Down arrows and verify current-row movement is visually clear.
4. Double-click a folder row and verify navigation occurs.
5. Double-click a file row and verify it opens.
6. Press `Space` on the current row and verify Quick Look targets that row.
7. Press `Ctrl+Enter` on the current folder row and verify navigation occurs.
8. Press `Ctrl+Enter` on the current file row and verify the file opens.
9. Press `Return` on the current row and verify rename begins.
10. Press `F2` on the current row and verify rename begins.
11. Slow second-click a selected row and verify rename begins.
12. Type printable text after selecting a row and verify slow-click rename is cancelled.
13. Right-click an unselected row and verify selection moves to that row first.
14. Right-click empty space below the rows and verify the folder-level menu appears.
15. Invoke `Open` from the context menu and verify it targets the selected/current row.
16. Invoke `Open With...` on a single file and verify the dialog file name is correct.
17. Multi-select rows and verify context-menu labels reflect multi-selection.
18. Toggle preview pane on and verify it seeds from the current row.
19. Change current row while preview is visible and verify preview updates.
20. Use header sort changes and verify selection/current row remain sane.
21. Copy and paste a file and verify the list refreshes without losing coherence.
22. Create a new folder and verify it appears in the list and can be selected.
23. Move an item to trash and verify current row fallback remains sane.
24. Leave Details view and return to it; verify the current folder is preserved.

## Compact View Tests (24)

1. Switch to Compact view and verify the current folder remains unchanged.
2. Single-click a file and verify the expected item becomes selected.
3. Single-click a folder and verify it becomes selected without opening.
4. Double-click a folder and verify navigation occurs.
5. Double-click a file and verify it opens.
6. Press `Space` on the active item and verify Quick Look targets it.
7. Press `Ctrl+Enter` on a folder and verify navigation occurs.
8. Press `Ctrl+Enter` on a file and verify it opens.
9. Press `Return` on the active item and verify rename begins.
10. Press `F2` on the active item and verify rename begins.
11. Slow second-click a selected item and verify rename begins.
12. Type printable text after selection and verify slow-click rename is cancelled.
13. Right-click an unselected item and verify selection moves first.
14. Right-click empty space and verify the folder-level menu appears.
15. Use `Open` from the context menu on a folder and verify navigation occurs.
16. Use `Open With...` on a single file and verify it targets the correct file.
17. Multi-select multiple items and verify context-menu labels match the count.
18. Toggle preview pane on and verify it seeds from the active item.
19. Change selection with preview visible and verify preview updates.
20. Copy and paste an item and verify the compact layout refreshes correctly.
21. Create a new folder and verify it appears in the current folder.
22. Move an item to trash and verify selection fallback stays sane.
23. Switch away from Compact and back; verify the current folder is preserved.
24. Verify zoom changes still feel coherent in Compact view after interaction tests.

## Miller Columns Tests (24)

1. Switch to Miller view and verify the current folder remains unchanged.
2. Single-click a folder in the active column and verify a deeper column appears.
3. Single-click a file and verify selection updates without creating a new column.
4. Press `Right` on a selected folder and verify it enters the folder.
5. Press `Right` on a selected file and verify nothing happens.
6. Press `Left` in a deeper column and verify the rightmost column collapses.
7. Press `Ctrl+Enter` on a selected folder and verify it enters that folder.
8. Press `Ctrl+Enter` on a selected file and verify it opens the file.
9. Press `Return` on a selected item and verify rename begins.
10. Press `F2` on a selected item and verify rename begins.
11. Slow second-click a selected item and verify rename begins.
12. Type printable text and verify type-to-select works in the active column.
13. Press `Space` on a selected file and verify Quick Look targets that file.
14. Verify preview pane updates when moving selection between columns.
15. Right-click an unselected item in a column and verify selection moves to it first.
16. Right-click empty space in a column and verify folder-level actions target that column's folder.
17. Use `Open` from the context menu on a selected file and verify it opens.
18. Use `Open` from the context menu on a selected folder and verify navigation occurs appropriately.
19. Use `Open With...` on a single file and verify the dialog targets the correct file.
20. Multi-select items in a column and verify context-menu labels reflect the count.
21. Create a new folder from a subcolumn empty-space menu and verify it is created in that specific folder.
22. Copy and paste into a folder column and verify the correct destination is used.
23. Navigate several columns deep, then move back left; verify stale columns do not remain.
24. Leave Miller view and return to it; verify the current folder is preserved.

---

## Minimum Release Gate

Before tagging a release after `Pane`/`MillerView` interaction refactors, run at minimum:

- Icons tests: 1, 6, 8, 10, 14, 17, 19
- Details tests: 1, 6, 8, 9, 13, 16, 18
- Compact tests: 1, 6, 8, 9, 13, 16, 18
- Miller tests: 1, 4, 5, 7, 9, 13, 15, 21, 22, 23
