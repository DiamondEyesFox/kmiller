# --- 0) Paths ---
PROJ=~/Downloads/kmiller

# --- 1) Backup current Pane files
mkdir -p "$PROJ/.backup-$(date +%s)"
cp -v "$PROJ/src/Pane.h" "$PROJ/src/Pane.cpp" "$PROJ/.backup-$(date +%s)/" 2>/dev/null || true

# --- 2) Write a clean, known-good Pane.h ---
cat > "$PROJ/src/Pane.h" <<'EOF'
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
EOF

# --- 3) Write a clean, known-good Pane.cpp ---
cat > "$PROJ/src/Pane.cpp" <<'EOF'
#include "Pane.h"
#include "MillerView.h"
#include "QuickLookDialog.h"
#include "ThumbCache.h"

#include <QVBoxLayout>
#include <QToolBar>
#include <QComboBox>
#include <QSlider>
#include <QStackedWidget>
#include <QListView>
#include <QTreeView>
#include <QMenu>
#include <QLabel>
#include <QDir>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QAbstractItemView>

#include <KUrlNavigator>
#include <KFileItem>
#include <KDirModel>
#include <KDirLister>
#include <KDirSortFilterProxyModel>
#include <KIO/OpenUrlJob>

Pane::Pane(const QUrl &startUrl, QWidget *parent) : QWidget(parent) {
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0,0,0,0);

    // --- Compact toolbar with view + zoom (unchanged) ---
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

    // --- Clickable path bar (KUrlNavigator): restores the path UX ---
    nav = new KUrlNavigator(this);
    v->addWidget(nav);

    // --- Views stack ---
    stack = new QStackedWidget(this);
    v->addWidget(stack);

    // KIO model pipeline (used by icons/details/compact)
    dirModel = new KDirModel(this);
    proxy = new KDirSortFilterProxyModel(this);
    proxy->setSourceModel(dirModel);
    proxy->setSortFoldersFirst(true);
    proxy->setDynamicSortFilter(true);

    // Icons
    iconView = new QListView(this);
    iconView->setViewMode(QListView::IconMode);
    iconView->setResizeMode(QListView::Adjust);
    iconView->setModel(proxy);
    iconView->setIconSize(QSize(64,64));
    iconView->setEditTriggers(QAbstractItemView::NoEditTriggers);           // prevent rename on dblclick
    iconView->setSelectionBehavior(QAbstractItemView::SelectRows);
    stack->addWidget(iconView);

    // Details
    detailsView = new QTreeView(this);
    detailsView->setModel(proxy);
    detailsView->setRootIsDecorated(false);
    detailsView->setAlternatingRowColors(true);
    detailsView->setSortingEnabled(true);
    detailsView->header()->setStretchLastSection(true);
    detailsView->setEditTriggers(QAbstractItemView::NoEditTriggers);        // prevent rename on dblclick
    detailsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    stack->addWidget(detailsView);

    // Compact
    compactView = new QListView(this);
    compactView->setModel(proxy);
    compactView->setUniformItemSizes(true);
    compactView->setEditTriggers(QAbstractItemView::NoEditTriggers);        // prevent rename on dblclick
    compactView->setSelectionBehavior(QAbstractItemView::SelectRows);
    stack->addWidget(compactView);

    // Miller columns
    miller = new MillerView(this);
    stack->addWidget(miller);

    ql = new QuickLookDialog(this);
    thumbs = new ThumbCache(this);

    // Signals
    connect(viewBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &Pane::onViewModeChanged);
    connect(zoom, &QSlider::valueChanged, this, &Pane::onZoomChanged);
    connect(nav,  &KUrlNavigator::urlChanged, this, &Pane::onNavigatorUrlChanged);

    // Standard views: double-click opens; Return opens
    auto wireOpen = [this](QAbstractItemView *v){
        connect(v, &QAbstractItemView::activated, this, &Pane::onActivated);
        connect(v, &QAbstractItemView::doubleClicked, this, &Pane::onActivated);
        // Right-click menu
        v->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(v, &QWidget::customContextMenuRequested, this, [this, v](const QPoint &pos){
            showContextMenu(v->viewport()->mapToGlobal(pos));
        });
    };
    wireOpen(iconView);
    wireOpen(detailsView);
    wireOpen(compactView);

    applyIconSize(64);
    setRoot(startUrl);

    // Start in Miller view per your preference
    setViewMode(3);
    viewBox->setCurrentIndex(3);
}

