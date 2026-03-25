#include "Pane.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <KDirLister>
#include <KFileItem>
#include <KIO/OpenUrlJob>
#include <KIO/Global>

// constructor + setup
Pane::Pane(const QUrl &startUrl, QWidget *parent) : QWidget(parent) {
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0,0,0,0);

    tb = new QToolBar(this);
    viewBox = new QComboBox(this);
    viewBox->addItems({"Icons","Details","Compact","Miller"});
    tb->addWidget(viewBox);
    tb->addSeparator();
    tb->addWidget(new QLabel(" Zoom "));
    zoom = new QSlider(Qt::Horizontal, this);
    zoom->setRange(32, 192);
    zoom->setValue(64);
    zoom->setFixedWidth(200);
    tb->addWidget(zoom);
    v->addWidget(tb);

    nav = new KUrlNavigator(this);
    v->addWidget(nav);

    stack = new QStackedWidget(this);
    v->addWidget(stack);

    dirModel = new KDirModel(this);
    proxy = new KDirSortFilterProxyModel(this);
    proxy->setSourceModel(dirModel);
    proxy->setSortFoldersFirst(true);
    proxy->setDynamicSortFilter(true);

    iconView = new QListView(this);
    iconView->setViewMode(QListView::IconMode);
    iconView->setResizeMode(QListView::Adjust);
    iconView->setModel(proxy);
    iconView->setIconSize(QSize(64,64));
    stack->addWidget(iconView);

    detailsView = new QTreeView(this);
    detailsView->setModel(proxy);
    detailsView->setRootIsDecorated(false);
    detailsView->setAlternatingRowColors(true);
    detailsView->setSortingEnabled(true);
    detailsView->header()->setStretchLastSection(true);
    stack->addWidget(detailsView);

    compactView = new QListView(this);
    compactView->setModel(proxy);
    compactView->setUniformItemSizes(true);
    stack->addWidget(compactView);

    miller = new MillerView(this);
    stack->addWidget(miller);

    ql = new QuickLookDialog(this);
    thumbs = new ThumbCache(this);

    connect(viewBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Pane::onViewModeChanged);
    connect(zoom, &QSlider::valueChanged, this, &Pane::onZoomChanged);
    connect(nav, &KUrlNavigator::urlChanged, this, &Pane::onNavigatorUrlChanged);

    // context menus for standard views
    auto setup = [this](QAbstractItemView *vv){
        vv->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(vv, &QWidget::customContextMenuRequested, this, [this, vv](const QPoint &pos){
            showContextMenu(vv->viewport()->mapToGlobal(pos));
        });
        connect(vv, &QAbstractItemView::activated, this, &Pane::onActivated);
    };
    setup(iconView); setup(detailsView); setup(compactView);

    // only Miller view gets arrow keys + Space
    miller->installEventFilter(this);
    connect(miller, &MillerView::contextMenuRequested, this, &Pane::showContextMenu);

    applyIconSize(64);
    setRoot(startUrl);
}
