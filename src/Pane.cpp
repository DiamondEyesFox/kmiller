#include <QDir>
#include <QShortcut>
#include <QList>
#include "Pane.h"
#include "MillerView.h"
#include "QuickLookDialog.h"
#include "ThumbCache.h"

#include <QToolBar>
#include <QComboBox>
#include <QSlider>
#include <QStackedWidget>
#include <QListView>
#include <QTreeView>
#include <QMenu>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QFileInfo>
#include <QImageReader>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QFileIconProvider>
#include <QMimeDatabase>

#include <KUrlNavigator>
#include <KFileItem>
#include <KDirModel>
#include <KDirLister>
#include <KDirSortFilterProxyModel>
#include <KIO/OpenUrlJob>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>

#include <poppler-qt6.h>

static bool isImageFile(const QString &path) {
    const QByteArray fmt = QImageReader::imageFormat(path);
    return !fmt.isEmpty();
}

static QString getThumbnailPath(const QUrl &url) {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty()) cacheDir = QDir::homePath() + "/.cache";
    
    QDir dir(cacheDir + "/kmiller/thumbs");
    if (!dir.exists()) dir.mkpath(dir.absolutePath());
    
    QString urlStr = url.toString();
    QString hash = QString(QCryptographicHash::hash(urlStr.toUtf8(), QCryptographicHash::Md5).toHex());
    return dir.absoluteFilePath(hash + ".png");
}