void Pane::setRoot(const QUrl &url) {
    currentRoot = url;
    if (nav) nav->setUrl(url); // keep path bar synced
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }
    if (miller) miller->setRootUrl(url);
}

void Pane::onNavigatorUrlChanged(const QUrl &url) {
    setRoot(url);
}

void Pane::applyIconSize(int px) {
    if (iconView) iconView->setIconSize(QSize(px,px));
}

QUrl Pane::urlForIndex(const QModelIndex &proxyIndex) const {
    QModelIndex src = proxy->mapToSource(proxyIndex);
    KFileItem item = dirModel->itemForIndex(src);
    return item.url();
}

void Pane::onActivated(const QModelIndex &idx) {
    QUrl url = urlForIndex(idx);
    if (!url.isValid()) return;
    KFileItem item = dirModel->itemForIndex(proxy->mapToSource(idx));
    if (item.isDir()) {
        setRoot(url);
    } else {
        auto *job = new KIO::OpenUrlJob(url);
        job->start();
    }
}

void Pane::showContextMenu(const QPoint &globalPos, const QUrl &specificUrl) {
    QUrl u = specificUrl;

    // If no explicit URL (Miller), try current standard view selection
    if (!u.isValid()) {
        if (auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
            QModelIndex idx = v->currentIndex();
            if (idx.isValid()) u = urlForIndex(idx);
        }
    }
    if (!u.isValid()) return;

    QMenu menu;
    QAction *actOpen = menu.addAction("Open");
    QAction *actQL   = menu.addAction("Quick Look");
    QAction *chosen  = menu.exec(globalPos);
    if (!chosen) return;

    if (chosen == actOpen) {
        auto *job = new KIO::OpenUrlJob(u);
        job->start();
        return;
    }
    if (chosen == actQL) {
        if (!ql) ql = new QuickLookDialog(this);
        if (u.isLocalFile()) ql->showFile(u.toLocalFile());
        return;
    }
}

void Pane::quickLookSelected() {
    if (!ql) ql = new QuickLookDialog(this);
    if (auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
        QModelIndex idx = v->currentIndex();
        if (!idx.isValid()) return;
        QUrl u = urlForIndex(idx);
        if (u.isLocalFile()) ql->showFile(u.toLocalFile());
    }
}

void Pane::setViewMode(int idx) {
    if (!stack) return;
    if (idx < 0 || idx > 3) return;
    stack->setCurrentIndex(idx);

    if (idx == 3) {
        // Miller manages its own selection/focus
        miller->setFocus(Qt::OtherFocusReason);
        return;
    }

    // Standard views: select first row and focus
    if (auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
        QModelIndex parent = proxy->mapFromSource(dirModel->indexForUrl(currentRoot));
        if (!v->currentIndex().isValid() && proxy->rowCount(parent) > 0) {
            QModelIndex first = proxy->index(0,0,parent);
            v->setCurrentIndex(first);
            if (auto *sm = v->selectionModel())
                sm->select(first, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            v->scrollTo(first);
        }
        v->setFocus(Qt::OtherFocusReason);
    }
}

void Pane::onViewModeChanged(int idx) { setViewMode(idx); }
void Pane::onZoomChanged(int val)     { applyIconSize(val); }

void Pane::goUp() {
    if (!currentRoot.isLocalFile()) return;
    QDir d(currentRoot.toLocalFile());
    if (!d.cdUp()) return;
    setRoot(QUrl::fromLocalFile(d.absolutePath()));
}

void Pane::goHome() {
    setRoot(QUrl::fromLocalFile(QDir::homePath()));
}
EOF

# --- 4) Build & install as 0.4.2 (so it won’t collide with olds) ---
cd "$PROJ"
rm -rf build && mkdir build && cd build
cmake -DKMILLER_VERSION=0.4.2 ..
make -j"$(nproc)"
sudo make install

# --- 5) Point CLI + menu to this exact binary ---
sudo ln -sfn /opt/kmiller/versions/0.4.2/bin/kmiller /usr/local/bin/kmiller
sudo sed -i 's|^Exec=.*|Exec=/opt/kmiller/versions/0.4.2/bin/kmiller %U|' /usr/local/share/applications/kmiller.desktop
kbuildsycoca6 --noincremental 2>/dev/null || true
update-desktop-database /usr/local/share/applications 2>/dev/null || true

# --- 6) Show what will run ---
echo "CLI -> $(readlink -f /usr/local/bin/kmiller)"
grep -m1 '^Exec=' /usr/local/share/applications/kmiller.desktop
