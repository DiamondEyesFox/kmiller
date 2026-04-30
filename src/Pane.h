#pragma once
#include "PaneNavigationState.h"

#include <QWidget>
#include <QUrl>
#include <QPoint>
#include <QAbstractProxyModel>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QPersistentModelIndex>
#include <QPointer>

class QTimer;
class QToolBar;
class QComboBox;
class QSlider;
class QStackedWidget;
class QListView;
class QTreeView;
class QSplitter;
class QLabel;
class QTextEdit;
class QAbstractItemView;

class KUrlNavigator;
class MillerView;
class QuickLookDialog;
class ThumbCache;
class KDirModel;
class KDirSortFilterProxyModel;
class KFilePreviewGenerator;

class Pane : public QWidget {
    Q_OBJECT

signals:
    void statusChanged(int totalFiles, int selectedFiles, qint64 selectedSize, const QString &extraInfo = QString());
    void urlChanged(const QUrl &url);

public:
    void showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls = QList<QUrl>());
    explicit Pane(const QUrl &startUrl, QWidget *parent = nullptr);

    void setRoot(const QUrl &url);
    void setUrl(const QUrl &url);
    QUrl currentUrl() const { return currentRoot; }

    // View & navigation used by MainWindow
    enum ViewMode { Icons = 0, Details = 1, Compact = 2, Miller = 3 };
    void setViewMode(int idx);
    int currentViewMode() const;
    void goUp();
    void goHome();
    void goBack();
    void goForward();
    void openSelected();
    bool canGoBack() const;
    bool canGoForward() const;
    
    // Sorting
    void setSortCriteria(int criteria);  // 0 Name, 1 Size, 2 Type, 3 Date
    void setSortOrder(Qt::SortOrder order);

    // Context menu usable by both Miller and the other views

    // Quick Look convenience
    void quickLookSelected();

    // File operations
    void copySelected();
    void cutSelected();
    void pasteFiles();
    void deleteSelected();
    void deleteSelectedPermanently();
    void moveToTrash();
    void renameSelected();
    void duplicateSelected();
    void createNewFolder();
    QList<QUrl> selectedUrls() const;

    // Preview pane
    void setPreviewVisible(bool on);
    bool previewVisible() const { return m_previewVisible; }
    
    // Hidden files
    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const { return m_showHiddenFiles; }

    // Settings truthfulness — propagate saved settings to live panes
    void setShowThumbnails(bool show);
    void setShowFileExtensions(bool show);
    void setMillerColumnWidth(int width);
    void setFollowSymlinks(bool follow);
    bool canNavigateIntoDirectoryForTesting(const QUrl &url) const { return shouldNavigateIntoDirectory(url); }
    
    // Status updates
    void updateStatus();
    
    // Zoom control
    void setZoomValue(int value);

    // Focus the active view
    void focusView();

    // Refresh directory listing (e.g. after undo/redo)
    void refreshCurrentLocation();

    // Return the file path of the item at offset from currentPath in the active view's model order.
    // offset=+1 for next, -1 for previous. Returns empty string if at boundary.
    QString adjacentFilePath(const QString &currentPath, int offset) const;

    // Whether the current view is Miller columns (Quick Look uses this to
    // disable left/right arrow navigation which means column nav in Miller).
    bool isMillerViewActive() const;

    // Select a file by path in the active view (used by Quick Look to sync selection).
    void selectFileInView(const QString &filePath);



private slots:
    void onViewModeChanged(int idx);
    void onZoomChanged(int val);
    void onNavigatorUrlChanged(const QUrl &url);

    void onActivated(const QModelIndex &idx);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void syncNavigatorLocation(const QUrl &url);
    void applyLocation(const QUrl &url);
    void applyIconSize(int px);
    QUrl urlForIndex(const QModelIndex &proxyIndex) const;
    QList<QUrl> getSelectedUrls() const;
    void showHeaderContextMenu(const QPoint &pos);
    void showEmptySpaceContextMenu(const QPoint &pos, const QUrl &targetFolder = QUrl());
    
    void generateThumbnail(const QUrl &url) const;
    QIcon getIconForFile(const QUrl &url) const;

    void updatePreviewForUrl(const QUrl &u);
    void clearPreview();
    
    void showOpenWithDialog(const QUrl &url);
    void compressSelected();
    void extractArchive(const QUrl &archiveUrl);
    void extractArchiveHere(const QUrl &archiveUrl);
    void extractArchiveToNewFolder(const QUrl &archiveUrl);
    void runArchiveExtraction(const QUrl &archiveUrl, const QString &extractDir, const QUrl &selectUrlOnSuccess = QUrl());
    void createNewFolderIn(const QUrl &targetFolder);
    void createNewFileIn(const QUrl &targetFolder);
    void beginInlineRename(QAbstractItemView *view, const QModelIndex &index);
    void pasteFilesToDestination(const QUrl &destination);
    bool isClipboardCutOperation() const;
    void openTerminalAt(const QUrl &targetFolder);
    void updateEmptyFolderOverlay();
    bool shouldNavigateIntoDirectory(const QUrl &url) const;
    void refreshLocation(const QUrl &url);

    QToolBar *tb = nullptr;
    QComboBox *viewBox = nullptr;
    QSlider *zoom = nullptr;
    KUrlNavigator *nav = nullptr;
    
    // Search functionality
    class QLineEdit *searchBox = nullptr;
    QUrl m_preSearchUrl;
    QTimer *m_searchDebounce = nullptr;

    QSplitter *hsplit = nullptr;        // horizontal splitter: [stack] | [preview]
    QStackedWidget *stack = nullptr;

    QListView *iconView = nullptr;
    QTreeView *detailsView = nullptr;
    QListView *compactView = nullptr;
    MillerView *miller = nullptr;

    QWidget *preview = nullptr;
    QLabel  *previewImage = nullptr;
    QTextEdit *previewText = nullptr;

    QuickLookDialog *ql = nullptr;
    mutable ThumbCache *thumbs = nullptr;  // mutable: caching is logically const

    KDirModel *dirModel = nullptr;
    KDirSortFilterProxyModel *proxy = nullptr;
    KFilePreviewGenerator *m_previewGenerator = nullptr;

    QLabel *m_emptyFolderLabel = nullptr;

    QUrl currentRoot;
    bool m_previewVisible = false;
    bool m_showHiddenFiles = false;
    bool m_showFileExtensions = true;
    bool m_followSymlinks = false;
    
    PaneNavigationState m_navigationState;

    // Initialization state
    bool m_viewInitialized = false;

    // Inline rename editing state (guards eventFilter from intercepting editor keys)
    bool m_isEditing = false;
    QMetaObject::Connection m_closeEditorConnection;

    // Ctrl+L editable path bar toggle
    bool m_pathBarEditRequested = false;

    // Selection preservation across view mode switches
    QList<QUrl> m_savedSelectionUrls;
    QUrl m_savedCurrentUrl;

    // Finder-style slow second-click rename state for classic views.
    QPointer<QAbstractItemView> m_renameClickView;
    QPersistentModelIndex m_renameClickIndex;
    QElapsedTimer m_renameClickTimer;

};
