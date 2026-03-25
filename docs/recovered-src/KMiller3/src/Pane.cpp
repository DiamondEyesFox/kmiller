#include "Pane.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMimeData>
#include <QGuiApplication>
#include <KIO/Job>
#include <KIO/OpenUrlJob>
#include <KIO/PreviewJob>
#include <KFileItem>
#include <KIO/JobUiDelegate>

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

    setupContextMenu(iconView);
    setupContextMenu(detailsView);
    setupContextMenu(compactView);

    connect(iconView, &QListView::activated, this, &Pane::onActivated);
    connect(detailsView, &QTreeView::activated, this, &Pane::onActivated);
    connect(compactView, &QListView::activated, this, &Pane::onActivated);

    applyIconSize(64);
    setRoot(startUrl);
}

void Pane::setRoot(const QUrl &url) {
    nav->setUrl(url);
    dirModel->dirLister()->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    miller->setRootUrl(url);
}

void Pane::setUrl(const QUrl &url) {
    setRoot(url);
}

void Pane::onViewModeChanged(int idx) {
    stack->setCurrentIndex(idx);
}

void Pane::onZoomChanged(int val) {
    applyIconSize(val);
}

void Pane::applyIconSize(int px) {
    iconView->setIconSize(QSize(px,px));
}

void Pane::onNavigatorUrlChanged(const QUrl &url) {
    setRoot(url);
}

QUrl Pane::urlForIndex(const QModelIndex &proxyIndex) const {
    QModelIndex src = proxy->mapToSource(proxyIndex);
    KFileItem item = dirModel->itemForIndex(src);
    return item.url();
}

void Pane::onActivated(const QModelIndex &idx) {
    QUrl url = urlForIndex(idx);
    if (url.isEmpty()) return;
    if (url.scheme() == "trash" || url.isLocalFile() || url.scheme()=="smb" || url.scheme()=="sftp") {
        // navigate into directories, open files via OpenUrlJob
    }
    // Determine if dir or file
    QModelIndex src = proxy->mapToSource(idx);
    KFileItem item = dirModel->itemForIndex(src);
    if (item.isDir()) {
        setRoot(url);
    } else {
        auto *job = new KIO::OpenUrlJob(url);
        job->start();
    }
}

void Pane::setupContextMenu(QAbstractItemView *v) {
    v->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(v, &QWidget::customContextMenuRequested, this, [this, v](const QPoint &pos){
        showContextMenu(v->viewport()->mapToGlobal(pos));
    });
}

void Pane::showContextMenu(const QPoint &globalPos) {
    // Determine selection from the current view
    QAbstractItemView *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (!v) return;
    QModelIndex idx = v->currentIndex();
    QList<KFileItem> items;
    if (idx.isValid()) {
        QModelIndex src = proxy->mapToSource(idx);
        items << dirModel->itemForIndex(src);
    }
    KFileItemActions actions;
    QMenu menu;
    actions.addOpenWithActions(&menu, items, QString());
    // Trash / Delete
    QAction *actTrash = menu.addAction("Move to Trash");
    QAction *actDelete = menu.addAction("Delete");
    QAction *actRename = menu.addAction("Rename");
    QAction *actProperties = menu.addAction("Properties");

    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;

    if (chosen == actRename) {
        v->edit(idx);
        return;
    }
    if (items.isEmpty()) return;

    QList<QUrl> urls; urls << items.first().url();

    if (chosen == actTrash) {
        auto *job = KIO::trash(urls);
        KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Trash, urls, QUrl("trash:/"), job);
    } else if (chosen == actDelete) {
        auto *job = KIO::del(urls);
        KIO::FileUndoManager::self()->recordJob(KIO::FileUndoManager::Rm, urls, QUrl(), job);
    } else if (chosen == actProperties) {
        // Launch properties dialog via KIO::OpenUrlJob to kpropertiesdialog if available
        auto *job = new KIO::OpenUrlJob(items.first().url());
        job->setRunExecutables(false);
        job->start();
    }
}
