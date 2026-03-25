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
class QHeaderView;
class KUrlNavigator;
class MillerView;
class QuickLookDialog;
class ThumbCache;
class KDirModel;
class KDirSortFilterProxyModel;

class Pane : public QWidget {
    Q_OBJECT
public:
    explicit Pane(const QUrl &startUrl, QWidget *parent = nullptr);

    void setRoot(const QUrl &url);
    void setUrl(const QUrl &url) { setRoot(url); }

    // Menu hook usable by all views (and Miller via explicit URL)
    void showContextMenu(const QPoint &globalPos, const QUrl &specificUrl = QUrl());

    // Quick Look helper used by the main window
    void quickLookSelected();

    // View helpers (called by menu/toolbar)
    void setViewMode(int idx);      // 0 Icons, 1 Details, 2 Compact, 3 Miller
    void goUp();
    void goHome();

private slots:
    void onViewModeChanged(int idx);
    void onZoomChanged(int val);
    void onNavigatorUrlChanged(const QUrl &url);
    void onActivated(const QModelIndex &idx);

private:
    void applyIconSize(int px);
    QUrl urlForIndex(const QModelIndex &proxyIndex) const;

    QToolBar *tb = nullptr;
    QComboBox *viewBox = nullptr;
    QSlider *zoom = nullptr;
    KUrlNavigator *nav = nullptr;
    QStackedWidget *stack = nullptr;

    QListView *iconView = nullptr;
    QTreeView *detailsView = nullptr;
    QListView *compactView = nullptr;
    MillerView *miller = nullptr;

    QuickLookDialog *ql = nullptr;
    ThumbCache *thumbs = nullptr;

    KDirModel *dirModel = nullptr;
    KDirSortFilterProxyModel *proxy = nullptr;

    QUrl currentRoot;               // current location
};