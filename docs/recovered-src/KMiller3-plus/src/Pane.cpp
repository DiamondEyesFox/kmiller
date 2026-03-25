#include "Pane.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <KDirLister>
#include <KFileItem>
#include <KIO/OpenUrlJob>
#include <KIO/Global>

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

    auto setup = [this](QAbstractItemView *vv){
        vv->setContextMenuPolicy(Qt::CustomContextMenu);
        vv->installEventFilter(this); // capture arrow keys
        connect(vv, &QWidget::customContextMenuRequested, this, [this, vv](const QPoint &pos){
            showContextMenu(vv->viewport()->mapToGlobal(pos));
        });
        connect(vv, &QAbstractItemView::activated, this, &Pane::onActivated);
    };
    setup(iconView); setup(detailsView); setup(compactView);

    applyIconSize(64);
    setRoot(startUrl);
}

void Pane::setRoot(const QUrl &url) {
    nav->setLocationUrl(url);
    dirModel->dirLister()->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    miller->setRootUrl(url);
}

void Pane::setUrl(const QUrl &url) { setRoot(url); }

void Pane::setViewMode(int idx) {
    if (idx >=0 && idx <=3) {
        viewBox->setCurrentIndex(idx);
        stack->setCurrentIndex(idx);
    }
}

void Pane::goUp() {
    setRoot(parentUrl(nav->locationUrl()));
}
void Pane::goHome() {
    setRoot(QUrl::fromLocalFile(QDir::homePath()));
}

void Pane::onViewModeChanged(int idx) { stack->setCurrentIndex(idx); }
void Pane::onZoomChanged(int val) { applyIconSize(val); }
void Pane::applyIconSize(int px) { iconView->setIconSize(QSize(px,px)); }
void Pane::onNavigatorUrlChanged(const QUrl &url) { setRoot(url); }

QUrl Pane::urlForIndex(const QModelIndex &proxyIndex) const {
    QModelIndex src = proxy->mapToSource(proxyIndex);
    KFileItem item = dirModel->itemForIndex(src);
    return item.url();
}

QUrl Pane::parentUrl(const QUrl &u) const {
    // Prefer KIO helper for schemes it understands
    QUrl up = KIO::upUrl(u);
    if (!up.isEmpty())
        return up;
    // Fallback for local
    if (u.isLocalFile()) {
        QDir d(u.toLocalFile());
        d.cdUp();
        return QUrl::fromLocalFile(d.absolutePath());
    }
    // Generic fallback
    QString path = u.path();
    int slash = path.lastIndexOf('/');
    if (slash > 0) path = path.left(slash);
    QUrl out = u;
    out.setPath(path);
    return out;
}

bool Pane::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Right) {
            QAbstractItemView *v = qobject_cast<QAbstractItemView*>(obj);
            if (!v) return false;
            QModelIndex idx = v->currentIndex();
            if (idx.isValid()) {
                QUrl u = urlForIndex(idx);
                // enter directory on Right
                if (!u.isEmpty()) {
                    // best effort: if dir, navigate in
                    QModelIndex src = proxy->mapToSource(idx);
                    KFileItem item = dirModel->itemForIndex(src);
                    if (item.isDir()) { setRoot(u); return true; }
                }
            }
        } else if (ke->key() == Qt::Key_Left) {
            // go up on Left
            goUp();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void Pane::onActivated(const QModelIndex &idx) {
    QUrl url = urlForIndex(idx);
    if (url.isEmpty()) return;
    QModelIndex src = proxy->mapToSource(idx);
    KFileItem item = dirModel->itemForIndex(src);
    if (item.isDir()) {
        setRoot(url);
    } else {
        auto *job = new KIO::OpenUrlJob(url);
        job->start();
    }
}

void Pane::showContextMenu(const QPoint &globalPos) {
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
    actions.insertOpenWithActionsTo(nullptr, &menu, {}); // KF6 signature

    QAction *actRename = menu.addAction("Rename");
    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;
    if (chosen == actRename) { v->edit(idx); return; }
}
