#pragma once
#include <QWidget>
#include <QUrl>
#include <QPoint>

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
    void statusChanged(int totalFiles, int selectedFiles);

public:
    void showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls = QList<QUrl>());
    explicit Pane(const QUrl &startUrl, QWidget *parent = nullptr);

    void setRoot(const QUrl &url);
    void setUrl(const QUrl &url);

    // View & navigation used by MainWindow
    void setViewMode(int idx);      // 0 Icons, 1 Details, 2 Compact, 3 Miller
    void goUp();
    void goHome();
    void goBack();
    void goForward();
    bool canGoBack() const;
    bool canGoForward() const;

    // Context menu usable by both Miller and the other views

    // Quick Look convenience
    void quickLookSelected();

    // File operations
    void copySelected();
    void cutSelected();
    void pasteFiles();
    void deleteSelected();
    void renameSelected();
    void createNewFolder();

    // Preview pane
    void setPreviewVisible(bool on);
    bool previewVisible() const { return m_previewVisible; }
    
    // Status updates
    void updateStatus();

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
    
    void generateThumbnail(const QUrl &url);
    QIcon getIconForFile(const QUrl &url) const;

    void updatePreviewForUrl(const QUrl &u);
    void clearPreview();

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
    ThumbCache *thumbs = nullptr;

    KDirModel *dirModel = nullptr;
    KDirSortFilterProxyModel *proxy = nullptr;

    QUrl currentRoot;
    bool m_previewVisible = false;
    
    // Navigation history
    QList<QUrl> m_history;
    int m_historyIndex = -1;
    
    // Clipboard state
    QList<QUrl> clipboardUrls;
    bool isCutOperation = false;
};