Pane::Pane(const QUrl &startUrl, QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);

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
    
    tb->addSeparator();
    tb->addWidget(new QLabel(" Search "));
    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText("Filter files...");
    searchBox->setFixedWidth(150);
    tb->addWidget(searchBox);

    root->addWidget(tb);

    nav = new KUrlNavigator(this);
    root->addWidget(nav);

    // Splitter: left = stacked views, right = preview
    hsplit = new QSplitter(Qt::Horizontal, this);
    hsplit->setChildrenCollapsible(false);
    hsplit->setHandleWidth(3); // thinner handle
    root->addWidget(hsplit);

    stack = new QStackedWidget(this);
    hsplit->addWidget(stack);

    // Preview area
    preview = new QWidget(this);
    auto *pv = new QVBoxLayout(preview);
    pv->setContentsMargins(8,8,8,8);
    previewImage = new QLabel(preview);
    previewImage->setAlignment(Qt::AlignCenter);
    previewImage->setMinimumHeight(160);
    previewImage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewImage->setScaledContents(false);

    previewText = new QTextEdit(preview);
    previewText->setReadOnly(true);
    previewText->setMinimumHeight(80);

    pv->addWidget(previewImage, 1);
    pv->addWidget(previewText, 0);
    hsplit->addWidget(preview);
    preview->setVisible(false); // hidden by default
    preview->setMinimumWidth(200); // minimum preview width
    preview->setMaximumWidth(400); // reasonable max width
    
    // Set initial sizes: 70% main view, 30% preview
    hsplit->setSizes({700, 300});

    dirModel = new KDirModel(this);
    proxy = new KDirSortFilterProxyModel(this);
    proxy->setSourceModel(dirModel);
    proxy->setSortFoldersFirst(true);
    proxy->setDynamicSortFilter(true);
    
    // Connect to model changes for status updates
    connect(proxy, &QAbstractItemModel::rowsInserted, this, &Pane::updateStatus);
    connect(proxy, &QAbstractItemModel::rowsRemoved, this, &Pane::updateStatus);
    connect(proxy, &QAbstractItemModel::modelReset, this, &Pane::updateStatus);

    iconView = new QListView(this);
    iconView->setViewMode(QListView::IconMode);
    iconView->setResizeMode(QListView::Adjust);
    iconView->setModel(proxy);
    iconView->setIconSize(QSize(64,64));
    iconView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    iconView->setDragDropMode(QAbstractItemView::DragDrop);
    iconView->setDefaultDropAction(Qt::CopyAction);
    stack->addWidget(iconView);

    detailsView = new QTreeView(this);
    detailsView->setModel(proxy);
    detailsView->setRootIsDecorated(false);
    detailsView->setAlternatingRowColors(true);
    detailsView->setSortingEnabled(true);
    detailsView->header()->setStretchLastSection(true);
    detailsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailsView->setDragDropMode(QAbstractItemView::DragDrop);
    detailsView->setDefaultDropAction(Qt::CopyAction);
    stack->addWidget(detailsView);

    compactView = new QListView(this);
    compactView->setModel(proxy);
    compactView->setUniformItemSizes(true);
    compactView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    compactView->setDragDropMode(QAbstractItemView::DragDrop);
    compactView->setDefaultDropAction(Qt::CopyAction);
    stack->addWidget(compactView);

    miller = new MillerView(this);
    stack->addWidget(miller);
    // Ensure Ctrl+A selects all in classic views
    for (QAbstractItemView* v : { static_cast<QAbstractItemView*>(iconView), static_cast<QAbstractItemView*>(detailsView), static_cast<QAbstractItemView*>(compactView) }) {
        if (!v) continue;
        auto *sc = new QShortcut(QKeySequence::SelectAll, v);
        connect(sc, &QShortcut::activated, v, [v]{ if (v->selectionModel()) v->selectionModel()->select(QModelIndex(), QItemSelectionModel::Select | QItemSelectionModel::Rows); });
    }
    // Miller: multi-item context menu + Quick Look
    connect(miller, &MillerView::quickLookRequested, this, [this](const QString &p){ if (ql && ql->isVisible()) { ql->close(); } else { if (!ql) ql = new QuickLookDialog(this); ql->showFile(p); } });
    connect(miller, &MillerView::contextMenuRequested, this, [this](const QUrl &u, const QPoint &g){ showContextMenu(g, {u}); });
    connect(miller, &MillerView::selectionChanged, this, [this](const QUrl &url){ if (m_previewVisible) updatePreviewForUrl(url); });

    ql = new QuickLookDialog(this);
    thumbs = new ThumbCache(this);

    connect(viewBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &Pane::onViewModeChanged);
    connect(zoom, &QSlider::valueChanged, this, &Pane::onZoomChanged);
    connect(nav, &KUrlNavigator::urlChanged, this, &Pane::onNavigatorUrlChanged);
    
    // Search/filter functionality
    connect(searchBox, &QLineEdit::textChanged, this, [this](const QString &text){
        if (proxy) {
            proxy->setFilterWildcard(text.isEmpty() ? QString() : QString("*%1*").arg(text));
        }
    });

    // Selection -> preview (Icons/Details/Compact)
    auto hookSel = [this](QAbstractItemView *v){
        if (!v) return;
        v->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(v, &QWidget::customContextMenuRequested, this, [this, v](const QPoint &pos){
            showContextMenu(v->viewport()->mapToGlobal(pos));
        });
        connect(v->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &Pane::onCurrentChanged);
        connect(v->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &Pane::updateStatus);
        connect(v, &QAbstractItemView::activated, this, &Pane::onActivated); // double-click opens
        v->setEditTriggers(QAbstractItemView::NoEditTriggers);
    };
    hookSel(iconView);
    hookSel(detailsView);
    hookSel(compactView);

    applyIconSize(64);
    setRoot(startUrl);
    viewBox->setCurrentIndex(0); // sync UI to Icons
    setViewMode(0); // default to Icons view
}

void Pane::setRoot(const QUrl &url) {
    // Add to history if this is a new navigation (not back/forward)
    if (currentRoot != url && m_historyIndex >= 0) {
        // Remove any forward history when navigating to a new location
        while (m_history.size() > m_historyIndex + 1) {
            m_history.removeLast();
        }
        m_history.append(url);
        m_historyIndex = m_history.size() - 1;
    } else if (m_historyIndex < 0) {
        // First navigation
        m_history.clear();
        m_history.append(url);
        m_historyIndex = 0;
    }
    
    currentRoot = url;
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }
    if (miller) miller->setRootUrl(url);
    if (nav && nav->locationUrl() != url) nav->setLocationUrl(url);
}

