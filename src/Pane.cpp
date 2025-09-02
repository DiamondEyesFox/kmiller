#include <QDir>
#include <QShortcut>
#include <QList>
#include "Pane.h"
#include "MillerView.h"
#include "QuickLookDialog.h"
#include "ThumbCache.h"
#include "PropertiesDialog.h"

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
#include <QMimeDatabase>
#include <QDialog>
#include <QVBoxLayout>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFileDialog>
#include <QCheckBox>
#include <QSettings>
#include <QTimer>

#include <poppler-qt6.h>
#include <QKeyEvent>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTimer>
#include <QIdentityProxyModel>

static bool isImageFile(const QString &path) {
    const QByteArray fmt = QImageReader::imageFormat(path);
    return !fmt.isEmpty();
}

static bool isArchiveFile(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == "zip" || suffix == "tar" || suffix == "gz" || 
           suffix == "bz2" || suffix == "xz" || suffix == "7z" ||
           suffix == "rar" || suffix == "tgz" || suffix == "tbz2" ||
           suffix == "txz" || suffix == "lzma";
}

static QPixmap getFileTypeIcon(const QFileInfo &fi, int size = 128) {
    QMimeDatabase db;
    QString mimeType;
    
    if (fi.isDir()) {
        // Special folder types
        QString name = fi.fileName().toLower();
        if (name == "documents" || name == "doc") {
            return QIcon::fromTheme("folder-documents").pixmap(size, size);
        } else if (name == "downloads" || name == "download") {
            return QIcon::fromTheme("folder-download").pixmap(size, size);
        } else if (name == "pictures" || name == "images") {
            return QIcon::fromTheme("folder-pictures").pixmap(size, size);
        } else if (name == "music" || name == "audio") {
            return QIcon::fromTheme("folder-music").pixmap(size, size);
        } else if (name == "videos" || name == "movies") {
            return QIcon::fromTheme("folder-videos").pixmap(size, size);
        } else if (name == "desktop") {
            return QIcon::fromTheme("folder-desktop").pixmap(size, size);
        } else if (name == "home") {
            return QIcon::fromTheme("folder-home").pixmap(size, size);
        } else {
            return QIcon::fromTheme("folder").pixmap(size, size);
        }
    }
    
    // Get MIME type for files
    auto mt = db.mimeTypeForFile(fi.filePath(), QMimeDatabase::MatchContent);
    mimeType = mt.name();
    
    // Try to get icon from MIME type
    QIcon icon = QIcon::fromTheme(mt.iconName());
    if (!icon.isNull()) {
        return icon.pixmap(size, size);
    }
    
    // Fallback icons based on file extensions and MIME types
    QString suffix = fi.suffix().toLower();
    
    if (mimeType.startsWith("image/")) {
        return QIcon::fromTheme("image-x-generic").pixmap(size, size);
    } else if (mimeType.startsWith("video/")) {
        return QIcon::fromTheme("video-x-generic").pixmap(size, size);
    } else if (mimeType.startsWith("audio/")) {
        return QIcon::fromTheme("audio-x-generic").pixmap(size, size);
    } else if (mimeType == "application/pdf") {
        return QIcon::fromTheme("application-pdf").pixmap(size, size);
    } else if (mimeType.startsWith("text/") || mimeType.contains("json") || mimeType.contains("xml")) {
        return QIcon::fromTheme("text-x-generic").pixmap(size, size);
    } else if (suffix == "zip" || suffix == "rar" || suffix == "tar" || suffix == "gz" || 
               suffix == "7z" || suffix == "bz2" || suffix == "xz") {
        return QIcon::fromTheme("package-x-generic").pixmap(size, size);
    } else if (suffix == "deb" || suffix == "rpm" || suffix == "pkg" || suffix == "dmg") {
        return QIcon::fromTheme("package-x-generic").pixmap(size, size);
    } else if (suffix == "exe" || suffix == "msi") {
        return QIcon::fromTheme("application-x-ms-dos-executable").pixmap(size, size);
    } else if (suffix == "appimage" || fi.isExecutable()) {
        return QIcon::fromTheme("application-x-executable").pixmap(size, size);
    }
    
    // Final fallback
    return QIcon::fromTheme("text-x-generic").pixmap(size, size);
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

    // Add spacer to push search to the right
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);
    
    // Search on the right side
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

    // Create zoom slider (for internal use, not in toolbar anymore)
    zoom = new QSlider(Qt::Horizontal, this);
    zoom->setRange(32, 192);
    zoom->setValue(64);
    zoom->setVisible(false); // Hidden, controlled from MainWindow status bar

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
    
    // Configure spacing like Finder - uniform grid with proper padding
    iconView->setUniformItemSizes(true);
    iconView->setGridSize(QSize(100, 100)); // Finder-like grid spacing
    iconView->setSpacing(8); // Space between items
    iconView->setWordWrap(true);
    iconView->setTextElideMode(Qt::ElideMiddle);
    
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
        v->setSelectionMode(QAbstractItemView::ExtendedSelection); // Enable multiselect like Miller view
        connect(v, &QWidget::customContextMenuRequested, this, [this, v](const QPoint &pos){
            showContextMenu(v->viewport()->mapToGlobal(pos));
        });
        connect(v->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &Pane::onCurrentChanged);
        connect(v->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &Pane::updateStatus);
        connect(v, &QAbstractItemView::activated, this, &Pane::onActivated); // double-click opens
        v->setEditTriggers(QAbstractItemView::NoEditTriggers);
        
        // Install spacebar event filter
        v->installEventFilter(this);
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

