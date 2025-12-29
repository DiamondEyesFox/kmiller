# KMiller Roadmap: 100 Proposed Changes

## Bug Fixes

1. **Fix double-click on file sometimes not opening** - Race condition between click and double-click handlers
2. **Fix preview pane not updating when switching tabs** - Selection state not synced across tabs
3. **Fix hidden files toggle not persisting across sessions** - Setting saved but not applied on reload
4. **Fix keyboard navigation breaking after rename operation** - Focus lost after editor closes
5. **Fix context menu appearing in wrong position on multi-monitor setups** - Global vs local coordinates
6. **Fix Places sidebar not updating when drives mount/unmount** - Missing refresh signal
7. **Fix zoom slider affecting wrong pane in multi-tab scenarios** - Slider connected to wrong pane
8. **Fix memory leak when rapidly switching directories** - Models not properly cleaned up
9. **Fix clipboard not clearing after paste** - Cut files still marked after paste
10. **Fix sort order not preserved when toggling hidden files** - Model reset clears sort

## Performance Improvements

11. **Lazy-load directory contents** - Don't load all columns at once for deep paths
12. **Cache thumbnail generation results** - Avoid regenerating on every view
13. **Defer icon loading for off-screen items** - Only load visible items
14. **Implement virtual scrolling for large directories** - Handle 10k+ files smoothly
15. **Background thread for file operations** - Don't block UI during copy/move
16. **Optimize QFileSystemModel watcher usage** - Reduce filesystem polling
17. **Pool QListView instances for Miller columns** - Reuse instead of recreate
18. **Compress thumbnail cache** - Reduce disk usage
19. **Batch file stat operations** - Reduce syscalls for large directories
20. **Preload adjacent directories** - Anticipate navigation

## UI/UX Improvements

21. **Add breadcrumb path bar option** - Clickable path segments like modern Finder
22. **Add column width persistence** - Remember Miller column widths
23. **Add smooth scrolling animations** - Polish for navigation
24. **Add selection count in status bar** - "5 of 127 items selected"
25. **Add file size totals in status bar** - Show selected files total size
26. **Add visual feedback for drag operations** - Highlight drop targets
27. **Add column resize handles** - Drag to resize Miller columns
28. **Add alternating row colors option** - Easier to read long lists
29. **Add file extension coloring** - Color-code by type
30. **Add icon badges for symlinks/aliases** - Visual indicator for links

## Finder Parity Features

31. **Add Quick Actions** - Right-click actions for common operations
32. **Add Tags support** - Colored tags like Finder
33. **Add Stacks** - Group files by type/date on desktop
34. **Add Gallery view** - Large thumbnails with metadata sidebar
35. **Add Column view preview column** - Full preview in rightmost column
36. **Add "Show View Options" dialog** - Per-folder view customization
37. **Add "Clean Up" and "Arrange By"** - Icon view organization
38. **Add "Get Info" panel** - Detailed file properties with icon preview
39. **Add Spotlight-style search** - Full-text search with filters
40. **Add Smart Folders** - Saved searches that update dynamically

## Navigation Features

41. **Add tab bar drag reordering** - Drag tabs to reorder
42. **Add tab duplication** - Duplicate current tab
43. **Add split pane view** - Two panes side by side
44. **Add recent locations dropdown** - Quick access to history
45. **Add favorites in path bar** - Pin folders to path bar
46. **Add "Go to Folder" dialog (Cmd+Shift+G)** - Type path directly
47. **Add enclosing folder shortcut** - Jump to parent
48. **Add "Connect to Server" dialog** - SMB/SFTP/FTP connections
49. **Add sidebar section collapsing** - Collapse Places categories
50. **Add custom sidebar items** - Drag folders to sidebar

## File Operations

51. **Add conflict resolution dialog** - Options when file exists
52. **Add progress dialog for long operations** - Copy/move progress
53. **Add operation queue** - Queue multiple copy/move operations
54. **Add undo for file operations** - Undo delete/move/rename
55. **Add batch rename tool** - Rename multiple files with patterns
56. **Add duplicate file finder** - Find and manage duplicates
57. **Add secure delete option** - Overwrite before delete
58. **Add file comparison** - Compare two files/folders
59. **Add create alias/symlink** - Right-click to create link
60. **Add "Move to..." and "Copy to..." dialogs** - Quick move/copy

## Preview & Thumbnails

61. **Add video thumbnail generation** - Preview frames for videos
62. **Add PDF multi-page preview** - Scroll through pages
63. **Add audio waveform preview** - Visualize audio files
64. **Add code syntax highlighting** - Colored code preview
65. **Add markdown rendering** - Render markdown in preview
66. **Add archive contents preview** - Show zip/tar contents
67. **Add font preview** - Preview font files with sample text
68. **Add 3D model preview** - Basic 3D file preview
69. **Add EXIF data display** - Show photo metadata
70. **Add Office document preview** - Preview docx/xlsx/pptx

