#pragma once
#include <QWidget>
#include <QToolBar>
#include <QComboBox>
#include <QSlider>
#include <QStackedWidget>
#include <QListView>
#include <QTreeView>
#include <QMenu>
#include <QKeyEvent>
#include <KUrlNavigator>
#include <KDirModel>
#include <KDirSortFilterProxyModel>
#include <KFileItemActions>
#include <KIO/FileUndoManager>
#include "MillerView.h"
#include "QuickLookDialog.h"
#include "ThumbCache.h"

class Pane : public QWidget {
    Q_OBJECT
public:
    explicit Pane(const QUrl &startUrl, QWidget *parent=nullptr);
    void setUrl(const QUrl &url);
    void setViewMode(int idx);
    void goUp();
    void goHome();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onViewModeChanged(int idx);
    void onZoomChanged(int val);
    void onNavigatorUrlChanged(const QUrl &url);
    void onActivated(const QModelIndex &idx);
    void showContextMenu(const QPoint &pos);

private:
    enum ViewMode { Icons=0, Details=1, Compact=2, Miller=3 };

    QToolBar *tb;
    QComboBox *viewBox;
    QSlider *zoom;

    KUrlNavigator *nav;

    QStackedWidget *stack;
    QListView *iconView;
    QTreeView *detailsView;
    QListView *compactView;
    MillerView *miller;

    KDirModel *dirModel;
    KDirSortFilterProxyModel *proxy;

    QuickLookDialog *ql;
    ThumbCache *thumbs;

    void setRoot(const QUrl &url);
    void applyIconSize(int px);
    void setupContextMenu(QAbstractItemView *v);
    QUrl urlForIndex(const QModelIndex &proxyIndex) const;
    QUrl parentUrl(const QUrl &u) const;
};
