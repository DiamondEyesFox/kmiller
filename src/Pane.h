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

class MillerView;
class QuickLookDialog;
class ThumbCache;
class KDirModel;
class KDirSortFilterProxyModel;

class Pane : public QWidget {
    Q_OBJECT
public:
    void showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls = QList<QUrl>());
    explicit Pane(const QUrl &startUrl, QWidget *parent = nullptr);

    void setRoot(const QUrl &url);
    void setUrl(const QUrl &url);

    // View & navigation used by MainWindow
    void setViewMode(int idx);      // 0 Icons, 1 Details, 2 Compact, 3 Miller
    void goUp();
    void goHome();

    // Context menu usable by both Miller and the other views

    // Quick Look convenience
    void quickLookSelected();

    // Preview pane
    void setPreviewVisible(bool on);
    bool previewVisible() const { return m_previewVisible; }

private slots:
    void onViewModeChanged(int idx);
    void onZoomChanged(int val);

    void onActivated(const QModelIndex &idx);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void applyIconSize(int px);
    QUrl urlForIndex(const QModelIndex &proxyIndex) const;

    void updatePreviewForUrl(const QUrl &u);
    void clearPreview();

    QToolBar *tb = nullptr;
    QComboBox *viewBox = nullptr;
    QSlider *zoom = nullptr;

    QSplitter *vsplit = nullptr;        // vertical splitter: [stack] over [preview]
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
};