int Pane::currentViewMode() const {
    return stack ? stack->currentIndex() : 0;
}
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

void Pane::setShowHiddenFiles(bool show) {
    m_showHiddenFiles = show;
    if (dirModel) {
        if (auto *lister = dirModel->dirLister()) {
            lister->setShowHiddenFiles(show);
            // Refresh the current directory to apply changes
            lister->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
        }
    }
    // Also update Miller view
    if (miller) {
        miller->setShowHiddenFiles(show);
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
        // Show folder icon
        QPixmap folderIcon = getFileTypeIcon(fi, 128);
        if (!folderIcon.isNull()) {
            previewImage->setPixmap(folderIcon);
        }
        
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

    // Fallback - show file type icon
    QPixmap fileIcon = getFileTypeIcon(fi, 128);
    if (!fileIcon.isNull()) {
        previewImage->setPixmap(fileIcon);
    }
    
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
    QAction *actOpenWith = menu.addAction("Open With...");
    QAction *actQL   = menu.addAction("Quick Look");
    menu.addSeparator();
    QAction *actCut = menu.addAction("Cut");
    QAction *actCopy = menu.addAction("Copy");
    QAction *actPaste = menu.addAction("Paste");
    menu.addSeparator();
    QAction *actDuplicate = menu.addAction("Duplicate");
    menu.addSeparator();
    QAction *actRename = menu.addAction("Rename");
    QAction *actDelete = menu.addAction("Delete");
    menu.addSeparator();
    
    // Compress/Extract based on file type
    QAction *actCompress = nullptr;
    QAction *actExtract = nullptr;
    
    QFileInfo fi(u.toLocalFile());
    if (isArchiveFile(u.toLocalFile())) {
        actExtract = menu.addAction("Extract Here");
    } else {
        actCompress = menu.addAction("Compress");
    }
    menu.addSeparator();
    
    QAction *actNewFolder = menu.addAction("New Folder");
    menu.addSeparator();
    QAction *actProperties = menu.addAction("Properties");
    
    QAction *chosen  = menu.exec(globalPos);
    if (!chosen) return;

    if (chosen == actOpen) {
        auto *job = new KIO::OpenUrlJob(u);
        job->start();
        return;
    }
    if (chosen == actOpenWith) {
        showOpenWithDialog(u);
        return;
    }
    if (chosen == actQL) {
        quickLookSelected();
        return;
    }
    if (chosen == actCut) {
        cutSelected();
        return;
    }
    if (chosen == actCopy) {
        copySelected();
        return;
    }
    if (chosen == actPaste) {
        pasteFiles();
        return;
    }
    if (chosen == actDuplicate) {
        duplicateSelected();
        return;
    }
    if (chosen == actRename) {
        renameSelected();
        return;
    }
    if (chosen == actDelete) {
        deleteSelected();
        return;
    }
    if (chosen == actCompress) {
        compressSelected();
        return;
    }
    if (chosen == actExtract) {
        extractArchive(u);
        return;
    }
    if (chosen == actNewFolder) {
        createNewFolder();
        return;
    }
    if (chosen == actProperties) {
        // Get selected URLs or fall back to current URL
        QList<QUrl> selectedUrls = getSelectedUrls();
        if (selectedUrls.isEmpty() && u.isValid()) {
            selectedUrls = {u};
        }
        
        if (!selectedUrls.isEmpty()) {
            PropertiesDialog *dialog;
            if (selectedUrls.size() == 1) {
                dialog = new PropertiesDialog(selectedUrls.first(), this);
            } else {
                dialog = new PropertiesDialog(selectedUrls, this);
            }
            dialog->exec();
            dialog->deleteLater();
        }
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
    
    QString newFolderPath = QDir(currentRoot.toLocalFile()).filePath(folderName);
    if (QDir().mkpath(newFolderPath)) {
        // Refresh the directory listing to show the new folder
        if (dirModel) {
            // Force a refresh of the KDirLister
            if (auto *lister = dirModel->dirLister()) {
                lister->updateDirectory(currentRoot);
            }
        }
        
        // Also refresh the proxy model
        if (proxy) {
            proxy->invalidate();
        }
        
        // Update the views
        updateStatus();
        
        // Try to select the new folder
        QTimer::singleShot(100, this, [this, folderName]() {
            // Try to select the newly created folder
            if (proxy) {
                for (int i = 0; i < proxy->rowCount(); ++i) {
                    QModelIndex idx = proxy->index(i, 0);
                    QString fileName = idx.data(Qt::DisplayRole).toString();
                    if (fileName == folderName) {
                        if (auto *view = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
                            view->setCurrentIndex(idx);
                            view->scrollTo(idx);
                        }
                        break;
                    }
                }
            }
        });
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

void Pane::setZoomValue(int value) {
    if (zoom) {
        zoom->setValue(value);
    }
    applyIconSize(value);
}

bool Pane::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Space) {
            // Handle spacebar for QuickLook in all view modes
            auto *view = qobject_cast<QAbstractItemView*>(obj);
            if (view) {
                QModelIndex current = view->currentIndex();
                if (current.isValid()) {
                    QUrl url = urlForIndex(current);
                    if (url.isValid()) {
                        quickLookSelected();
                        return true; // Event handled
                    }
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void Pane::showOpenWithDialog(const QUrl &url) {
    if (!url.isValid() || !url.isLocalFile()) return;
    
    QString filePath = url.toLocalFile();
    QFileInfo fi(filePath);
    
    // Get MIME type
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForFile(filePath);
    QString mimeTypeName = mimeType.name();
    
    // Check if there's already a default application for this MIME type
    QSettings settings;
    QString existingDefault = settings.value(QString("fileAssociations/%1").arg(mimeTypeName)).toString();
    
    // If there's a default app and it exists, try to use it directly
    if (!existingDefault.isEmpty()) {
        QStringList arguments;
        arguments << filePath;
        
        if (QProcess::startDetached(existingDefault, arguments)) {
            return; // Successfully opened with default app
        }
        // If it failed, continue to show dialog
    }
    
    // Create simple dialog with common applications
    QDialog dialog(this);
    dialog.setWindowTitle(QString("Open %1 with...").arg(fi.fileName()));
    dialog.setModal(true);
    dialog.resize(400, 300);
    
    auto *layout = new QVBoxLayout(&dialog);
    
    auto *label = new QLabel(QString("Choose an application to open %1:").arg(fi.fileName()));
    layout->addWidget(label);
    
    auto *listWidget = new QListWidget();
    layout->addWidget(listWidget);
    
    // Add "Always use this application" checkbox
    auto *alwaysUseCheckbox = new QCheckBox("Always open this file type with the selected application");
    layout->addWidget(alwaysUseCheckbox);
    
    // Add some common applications based on file type
    QStringList applications;
    
    if (mimeTypeName.startsWith("text/") || mimeTypeName.contains("json") || mimeTypeName.contains("xml")) {
        applications << "gedit" << "kate" << "nano" << "vim" << "code" << "atom";
    } else if (mimeTypeName.startsWith("image/")) {
        applications << "gwenview" << "gimp" << "kolourpaint" << "feh" << "eog";
    } else if (mimeTypeName.startsWith("video/")) {
        applications << "vlc" << "mpv" << "dragon" << "totem";
    } else if (mimeTypeName.startsWith("audio/")) {
        applications << "vlc" << "audacity" << "amarok" << "clementine";
    } else if (mimeTypeName == "application/pdf") {
        applications << "okular" << "evince" << "firefox" << "chrome";
    } else {
        applications << "gedit" << "kate" << "firefox" << "vlc";
    }
    
    // Add generic applications
    applications << "konsole" << "xdg-open";
    
    for (const QString &app : applications) {
        auto *item = new QListWidgetItem(app);
        item->setData(Qt::UserRole, app);
        listWidget->addItem(item);
    }
    
    // Add custom command option
    auto *customItem = new QListWidgetItem("Custom command...");
    customItem->setData(Qt::UserRole, "custom");
    listWidget->addItem(customItem);
    
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    connect(listWidget, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);
    
    if (dialog.exec() == QDialog::Accepted) {
        auto *selectedItem = listWidget->currentItem();
        if (selectedItem) {
            QString command = selectedItem->data(Qt::UserRole).toString();
            
            if (command == "custom") {
                // Show input dialog for custom command
                bool ok;
                QString customCommand = QInputDialog::getText(&dialog, "Custom Command",
                                                            "Enter command to open file with:",
                                                            QLineEdit::Normal, "", &ok);
                if (ok && !customCommand.isEmpty()) {
                    command = customCommand;
                } else {
                    return;
                }
            }
            
            // Save as default application if checkbox is checked
            if (alwaysUseCheckbox->isChecked()) {
                QSettings settings;
                settings.setValue(QString("fileAssociations/%1").arg(mimeTypeName), command);
                settings.sync();
            }
            
            // Execute the command
            QStringList arguments;
            arguments << filePath;
            
            if (!QProcess::startDetached(command, arguments)) {
                QMessageBox::warning(this, "Error", 
                                    QString("Failed to open file with %1").arg(command));
            }
        }
    }
}

void Pane::compressSelected() {
    QList<QUrl> urls = getSelectedUrls();
    if (urls.isEmpty()) return;
    
    // Get the directory for output
    QString outputDir = currentRoot.toLocalFile();
    
    // If single file/folder selected, suggest name based on it
    QString suggestedName;
    if (urls.size() == 1) {
        QFileInfo fi(urls.first().toLocalFile());
        suggestedName = fi.baseName() + ".zip";
    } else {
        suggestedName = QString("%1_files.zip").arg(urls.size());
    }
    
    bool ok;
    QString archiveName = QInputDialog::getText(this, "Compress Files",
                                               QString("Archive name (in %1):").arg(QFileInfo(outputDir).fileName()),
                                               QLineEdit::Normal, suggestedName, &ok);
    
    if (!ok || archiveName.isEmpty()) return;
    
    // Ensure .zip extension
    if (!archiveName.toLower().endsWith(".zip")) {
        archiveName += ".zip";
    }
    
    QString archivePath = outputDir + "/" + archiveName;
    
    // Check if file exists
    if (QFile::exists(archivePath)) {
        int ret = QMessageBox::question(this, "File Exists",
                                      QString("Archive '%1' already exists. Overwrite?").arg(archiveName),
                                      QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }
    
    // Build command to create zip archive
    QStringList arguments;
    arguments << "-r" << archivePath;
    
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            QString path = url.toLocalFile();
            arguments << QFileInfo(path).fileName();
        }
    }
    
    // Change to the directory containing the files
    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(outputDir);
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, archiveName, process](int exitCode) {
                if (exitCode == 0) {
                    QMessageBox::information(this, "Compression Complete",
                                           QString("Archive '%1' created successfully.").arg(archiveName));
                    // Refresh the directory listing
                    if (auto *l = dirModel->dirLister()) {
                        l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
                    }
                } else {
                    QMessageBox::warning(this, "Compression Failed",
                                       QString("Failed to create archive '%1'.").arg(archiveName));
                }
                process->deleteLater();
            });
    
    process->start("zip", arguments);
    
    if (!process->waitForStarted()) {
        QMessageBox::warning(this, "Error", "Failed to start compression. Is 'zip' installed?");
        process->deleteLater();
    }
}

void Pane::extractArchive(const QUrl &archiveUrl) {
    if (!archiveUrl.isValid() || !archiveUrl.isLocalFile()) return;
    
    QString archivePath = archiveUrl.toLocalFile();
    QFileInfo fi(archivePath);
    QString outputDir = fi.absolutePath();
    
    // Ask user for extraction location
    QString extractDir = QFileDialog::getExistingDirectory(this, "Extract to Directory",
                                                          outputDir,
                                                          QFileDialog::ShowDirsOnly);
    if (extractDir.isEmpty()) return;
    
    QString suffix = fi.suffix().toLower();
    QStringList arguments;
    QString command;
    
    // Choose appropriate extraction command based on file type
    if (suffix == "zip") {
        command = "unzip";
        arguments << "-o" << archivePath << "-d" << extractDir;
    } else if (suffix == "tar") {
        command = "tar";
        arguments << "-xf" << archivePath << "-C" << extractDir;
    } else if (suffix == "gz" || suffix == "tgz") {
        command = "tar";
        arguments << "-xzf" << archivePath << "-C" << extractDir;
    } else if (suffix == "bz2" || suffix == "tbz2") {
        command = "tar";
        arguments << "-xjf" << archivePath << "-C" << extractDir;
    } else if (suffix == "xz" || suffix == "txz") {
        command = "tar";
        arguments << "-xJf" << archivePath << "-C" << extractDir;
    } else if (suffix == "7z") {
        command = "7z";
        arguments << "x" << archivePath << "-o" + extractDir;
    } else if (suffix == "rar") {
        command = "unrar";
        arguments << "x" << archivePath << extractDir;
    } else {
        QMessageBox::warning(this, "Unsupported Format",
                           QString("Extraction of '%1' files is not supported.").arg(suffix.toUpper()));
        return;
    }
    
    QProcess *process = new QProcess(this);
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, fi, extractDir, process](int exitCode) {
                if (exitCode == 0) {
                    QMessageBox::information(this, "Extraction Complete",
                                           QString("Archive '%1' extracted successfully to '%2'.")
                                           .arg(fi.fileName()).arg(extractDir));
                } else {
                    QMessageBox::warning(this, "Extraction Failed",
                                       QString("Failed to extract archive '%1'.").arg(fi.fileName()));
                }
                process->deleteLater();
            });
    
    process->start(command, arguments);
    
    if (!process->waitForStarted()) {
        QMessageBox::warning(this, "Error", 
                           QString("Failed to start extraction. Is '%1' installed?").arg(command));
        process->deleteLater();
    }
}

void Pane::duplicateSelected() {
    QList<QUrl> urls = getSelectedUrls();
    if (urls.isEmpty()) return;
    
    QString currentDir = currentRoot.toLocalFile();
    
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) continue;
        
        QString sourcePath = url.toLocalFile();
        QFileInfo fi(sourcePath);
        
        // Generate unique name for duplicate
        QString baseName = fi.completeBaseName();
        QString extension = fi.suffix();
        QString duplicateName;
        
        if (extension.isEmpty()) {
            duplicateName = QString("%1 copy").arg(baseName);
        } else {
            duplicateName = QString("%1 copy.%2").arg(baseName).arg(extension);
        }
        
        QString duplicatePath = fi.absolutePath() + "/" + duplicateName;
        
        // If "copy" version exists, add numbers
        int counter = 1;
        while (QFile::exists(duplicatePath)) {
            counter++;
            if (extension.isEmpty()) {
                duplicateName = QString("%1 copy %2").arg(baseName).arg(counter);
            } else {
                duplicateName = QString("%1 copy %2.%3").arg(baseName).arg(counter).arg(extension);
            }
            duplicatePath = fi.absolutePath() + "/" + duplicateName;
        }
        
        // Perform the duplication
        bool success = false;
        if (fi.isDir()) {
            // For directories, use cp -r command
            QProcess *process = new QProcess(this);
            QStringList arguments;
            arguments << "-r" << sourcePath << duplicatePath;
            
            process->start("cp", arguments);
            success = process->waitForFinished(10000) && process->exitCode() == 0;
            process->deleteLater();
        } else {
            // For files, use QFile::copy
            success = QFile::copy(sourcePath, duplicatePath);
        }
        
        if (!success) {
            QMessageBox::warning(this, "Duplicate Failed",
                                QString("Failed to duplicate '%1'.").arg(fi.fileName()));
            return;
        }
    }
    
    // Refresh the directory listing to show new duplicates
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }
    
    QMessageBox::information(this, "Duplicate Complete",
                           QString("Successfully duplicated %1 item(s).").arg(urls.size()));
}

void Pane::setSortCriteria(int criteria) {
    if (!proxy) return;
    
    int column = 0;
    switch (criteria) {
        case 0: column = 0; break; // Name
        case 1: column = 1; break; // Size  
        case 2: column = 2; break; // Type
        case 3: column = 3; break; // Date Modified
        default: column = 0; break;
    }
    
    proxy->setSortRole(Qt::DisplayRole);
    proxy->sort(column, proxy->sortOrder());
}

void Pane::setSortOrder(Qt::SortOrder order) {
    if (!proxy) return;
    
    int currentColumn = proxy->sortColumn();
    if (currentColumn < 0) currentColumn = 0; // Default to name
    
    proxy->sort(currentColumn, order);
}

