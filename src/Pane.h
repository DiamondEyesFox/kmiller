#pragma once
#include <QWidget>
#include <QUrl>
#include <QPoint>
#include <QAbstractProxyModel>
#include <QElapsedTimer>
#include <QPersistentModelIndex>
#include <QPointer>

#include "PaneNavigationState.h"

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
class QStandardItemModel;

class KUrlNavigator;
class MillerView;
class QuickLookDialog;
class ThumbCache;
class KDirModel;
class KDirSortFilterProxyModel;

class Pane : public QWidget {
    Q_OBJECT

signals:
    void statusChanged(int totalFiles, int selectedFiles, qint64 selectedSize);
    void urlChanged(const QUrl &url);

public:
    void showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls = QList<QUrl>());
    explicit Pane(const QUrl &startUrl, QWidget *parent = nullptr);

    void setRoot(const QUrl &url);
    void setUrl(const QUrl &url);
    QUrl currentUrl() const { return currentRoot; }

    // View & navigation used by MainWindow
    void setViewMode(int idx);      // 0 Icons, 1 Details, 2 Compact, 3 Miller
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
    void refreshCurrentLocation();

    // Preview pane
    void setPreviewVisible(bool on);
    bool previewVisible() const { return m_previewVisible; }
    
    // Hidden files
    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const { return m_showHiddenFiles; }

    // View settings
    void setShowThumbnails(bool show);
    void setShowFileExtensions(bool show);
    void setMillerColumnWidth(int width);
    void setFollowSymlinks(bool follow);
    
    // Status updates
    void updateStatus();
    
    // Zoom control
    void setZoomValue(int value);

    // Focus the active view
    void focusView();

private slots:
    void onViewModeChanged(int idx);
    void onZoomChanged(int val);
    void onNavigatorUrlChanged(const QUrl &url);

    void onActivated(const QModelIndex &idx);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void applyLocation(const QUrl &url, bool emitChangeSignal = true);
    void beginSearch(const QString &query);
    void clearSearchMode();
    void syncNavigatorLocation(const QUrl &url);
    void applyIconSize(int px);
    QUrl urlForIndex(const QModelIndex &proxyIndex) const;
    QModelIndex currentSelectionIndexForView(QAbstractItemView *view) const;
    QModelIndex currentSelectionIndex() const;
    QUrl primarySelectionUrl() const;
    QList<QUrl> getSelectedUrls() const;
    void selectUrlInActiveView(const QUrl &targetUrl);
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
    void beginInlineRename(QAbstractItemView *view, const QModelIndex &index);
    void pasteFilesToDestination(const QUrl &destination);
    bool isClipboardCutOperation() const;
    void openTerminalAt(const QUrl &targetFolder);
    bool tryNavigateToDirectoryUrl(const QUrl &url, bool showBlockedMessage);

    QToolBar *tb = nullptr;
    QComboBox *viewBox = nullptr;
    QSlider *zoom = nullptr;
    KUrlNavigator *nav = nullptr;
    
    // Search functionality
    class QLineEdit *searchBox = nullptr;

    QSplitter *hsplit = nullptr;        // horizontal splitter: [stack] | [preview]
    QStackedWidget *stack = nullptr;

    QListView *iconView = nullptr;
    QTreeView *detailsView = nullptr;
    QListView *compactView = nullptr;
    QTreeView *searchResultsView = nullptr;
    MillerView *miller = nullptr;

    QWidget *preview = nullptr;
    QLabel  *previewImage = nullptr;
    QTextEdit *previewText = nullptr;

    QuickLookDialog *ql = nullptr;
    mutable ThumbCache *thumbs = nullptr;  // mutable: caching is logically const

    KDirModel *dirModel = nullptr;
    KDirSortFilterProxyModel *proxy = nullptr;
    QStandardItemModel *searchResultsModel = nullptr;

    QUrl currentRoot;
    bool m_previewVisible = false;
    bool m_showHiddenFiles = false;
    bool m_showThumbnails = true;
    bool m_showFileExtensions = true;
    int m_millerColumnWidth = 200;
    bool m_followSymlinks = false;
    
    PaneNavigationState m_navigationState;

    // Initialization state
    bool m_viewInitialized = false;
    bool m_searchModeActive = false;
    int m_savedViewModeBeforeSearch = 3;
    quint64 m_searchRequestId = 0;

    // Finder-style slow second-click rename state for classic views.
    QPointer<QAbstractItemView> m_renameClickView;
    QPersistentModelIndex m_renameClickIndex;
    QElapsedTimer m_renameClickTimer;

};