void Pane::setUrl(const QUrl &url) { setRoot(url); }

void Pane::setViewMode(int idx) {
    if (!stack) return;
    if (idx < 0 || idx > 3) return;
    stack->setCurrentIndex(idx);

    // Ensure a sane current/selection when switching (for non-Miller)
    if (idx != 3) {
        auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
        if (v) {
            QModelIndex rootIdx = v->rootIndex();
            if (!rootIdx.isValid()) {
                // map current root into proxy:
                QModelIndex pr = proxy->mapFromSource(dirModel->indexForUrl(currentRoot));
                rootIdx = pr;
                v->setRootIndex(rootIdx);
            }
            if (!v->currentIndex().isValid() && proxy->rowCount(rootIdx) > 0) {
                QModelIndex first = proxy->index(0,0,rootIdx);
                v->setCurrentIndex(first);
                if (auto *sm = v->selectionModel())
                    sm->select(first, QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
                v->scrollTo(first);
                onCurrentChanged(first, QModelIndex());
            }
        }
    }
}

void Pane::goUp() {
    if (!currentRoot.isLocalFile()) return;
    QDir d(currentRoot.toLocalFile());
    if (!d.cdUp()) return;
    setRoot(QUrl::fromLocalFile(d.absolutePath()));
}

void Pane::goHome() { setRoot(QUrl::fromLocalFile(QDir::homePath())); }

void Pane::onViewModeChanged(int idx) { setViewMode(idx); }
void Pane::onZoomChanged(int val) { applyIconSize(val); }
void Pane::onNavigatorUrlChanged(const QUrl &url) { setRoot(url); }

void Pane::applyIconSize(int px) { if (iconView) iconView->setIconSize(QSize(px,px)); }

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
    // Update preview to match activation as well (helps when toggled on)
    if (m_previewVisible) updatePreviewForUrl(url);
}

void Pane::onCurrentChanged(const QModelIndex &current, const QModelIndex &) {
    if (!m_previewVisible) return;
    const QUrl url = urlForIndex(current);
    if (url.isValid()) updatePreviewForUrl(url);
}

void Pane::quickLookSelected() {
    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (!v) return;
    
    QUrl url = urlForIndex(v->currentIndex());
    if (!url.isValid() || !url.isLocalFile()) return;
    
    if (!ql) ql = new QuickLookDialog(this);
    ql->showFile(url.toLocalFile());
}

void Pane::setPreviewVisible(bool on) {
    m_previewVisible = on;
    if (preview) preview->setVisible(on);
    if (on) {
        // seed with current selection if any
        if (auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
            const QUrl u = urlForIndex(v->currentIndex());
            if (u.isValid()) updatePreviewForUrl(u);
            else clearPreview();
        } else {
            clearPreview();
        }
    }
}

void Pane::clearPreview() {
    if (previewImage) previewImage->setPixmap(QPixmap());
    if (previewText)  previewText->setPlainText(QString());
}

