#pragma once
#include <QWidget>
#include <QUrl>
#include <QPoint>
#include <QAbstractProxyModel>

class QToolBar;
class QComboBox;
class QSlider;
class QStackedWidget;
class QListView;
class QTreeView;
class QSplitter;
class QLabel;
class QTextEdit;

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
    void moveToTrash();
    void renameSelected();
    void duplicateSelected();
    void createNewFolder();

    // Preview pane
    void setPreviewVisible(bool on);
    bool previewVisible() const { return m_previewVisible; }
    
    // Hidden files
    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const { return m_showHiddenFiles; }
    
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
    void pasteFilesToDestination(const QUrl &destination);
    bool isClipboardCutOperation() const;

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
    MillerView *miller = nullptr;

    QWidget *preview = nullptr;
    QLabel  *previewImage = nullptr;
    QTextEdit *previewText = nullptr;

    QuickLookDialog *ql = nullptr;
    mutable ThumbCache *thumbs = nullptr;  // mutable: caching is logically const

    KDirModel *dirModel = nullptr;
    KDirSortFilterProxyModel *proxy = nullptr;

    QUrl currentRoot;
    bool m_previewVisible = false;
    bool m_showHiddenFiles = false;
    
    // Navigation history
    QList<QUrl> m_history;
    int m_historyIndex = -1;

    // Initialization state
    bool m_viewInitialized = false;
};