## Keyboard & Shortcuts

71. **Add customizable keyboard shortcuts** - User-defined hotkeys
72. **Add vim-style navigation option** - hjkl navigation
73. **Add numeric keypad quick nav** - Jump to nth item
74. **Add Ctrl+1-4 for view switching** - Quick view mode change
75. **Add Cmd+[ and Cmd+] for back/forward** - Browser-style navigation
76. **Add Cmd+Up for enclosing folder** - Finder standard
77. **Add Cmd+Down for open** - Finder standard
78. **Add Cmd+Shift+. for toggle hidden** - Finder standard
79. **Add F2 for rename** - Windows standard (in addition to Enter)
80. **Add keyboard-driven file selection** - Shift+arrows to extend selection

## Search & Filtering

81. **Add real-time search/filter** - Filter as you type
82. **Add search scope options** - Current folder vs all subfolders
83. **Add file type filters** - Quick filter by type
84. **Add date range filters** - Filter by modification date
85. **Add size filters** - Filter by file size
86. **Add saved searches** - Save search queries
87. **Add regex search option** - Power user search
88. **Add content search** - Search file contents
89. **Add search results view** - Dedicated results pane
90. **Add search history** - Recent searches dropdown

## Settings & Configuration

91. **Add per-folder view settings** - Remember view mode per folder
92. **Add startup folder option** - Choose default start location
93. **Add confirm before delete toggle** - Optional confirmation
94. **Add single-click to open option** - Alternative to double-click
95. **Add show file extensions toggle** - Hide known extensions
96. **Add date format customization** - Choose date display format
97. **Add size format options** - KB/MB/GB vs KiB/MiB/GiB
98. **Add font/size customization** - Custom list fonts
99. **Add import/export settings** - Backup preferences
100. **Add command-line arguments** - Open specific path, etc.

---

# Priority Analysis

After deep consideration of user impact, implementation complexity, and alignment with the "classic Finder" vision, here are the recommended priorities:

## Tier 1: Critical (Do First)

These have high user impact and relatively low complexity:

| # | Item | Why |
|---|------|-----|
| 76 | Cmd+Up for enclosing folder | Finder muscle memory, trivial to add |
| 77 | Cmd+Down for open | Finder muscle memory, trivial to add |
| 78 | Cmd+Shift+. for toggle hidden | Finder standard, trivial |
| 24 | Selection count in status bar | High visibility, easy win |
| 25 | File size totals in status bar | Very useful, straightforward |
| 46 | "Go to Folder" dialog | Essential power user feature |
| 3 | Fix hidden files persistence | Bug that annoys users |
| 4 | Fix keyboard nav after rename | Breaks flow, noticeable |

## Tier 2: High Value (Do Soon)

Significant improvements with moderate effort:

| # | Item | Why |
|---|------|-----|
| 21 | Breadcrumb path bar | Modern UX expectation |
| 35 | Column view preview column | Core Finder feature missing |
| 51 | Conflict resolution dialog | Essential for file ops |
| 52 | Progress dialog | Essential for large copies |
| 54 | Undo for file operations | Safety net users expect |
| 55 | Batch rename tool | Huge productivity boost |
| 64 | Code syntax highlighting | Developer-friendly |
| 81 | Real-time search/filter | Modern expectation |
| 22 | Column width persistence | Polish that matters |

## Tier 3: Nice to Have (Backlog)

Good features but lower priority:

| # | Item | Why |
|---|------|-----|
| 32 | Tags support | Great but complex (needs xattr) |
| 39 | Spotlight-style search | Complex, needs indexing |
| 40 | Smart Folders | Complex, needs query engine |
| 43 | Split pane view | Useful but significant work |
| 70 | Office document preview | Needs external libs |

## Recommended Implementation Order

1. **v5.18**: Keyboard shortcuts (76-78), status bar improvements (24-25)
2. **v5.19**: Bug fixes (3, 4, 9, 10), "Go to Folder" dialog (46)
3. **v5.20**: Breadcrumb path bar (21), column preview (35)
4. **v5.21**: File operation dialogs (51, 52), undo (54)
5. **v5.22**: Batch rename (55), real-time filter (81)
6. **v6.0**: Major features - Tags (32), Smart Folders (40), Split pane (43)

## Quick Wins (< 1 hour each)

These can be done anytime as warmups:
- #76, #77, #78 (keyboard shortcuts)
- #24, #25 (status bar text)
- #23 (smooth scrolling - just add animation)
- #79 (F2 for rename)
- #91 could be started with just saving view mode to QSettings per path

## Architecture Notes

Some features require foundational work:
- **Tags (#32)** needs extended attribute (xattr) support
- **Smart Folders (#40)** needs a query language and indexer
- **Content search (#88)** needs background indexing service
- **Undo (#54)** needs an operation history stack

Consider building these foundations early even if full features come later.