void Pane::updatePreviewForUrl(const QUrl &u) {
    clearPreview();
    if (!u.isValid() || !u.isLocalFile()) return;

    const QString path = u.toLocalFile();
    QFileInfo fi(path);

    if (fi.isDir()) {
        previewText->setPlainText(QString("%1\n%2 items")
                                  .arg(fi.fileName().isEmpty() ? path : fi.fileName())
                                  .arg(QDir(path).entryList(QDir::NoDotAndDotDot|QDir::AllEntries).size()));
        return;
    }

    if (isImageFile(path)) {
        QPixmap pm(path);
        if (!pm.isNull()) {
            // fit-to-pane while preserving aspect ratio
            const int w = previewImage->width()  > 0 ? previewImage->width()  : 600;
            const int h = previewImage->height() > 0 ? previewImage->height() : 300;
            previewImage->setPixmap(pm.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        previewText->setPlainText(QString("%1 — %2 KB")
                                  .arg(fi.fileName())
                                  .arg((fi.size()+1023)/1024));
        return;
    }

    if (fi.suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
        std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
        if (doc) {
            doc->setRenderHint(Poppler::Document::Antialiasing);
            doc->setRenderHint(Poppler::Document::TextAntialiasing);
            std::unique_ptr<Poppler::Page> page(doc->page(0));
            if (page) {
                QImage img = page->renderToImage(120.0, 120.0);
                if (!img.isNull()) {
                    previewImage->setPixmap(QPixmap::fromImage(img));
                }
            }
            previewText->setPlainText(fi.fileName());
            return;
        }
    }

    // Fallback
    previewText->setPlainText(QString("%1 — %2 KB")
                              .arg(fi.fileName())
                              .arg((fi.size()+1023)/1024));
}

// --- temporary stub to compile; real menu can be filled later ---

// ---- clean stub (temporary, just to compile) ----
void Pane::showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls)
{
    QUrl u;
    if (!urls.isEmpty()) {
        u = urls.first();
    }

    // If no explicit URL, try the current standard view selection
    if (!u.isValid()) {
        QAbstractItemView *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
        if (v) {
            QModelIndex idx = v->currentIndex();
            if (idx.isValid()) {
                u = urlForIndex(idx);
            }
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

// ---- File Operations Implementation ----

QList<QUrl> Pane::getSelectedUrls() const {
    QList<QUrl> urls;
    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (!v || !v->selectionModel()) return urls;
    
    const auto selection = v->selectionModel()->selectedIndexes();
    for (const auto &idx : selection) {
        if (idx.column() == 0) { // avoid duplicates in details view
            QUrl url = urlForIndex(idx);
            if (url.isValid()) urls.append(url);
        }
    }
    return urls;
}

void Pane::copySelected() {
    clipboardUrls = getSelectedUrls();
    isCutOperation = false;
}

void Pane::cutSelected() {
    clipboardUrls = getSelectedUrls();
    isCutOperation = true;
}

void Pane::pasteFiles() {
    if (clipboardUrls.isEmpty()) return;
    
    QUrl destDir = currentRoot;
    if (isCutOperation) {
        // Move operation
        auto *job = KIO::move(clipboardUrls, destDir);
        clipboardUrls.clear(); // Clear clipboard after move
    } else {
        // Copy operation  
        auto *job = KIO::copy(clipboardUrls, destDir);
    }
    isCutOperation = false;
}

void Pane::deleteSelected() {
    const auto urls = getSelectedUrls();
    if (urls.isEmpty()) return;
    
    auto *job = KIO::del(urls);
}

void Pane::renameSelected() {
    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (!v || !v->selectionModel()) return;
    
    const QModelIndex current = v->selectionModel()->currentIndex();
    if (current.isValid()) {
        // Enable editing for rename functionality
        v->setEditTriggers(QAbstractItemView::SelectedClicked);
        v->edit(current);
        // Restore no edit triggers after rename
        v->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
}

void Pane::createNewFolder() {
    // Create a uniquely named new folder
    QString baseName = "New Folder";
    QString folderName = baseName;
    int counter = 1;
    
    QDir currentDir(currentRoot.toLocalFile());
    while (currentDir.exists(folderName)) {
        folderName = QString("%1 %2").arg(baseName).arg(counter++);
    }
    
    QUrl newFolderUrl = currentRoot.resolved(QUrl(folderName));
    if (QDir().mkdir(newFolderUrl.toLocalFile())) {
        // Refresh the directory listing to show the new folder
        if (auto *l = dirModel->dirLister()) {
            l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
        }
    }
}

void Pane::goBack() {
    if (!canGoBack()) return;
    
    m_historyIndex--;
    QUrl url = m_history[m_historyIndex];
    
    // Temporarily set currentRoot to prevent adding to history
    QUrl oldRoot = currentRoot;
    currentRoot = url;
    
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }
    if (miller) miller->setRootUrl(url);
    if (nav && nav->locationUrl() != url) nav->setLocationUrl(url);
}

void Pane::goForward() {
    if (!canGoForward()) return;
    
    m_historyIndex++;
    QUrl url = m_history[m_historyIndex];
    
    // Temporarily set currentRoot to prevent adding to history
    QUrl oldRoot = currentRoot;
    currentRoot = url;
    
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }
    if (miller) miller->setRootUrl(url);
    if (nav && nav->locationUrl() != url) nav->setLocationUrl(url);
}

bool Pane::canGoBack() const {
    return m_historyIndex > 0;
}

bool Pane::canGoForward() const {
    return m_historyIndex >= 0 && m_historyIndex < m_history.size() - 1;
}

void Pane::generateThumbnail(const QUrl &url) {
    if (!url.isLocalFile()) return;
    
    QString path = url.toLocalFile();
    QFileInfo fi(path);
    if (!fi.exists() || fi.isDir()) return;
    
    // Check if we already have a cached thumbnail
    if (thumbs && thumbs->has(url)) return;
    
    QString thumbPath = getThumbnailPath(url);
    QFileInfo thumbInfo(thumbPath);
    
    // Check if thumbnail is newer than original file
    if (thumbInfo.exists() && thumbInfo.lastModified() >= fi.lastModified()) {
        QPixmap thumb(thumbPath);
        if (!thumb.isNull() && thumbs) {
            thumbs->put(url, thumb);
            return;
        }
    }
    
    QPixmap thumbnail;
    
    if (isImageFile(path)) {
        QPixmap original(path);
        if (!original.isNull()) {
            thumbnail = original.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    } else if (fi.suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
        std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
        if (doc) {
            doc->setRenderHint(Poppler::Document::Antialiasing);
            doc->setRenderHint(Poppler::Document::TextAntialiasing);
            std::unique_ptr<Poppler::Page> page(doc->page(0));
            if (page) {
                QImage img = page->renderToImage(96.0, 96.0);
                if (!img.isNull()) {
                    thumbnail = QPixmap::fromImage(img).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
            }
        }
    }
    
    if (!thumbnail.isNull()) {
        // Save thumbnail to cache
        thumbnail.save(thumbPath, "PNG");
        
        // Add to memory cache
        if (thumbs) thumbs->put(url, thumbnail);
    }
}

QIcon Pane::getIconForFile(const QUrl &url) const {
    if (!url.isLocalFile()) return QIcon();
    
    // Check if we have a thumbnail
    if (thumbs && thumbs->has(url)) {
        return QIcon(thumbs->get(url));
    }
    
    // Check if thumbnail exists on disk
    QString thumbPath = getThumbnailPath(url);
    if (QFileInfo(thumbPath).exists()) {
        QPixmap thumb(thumbPath);
        if (!thumb.isNull()) {
            if (thumbs) thumbs->put(url, thumb);
            return QIcon(thumb);
        }
    }
    
    // For image files, generate thumbnail asynchronously
    QString path = url.toLocalFile();
    if (isImageFile(path) || QFileInfo(path).suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
        // Generate thumbnail in the background
        const_cast<Pane*>(this)->generateThumbnail(url);
    }
    
    // Return default icon for now
    QFileIconProvider provider;
    return provider.icon(QFileInfo(path));
}

void Pane::updateStatus() {
    if (!proxy) return;
    
    int totalFiles = proxy->rowCount();
    int selectedFiles = 0;
    
    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (v && v->selectionModel()) {
        selectedFiles = v->selectionModel()->selectedRows().count();
    }
    
    emit statusChanged(totalFiles, selectedFiles);
}
