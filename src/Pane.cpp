// Local headers
#include "Pane.h"
#include "MillerView.h"
#include "QuickLookDialog.h"
#include "ThumbCache.h"
#include "PropertiesDialog.h"
#include "FileOpsService.h"

// Qt Core
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QMimeData>
#include <QMimeDatabase>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QStandardPaths>

// Qt GUI
#include <QClipboard>
#include <QGuiApplication>
#include <QImageReader>
#include <QKeyEvent>
#include <QPainter>
#include <QFileIconProvider>

// Qt Widgets
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QFrame>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

// KDE Frameworks
#include <KApplicationTrader>
#include <KDirLister>
#include <KDirModel>
#include <KDirSortFilterProxyModel>
#include <KFileItem>
#include <KIO/ApplicationLauncherJob>
#include <KService>
#include <KUrlNavigator>

// External libraries
#include <poppler-qt6.h>

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

static bool confirmDeleteAction(QWidget *parent, const QList<QUrl> &urls, bool permanent) {
    if (urls.isEmpty()) return false;

    QString message;
    if (urls.size() == 1) {
        QFileInfo fi(urls.first().toLocalFile());
        const QString name = fi.fileName().isEmpty() ? urls.first().toString() : fi.fileName();
        if (permanent) {
            message = QString("Are you sure you want to PERMANENTLY delete \"%1\"?\n\nThis cannot be undone.").arg(name);
        } else {
            message = QString("Are you sure you want to move \"%1\" to Trash?").arg(name);
        }
    } else {
        if (permanent) {
            message = QString("Are you sure you want to PERMANENTLY delete %1 items?\n\nThis cannot be undone.").arg(urls.size());
        } else {
            message = QString("Are you sure you want to move %1 items to Trash?").arg(urls.size());
        }
    }

    const QString title = permanent ? "Confirm Permanent Delete" : "Confirm Move to Trash";
    QMessageBox::StandardButton reply = QMessageBox::question(
        parent, title, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return reply == QMessageBox::Yes;
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
    previewText->setFrameStyle(QFrame::NoFrame);
    previewText->setStyleSheet("QTextEdit { background: transparent; }");

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
    iconView->setDragEnabled(true);
    iconView->setAcceptDrops(true);
    iconView->setDropIndicatorShown(true);
    iconView->setDragDropMode(QAbstractItemView::DragDrop);
    iconView->setDefaultDropAction(Qt::MoveAction);  // Finder-like behavior
    
    // Configure spacing like Finder - uniform grid with proper padding
    iconView->setUniformItemSizes(true);
    iconView->setGridSize(QSize(100, 100)); // Finder-like grid spacing
    iconView->setSpacing(8); // Space between items
    iconView->setWordWrap(true);
    iconView->setTextElideMode(Qt::ElideMiddle);
    iconView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    stack->addWidget(iconView);

    detailsView = new QTreeView(this);
    detailsView->setModel(proxy);
    detailsView->setRootIsDecorated(false);
    detailsView->setAlternatingRowColors(true);
    detailsView->setSortingEnabled(true);
    detailsView->header()->setStretchLastSection(true);
    detailsView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(detailsView->header(), &QHeaderView::customContextMenuRequested, this, &Pane::showHeaderContextMenu);
    detailsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailsView->setDragEnabled(true);
    detailsView->setAcceptDrops(true);
    detailsView->setDropIndicatorShown(true);
    detailsView->setDragDropMode(QAbstractItemView::DragDrop);
    detailsView->setDefaultDropAction(Qt::MoveAction);  // Finder-like behavior
    detailsView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Load column visibility and width settings
    QSettings settings;
    for (int i = 0; i < 7; ++i) {  // 7 columns: Name, Size, Date Modified, Permissions, Owner, Group, Type
        bool visible = settings.value(QString("view/column%1Visible").arg(i), true).toBool();
        detailsView->header()->setSectionHidden(i, !visible);
        
        // Load column width (use default widths if not saved)
        int width = settings.value(QString("view/column%1Width").arg(i), -1).toInt();
        if (width > 0) {
            detailsView->header()->resizeSection(i, width);
        }
    }
    
    // Connect to save column widths when they change
    connect(detailsView->header(), &QHeaderView::sectionResized, this, [this](int logicalIndex, int, int newSize) {
        QSettings settings;
        settings.setValue(QString("view/column%1Width").arg(logicalIndex), newSize);
    });
    
    stack->addWidget(detailsView);

    compactView = new QListView(this);
    compactView->setModel(proxy);
    compactView->setUniformItemSizes(true);
    compactView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    compactView->setDragEnabled(true);
    compactView->setAcceptDrops(true);
    compactView->setDropIndicatorShown(true);
    compactView->setDragDropMode(QAbstractItemView::DragDrop);
    compactView->setDefaultDropAction(Qt::MoveAction);  // Finder-like behavior
    compactView->setContextMenuPolicy(Qt::CustomContextMenu);
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
    connect(miller, &MillerView::contextMenuRequested, this, [this](const QList<QUrl> &urls, const QPoint &g){ showContextMenu(g, urls); });
    connect(miller, &MillerView::emptySpaceContextMenuRequested, this, [this](const QUrl &folderUrl, const QPoint &g){ showEmptySpaceContextMenu(g, folderUrl); });
    connect(miller, &MillerView::selectionChanged, this, [this](const QUrl &url){
        if (m_previewVisible) updatePreviewForUrl(url);
        // Update QuickLook if it's open
        if (ql && ql->isVisible() && url.isLocalFile()) {
            ql->showFile(url.toLocalFile());
        }
    });
    connect(miller, &MillerView::navigatedTo, this, [this](const QUrl &url){ emit urlChanged(url); });

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
            QModelIndex index = v->indexAt(pos);
            if (index.isValid()) {
                // Finder-like behavior: right-click an unselected item to act on that item.
                if (QItemSelectionModel *sel = v->selectionModel();
                    sel && !sel->isSelected(index)) {
                    sel->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    v->setCurrentIndex(index);
                }
                showContextMenu(v->viewport()->mapToGlobal(pos));
            } else {
                showEmptySpaceContextMenu(pos);
            }
        });
        connect(v->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &Pane::onCurrentChanged);
        connect(v->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &Pane::updateStatus);
        connect(v, &QAbstractItemView::activated, this, &Pane::onActivated); // double-click opens
        v->setEditTriggers(QAbstractItemView::NoEditTriggers);

        connect(v, &QAbstractItemView::clicked, this, [this, v](const QModelIndex &idx) {
            if (!idx.isValid()) return;
            if (!v->selectionModel() || !v->selectionModel()->isSelected(idx)) return;

            const bool sameTarget = (m_renameClickView == v && m_renameClickIndex == idx);
            const qint64 elapsedMs = m_renameClickTimer.isValid() ? m_renameClickTimer.elapsed() : -1;
            // Finder-style slow second click on selected item starts rename.
            if (sameTarget && elapsedMs >= 500 && elapsedMs <= 2000) {
                beginInlineRename(v, idx);
                m_renameClickTimer.invalidate();
                m_renameClickIndex = QPersistentModelIndex();
                return;
            }

            m_renameClickView = v;
            m_renameClickIndex = idx;
            m_renameClickTimer.restart();
        });
        
        // Install spacebar event filter
        v->installEventFilter(this);
    };
    hookSel(iconView);
    hookSel(detailsView);
    hookSel(compactView);

    applyIconSize(64);
    setRoot(startUrl);

    // Defer view mode initialization until model is loaded to avoid race condition
    // where the view shows stale state before model populates
    connect(dirModel->dirLister(), &KDirLister::completed, this, [this]() {
        // Only run once on initial load (per Pane instance)
        if (!m_viewInitialized) {
            m_viewInitialized = true;
            // Sync view box and view mode after model is ready
            int savedView = QSettings().value("general/defaultView", 3).toInt();  // Miller Columns by default
            viewBox->setCurrentIndex(savedView);
            setViewMode(savedView);
        }
    }, Qt::QueuedConnection);

    // Set initial view box state (visual only, actual view set after model loads)
    viewBox->setCurrentIndex(0);
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
    emit urlChanged(url);
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
        FileOpsService::openUrl(url, this);
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
    if (stack->currentWidget() == miller) {
        const QList<QUrl> urls = miller->getSelectedUrls();
        if (urls.isEmpty() || !urls.first().isLocalFile()) return;
        if (!ql) ql = new QuickLookDialog(this);
        ql->showFile(urls.first().toLocalFile());
        return;
    }

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
        if (stack->currentWidget() == miller) {
            const QList<QUrl> urls = miller->getSelectedUrls();
            if (!urls.isEmpty() && urls.first().isValid()) {
                updatePreviewForUrl(urls.first());
            } else {
                clearPreview();
            }
        } else if (auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
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

void Pane::showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls)
{
    // Get effective selection - either passed URLs or current selection
    QList<QUrl> selectedUrls = urls;
    if (selectedUrls.isEmpty()) {
        selectedUrls = getSelectedUrls();
    }
    if (selectedUrls.isEmpty()) {
        // Try current index as fallback
        QAbstractItemView *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
        if (v) {
            QModelIndex idx = v->currentIndex();
            if (idx.isValid()) {
                QUrl u = urlForIndex(idx);
                if (u.isValid()) selectedUrls.append(u);
            }
        }
    }
    if (selectedUrls.isEmpty()) return;

    const int count = selectedUrls.size();
    const bool multiSelect = count > 1;
    const QUrl firstUrl = selectedUrls.first();
    const QFileInfo firstFi(firstUrl.toLocalFile());

    QMenu menu;

    // --- Open actions ---
    QAction *actOpen = menu.addAction(multiSelect ? QString("Open %1 Items").arg(count) : "Open");
    actOpen->setShortcut(QKeySequence("Ctrl+Down"));

    QAction *actOpenWith = menu.addAction("Open With...");
    actOpenWith->setEnabled(!multiSelect);  // Only for single selection

    QAction *actQL = menu.addAction("Quick Look");
    actQL->setShortcut(Qt::Key_Space);

    menu.addSeparator();

    // --- Edit actions ---
    QAction *actCut = menu.addAction(multiSelect ? QString("Cut %1 Items").arg(count) : "Cut");
    actCut->setShortcut(QKeySequence::Cut);

    QAction *actCopy = menu.addAction(multiSelect ? QString("Copy %1 Items").arg(count) : "Copy");
    actCopy->setShortcut(QKeySequence::Copy);

    // Paste behavior: if right-clicking a single folder, offer to paste INTO it
    const QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    bool canPaste = mimeData && mimeData->hasUrls();
    bool isCutOp = canPaste && isClipboardCutOperation();

    // Determine paste destination:
    // - Right-click folder → paste INTO that folder
    // - Right-click file → paste into same directory as that file (not currentRoot, which may be stale in Miller view)
    QUrl pasteDestination;
    bool pasteIntoFolder = false;
    if (!multiSelect && firstFi.isDir()) {
        pasteDestination = firstUrl;
        pasteIntoFolder = true;
    } else {
        // Use the parent directory of the selected file
        pasteDestination = QUrl::fromLocalFile(firstFi.absolutePath());
    }

    QString pasteLabel;
    if (pasteIntoFolder) {
        pasteLabel = isCutOp ? QString("Move Into \"%1\"").arg(firstFi.fileName())
                             : QString("Paste Into \"%1\"").arg(firstFi.fileName());
    } else {
        pasteLabel = isCutOp ? "Move Here" : "Paste";
    }

    QAction *actPaste = menu.addAction(pasteLabel);
    actPaste->setShortcut(QKeySequence::Paste);
    actPaste->setEnabled(canPaste);

    menu.addSeparator();

    QAction *actDuplicate = menu.addAction(multiSelect ? QString("Duplicate %1 Items").arg(count) : "Duplicate");
    actDuplicate->setShortcut(QKeySequence("Ctrl+D"));

    menu.addSeparator();

    // --- Rename (only for single selection) ---
    QAction *actRename = menu.addAction("Rename");
    actRename->setShortcut(Qt::Key_F2);
    actRename->setEnabled(!multiSelect);

    // --- Delete actions ---
    QAction *actTrash = menu.addAction(multiSelect ? QString("Move %1 Items to Trash").arg(count) : "Move to Trash");
    actTrash->setShortcut(QKeySequence("Ctrl+Backspace"));

    QAction *actDelete = menu.addAction(multiSelect ? QString("Delete %1 Items Permanently").arg(count) : "Delete Permanently");
    actDelete->setShortcut(QKeySequence("Shift+Delete"));

    menu.addSeparator();

    // --- Compress/Extract ---
    QAction *actCompress = nullptr;
    QAction *actExtract = nullptr;

    // Show Extract only for single archive selection
    if (!multiSelect && isArchiveFile(firstUrl.toLocalFile())) {
        actExtract = menu.addAction("Extract Here...");
    } else {
        // Show Compress for any selection (including multiple files)
        actCompress = menu.addAction(multiSelect ? QString("Compress %1 Items...").arg(count) : "Compress...");
    }

    menu.addSeparator();

    // --- Folder operations ---
    QAction *actNewFolder = menu.addAction("New Folder");
    actNewFolder->setShortcut(QKeySequence("Ctrl+Shift+N"));

    menu.addSeparator();

    // --- Properties ---
    QAction *actProperties = menu.addAction(multiSelect ? QString("Properties (%1 Items)").arg(count) : "Properties");
    actProperties->setShortcut(QKeySequence("Alt+Return"));

    // Execute menu
    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;

    // Handle actions
    if (chosen == actOpen) {
        for (const QUrl &url : selectedUrls) {
            FileOpsService::openUrl(url, this);
        }
        return;
    }
    if (chosen == actOpenWith) {
        showOpenWithDialog(firstUrl);
        return;
    }
    if (chosen == actQL) {
        if (!ql) ql = new QuickLookDialog(this);
        ql->showFile(firstUrl.toLocalFile());
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
        pasteFilesToDestination(pasteDestination);
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
    if (chosen == actTrash) {
        moveToTrash();
        return;
    }
    if (chosen == actDelete) {
        deleteSelectedPermanently();
        return;
    }
    if (chosen == actCompress) {
        compressSelected();
        return;
    }
    if (chosen == actExtract) {
        extractArchive(firstUrl);
        return;
    }
    if (chosen == actNewFolder) {
        createNewFolder();
        return;
    }
    if (chosen == actProperties) {
        PropertiesDialog *dialog;
        if (selectedUrls.size() == 1) {
            dialog = new PropertiesDialog(selectedUrls.first(), this);
        } else {
            dialog = new PropertiesDialog(selectedUrls, this);
        }
        dialog->exec();
        dialog->deleteLater();
        return;
    }
}

void Pane::showHeaderContextMenu(const QPoint &pos) {
    if (!detailsView) return;
    
    QHeaderView *header = detailsView->header();
    QMenu menu(this);
    
    // Add options for each column (in KDirModel order: Name, Size, ModifiedTime, Permissions, Owner, Group, Type)
    QStringList columnNames = {"Name", "Size", "Date Modified", "Permissions", "Owner", "Group", "Type"};
    
    for (int i = 0; i < columnNames.size(); ++i) {
        QAction *action = menu.addAction(columnNames[i]);
        action->setCheckable(true);
        action->setChecked(!header->isSectionHidden(i));
        action->setData(i);
        
        connect(action, &QAction::toggled, this, [this, i](bool visible) {
            if (detailsView && detailsView->header()) {
                detailsView->header()->setSectionHidden(i, !visible);
                // Save column visibility setting
                QSettings settings;
                settings.setValue(QString("view/column%1Visible").arg(i), visible);
            }
        });
    }
    
    menu.addSeparator();
    menu.addAction("Reset to Default", [this]() {
        if (detailsView && detailsView->header()) {
            QHeaderView *header = detailsView->header();
            for (int i = 0; i < 7; ++i) {
                header->setSectionHidden(i, false);
            }
            header->resizeSections(QHeaderView::ResizeToContents);
        }
    });
    
    menu.exec(header->mapToGlobal(pos));
}

void Pane::showEmptySpaceContextMenu(const QPoint &pos, const QUrl &targetFolder) {
    // For miller view, pos is already global; for other views, it's local
    auto *view = qobject_cast<QAbstractItemView*>(sender());
    QPoint globalPos = pos;

    // Determine paste destination - use targetFolder if provided, otherwise currentRoot
    QUrl pasteDestination = targetFolder.isValid() ? targetFolder : currentRoot;

    // Check if this is from a regular view (not miller)
    if (view) {
        QModelIndex index = view->indexAt(pos);
        if (index.isValid()) return; // Clicked on an item, don't show empty space menu
        globalPos = view->mapToGlobal(pos);
    }

    QMenu menu(this);

    // Add basic folder operations
    QAction *newFolderAction = menu.addAction("New Folder");
    newFolderAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    connect(newFolderAction, &QAction::triggered, this, [this, pasteDestination]() {
        createNewFolderIn(pasteDestination);
    });

    menu.addSeparator();

    // Add paste if clipboard has URLs
    const QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    bool canPaste = mimeData && mimeData->hasUrls();
    bool isCutOp = canPaste && isClipboardCutOperation();
    QAction *pasteAction = menu.addAction(isCutOp ? "Move Items Here" : "Paste");
    pasteAction->setEnabled(canPaste);
    pasteAction->setShortcut(QKeySequence::Paste);

    connect(pasteAction, &QAction::triggered, this, [this, pasteDestination]() {
        pasteFilesToDestination(pasteDestination);
    });

    menu.addSeparator();

    // View options
    auto *viewMenu = menu.addMenu("View");
    viewMenu->addAction("Icons", [this]() { setViewMode(0); });
    viewMenu->addAction("Details", [this]() { setViewMode(1); });
    viewMenu->addAction("Compact", [this]() { setViewMode(2); });
    viewMenu->addAction("Miller Columns", [this]() { setViewMode(3); });

    viewMenu->addSeparator();
    QAction *showHiddenAction = viewMenu->addAction("Show Hidden Files");
    showHiddenAction->setCheckable(true);
    showHiddenAction->setChecked(m_showHiddenFiles);
    connect(showHiddenAction, &QAction::toggled, this, &Pane::setShowHiddenFiles);

    menu.exec(globalPos);
}

// ---- File Operations Implementation ----

QList<QUrl> Pane::selectedUrls() const {
    QList<QUrl> urls = getSelectedUrls();
    if (!urls.isEmpty()) return urls;

    // Fallback to current item for non-Miller views.
    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (v) {
        QUrl current = urlForIndex(v->currentIndex());
        if (current.isValid()) urls.append(current);
    }
    return urls;
}

QList<QUrl> Pane::getSelectedUrls() const {
    QList<QUrl> urls;

    // Handle Miller view specially (it's not a QAbstractItemView)
    if (stack->currentWidget() == miller) {
        return miller->getSelectedUrls();
    }

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
    QList<QUrl> urls = getSelectedUrls();
    if (urls.isEmpty()) return;

    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);

    // For GNOME/Nautilus compatibility: x-special/gnome-copied-files format
    // Format: "copy\nfile:///path1\nfile:///path2..."
    QByteArray gnomeData = "copy";
    for (const QUrl &url : urls) {
        gnomeData += '\n' + url.toEncoded();
    }
    mimeData->setData("x-special/gnome-copied-files", gnomeData);

    // Note: KDE's x-kde-cutselection is NOT set for copy (absence = copy)

    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void Pane::cutSelected() {
    QList<QUrl> urls = getSelectedUrls();
    if (urls.isEmpty()) return;

    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);

    // KDE/Dolphin: mark as cut operation
    mimeData->setData("application/x-kde-cutselection", "1");

    // GNOME/Nautilus compatibility: x-special/gnome-copied-files format
    // Format: "cut\nfile:///path1\nfile:///path2..."
    QByteArray gnomeData = "cut";
    for (const QUrl &url : urls) {
        gnomeData += '\n' + url.toEncoded();
    }
    mimeData->setData("x-special/gnome-copied-files", gnomeData);

    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void Pane::pasteFiles() {
    pasteFilesToDestination(currentRoot);
}

void Pane::pasteFilesToDestination(const QUrl &destination) {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (!mimeData || !mimeData->hasUrls()) return;

    QList<QUrl> urls = mimeData->urls();
    if (urls.isEmpty()) return;

    // Check if cut operation - support both KDE and GNOME formats
    bool isCut = false;

    // KDE format: application/x-kde-cutselection == "1"
    QByteArray kdeData = mimeData->data("application/x-kde-cutselection");
    if (kdeData == QByteArray("1")) {
        isCut = true;
    }

    // GNOME format: x-special/gnome-copied-files starts with "cut\n"
    if (!isCut) {
        QByteArray gnomeData = mimeData->data("x-special/gnome-copied-files");
        if (gnomeData.startsWith("cut\n") || gnomeData.startsWith("cut\r")) {
            isCut = true;
        }
    }

    if (isCut) {
        FileOpsService::move(urls, destination, this);
        // Clear clipboard after move to prevent re-moving
        QGuiApplication::clipboard()->clear();
    } else {
        FileOpsService::copy(urls, destination, this);
    }
}

bool Pane::isClipboardCutOperation() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData) return false;

    // KDE format
    if (mimeData->data("application/x-kde-cutselection") == QByteArray("1")) {
        return true;
    }

    // GNOME format
    QByteArray gnomeData = mimeData->data("x-special/gnome-copied-files");
    if (gnomeData.startsWith("cut\n") || gnomeData.startsWith("cut\r")) {
        return true;
    }

    return false;
}

void Pane::deleteSelected() {
    const auto urls = selectedUrls();
    if (urls.isEmpty()) return;

    QSettings settings;
    const bool moveToTrashByDefault = settings.value("advanced/moveToTrash", true).toBool();
    const bool confirmDelete = settings.value("advanced/confirmDelete", true).toBool();

    if (moveToTrashByDefault) {
        if (confirmDelete && !confirmDeleteAction(this, urls, false)) return;
        FileOpsService::trash(urls, this);
        return;
    }

    deleteSelectedPermanently();
}

void Pane::deleteSelectedPermanently() {
    const auto urls = selectedUrls();
    if (urls.isEmpty()) return;

    QSettings settings;
    const bool confirmDelete = settings.value("advanced/confirmDelete", true).toBool();
    if (confirmDelete && !confirmDeleteAction(this, urls, true)) return;

    FileOpsService::del(urls, this);
}

void Pane::moveToTrash() {
    const auto urls = selectedUrls();
    if (urls.isEmpty()) return;

    QSettings settings;
    const bool confirmDelete = settings.value("advanced/confirmDelete", true).toBool();
    if (confirmDelete && !confirmDeleteAction(this, urls, false)) return;

    // Use KIO::trash for safe deletion (moves to trash)
    FileOpsService::trash(urls, this);
}

void Pane::renameSelected() {
    if (stack->currentWidget() == miller) {
        miller->renameSelected();
        return;
    }

    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (!v || !v->selectionModel()) return;

    const QModelIndex current = v->selectionModel()->currentIndex();
    if (!current.isValid()) return;

    beginInlineRename(v, current);
}

void Pane::beginInlineRename(QAbstractItemView *view, const QModelIndex &index) {
    if (!view || !index.isValid()) return;
    view->setCurrentIndex(index);
    view->setEditTriggers(QAbstractItemView::AllEditTriggers);
    view->edit(index);

    // Connect to delegate's closeEditor to restore no-edit state when done
    // Use a queued single-shot to avoid issues with multiple connections
    QAbstractItemDelegate *delegate = view->itemDelegate();
    if (delegate) {
        // Disconnect any previous connections to avoid stacking
        disconnect(delegate, &QAbstractItemDelegate::closeEditor, nullptr, nullptr);
        connect(delegate, &QAbstractItemDelegate::closeEditor, this, [view]() {
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        }, Qt::QueuedConnection);
    }
}

void Pane::createNewFolder() {
    createNewFolderIn(currentRoot);
}

void Pane::createNewFolderIn(const QUrl &targetFolder) {
    QUrl destinationUrl = targetFolder;
    if (!destinationUrl.isValid() || !destinationUrl.isLocalFile()) {
        destinationUrl = currentRoot;
    }

    if (!destinationUrl.isValid() || !destinationUrl.isLocalFile()) {
        return;
    }

    const QString destinationPath = destinationUrl.toLocalFile();

    // Create a uniquely named new folder
    QString baseName = "New Folder";
    QString folderName = baseName;
    int counter = 1;
    
    QDir currentDir(destinationPath);
    while (currentDir.exists(folderName)) {
        folderName = QString("%1 %2").arg(baseName).arg(counter++);
    }
    
    QString newFolderPath = currentDir.filePath(folderName);
    if (QDir().mkpath(newFolderPath)) {
        // Refresh the directory listing to show the new folder
        if (dirModel) {
            if (auto *lister = dirModel->dirLister()) {
                lister->updateDirectory(destinationUrl);
            }
        }
        
        if (proxy) {
            proxy->invalidate();
        }
        
        updateStatus();
        
        // Only attempt in-view selection when created in the active folder.
        if (destinationUrl == currentRoot) {
            QTimer::singleShot(100, this, [this, folderName]() {
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
}

void Pane::goBack() {
    if (!canGoBack()) return;
    
    m_historyIndex--;
    QUrl url = m_history[m_historyIndex];
    
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

void Pane::generateThumbnail(const QUrl &url) const {
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
        // Generate thumbnail (thumbs is mutable, generateThumbnail is const)
        generateThumbnail(url);
    }
    
    // Return default icon for now
    QFileIconProvider provider;
    return provider.icon(QFileInfo(path));
}

void Pane::updateStatus() {
    if (!proxy) return;

    int totalFiles = proxy->rowCount();
    int selectedFiles = 0;
    qint64 selectedSize = 0;

    if (stack->currentWidget() == miller) {
        const QList<QUrl> selected = miller->getSelectedUrls();
        selectedFiles = selected.size();
        for (const QUrl &url : selected) {
            if (!url.isLocalFile()) continue;
            QFileInfo fi(url.toLocalFile());
            if (fi.isFile()) {
                selectedSize += fi.size();
            }
        }
        emit statusChanged(totalFiles, selectedFiles, selectedSize);
        return;
    }

    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (v && v->selectionModel()) {
        const auto selectedRows = v->selectionModel()->selectedRows();
        selectedFiles = selectedRows.count();

        // Calculate total size of selected files
        for (const auto &idx : selectedRows) {
            QUrl url = urlForIndex(idx);
            if (url.isLocalFile()) {
                QFileInfo fi(url.toLocalFile());
                if (fi.isFile()) {
                    selectedSize += fi.size();
                }
            }
        }
    }

    emit statusChanged(totalFiles, selectedFiles, selectedSize);
}

void Pane::openSelected() {
    if (stack->currentWidget() == miller) {
        const QList<QUrl> urls = miller->getSelectedUrls();
        if (urls.isEmpty()) return;

        const QUrl url = urls.first();
        if (!url.isValid()) return;

        QFileInfo fi(url.toLocalFile());
        if (fi.isDir()) {
            setRoot(url);
        } else {
            FileOpsService::openUrl(url, this);
        }

        if (m_previewVisible) updatePreviewForUrl(url);
        return;
    }

    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (!v || !v->selectionModel()) return;

    QModelIndex idx = v->currentIndex();
    if (!idx.isValid()) return;

    onActivated(idx);
}

void Pane::setZoomValue(int value) {
    if (zoom) {
        zoom->setValue(value);
    }
    applyIconSize(value);
}

void Pane::focusView() {
    // Focus the current view widget
    if (stack->currentWidget() == miller) {
        miller->focusLastColumn();
    } else if (auto *view = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
        view->setFocus();
    }
}

bool Pane::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *view = qobject_cast<QAbstractItemView*>(obj);
        if (!view) {
            return QWidget::eventFilter(obj, event);
        }

        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Space) {
            // Handle spacebar for QuickLook in all view modes
            QModelIndex current = view->currentIndex();
            if (current.isValid()) {
                QUrl url = urlForIndex(current);
                if (url.isValid()) {
                    quickLookSelected();
                    return true; // Event handled
                }
            }
        }

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QModelIndex current = view->currentIndex();
            if (!current.isValid()) {
                return true;
            }

            if (keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
                onActivated(current);  // Ctrl+Enter opens selection.
            } else {
                beginInlineRename(view, current);  // Return renames selection.
            }
            return true;
        }

        if (keyEvent->key() == Qt::Key_F2) {
            QModelIndex current = view->currentIndex();
            if (current.isValid()) {
                beginInlineRename(view, current);
                return true;
            }
        }

        if (!keyEvent->modifiers().testFlag(Qt::ControlModifier)
            && !keyEvent->modifiers().testFlag(Qt::AltModifier)
            && !keyEvent->modifiers().testFlag(Qt::MetaModifier)) {
            // Typing any navigation/search character should cancel pending slow-click rename gesture.
            const QString typed = keyEvent->text();
            if (!typed.isEmpty() && typed.at(0).isPrint()) {
                m_renameClickTimer.invalidate();
                m_renameClickIndex = QPersistentModelIndex();
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

    // Create dialog
    QDialog dialog(this);
    dialog.setWindowTitle(QString("Open %1 with...").arg(fi.fileName()));
    dialog.setModal(true);
    dialog.resize(450, 350);

    auto *layout = new QVBoxLayout(&dialog);

    auto *label = new QLabel(QString("Choose an application to open <b>%1</b>:").arg(fi.fileName()));
    label->setWordWrap(true);
    layout->addWidget(label);

    auto *listWidget = new QListWidget();
    listWidget->setIconSize(QSize(24, 24));
    layout->addWidget(listWidget);

    // Query KDE for applications that can handle this MIME type
    const KService::List services = KApplicationTrader::queryByMimeType(mimeTypeName);

    for (const KService::Ptr &service : services) {
        auto *item = new QListWidgetItem(QIcon::fromTheme(service->icon()), service->name());
        item->setData(Qt::UserRole, service->desktopEntryName());
        item->setData(Qt::UserRole + 1, QVariant::fromValue(service));
        item->setToolTip(service->comment());
        listWidget->addItem(item);
    }

    // Also query for all applications if the MIME-specific list is short
    if (services.count() < 5) {
        listWidget->addItem(new QListWidgetItem("──── Other Applications ────"));
        listWidget->item(listWidget->count() - 1)->setFlags(Qt::NoItemFlags);

        // Add some common general-purpose apps
        const QStringList commonApps = {"org.kde.kate", "org.kde.dolphin", "firefox", "org.gnome.gedit"};
        for (const QString &appId : commonApps) {
            KService::Ptr service = KService::serviceByDesktopName(appId);
            if (service && !service->exec().isEmpty()) {
                // Check if already in list
                bool found = false;
                for (int i = 0; i < listWidget->count(); ++i) {
                    if (listWidget->item(i)->data(Qt::UserRole).toString() == appId) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    auto *item = new QListWidgetItem(QIcon::fromTheme(service->icon()), service->name());
                    item->setData(Qt::UserRole, service->desktopEntryName());
                    item->setData(Qt::UserRole + 1, QVariant::fromValue(service));
                    listWidget->addItem(item);
                }
            }
        }
    }

    // Add custom command option
    auto *customItem = new QListWidgetItem(QIcon::fromTheme("system-run"), "Run custom command...");
    customItem->setData(Qt::UserRole, "custom");
    listWidget->addItem(customItem);

    // Select first item
    if (listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
    }

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    connect(listWidget, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted) {
        auto *selectedItem = listWidget->currentItem();
        if (!selectedItem) return;

        QString appId = selectedItem->data(Qt::UserRole).toString();

        if (appId == "custom") {
            // Show input dialog for custom command
            bool ok;
            QString customCommand = QInputDialog::getText(&dialog, "Custom Command",
                                                        "Enter command to open file with:",
                                                        QLineEdit::Normal, "", &ok);
            if (ok && !customCommand.isEmpty()) {
                QStringList arguments;
                arguments << filePath;
                if (!QProcess::startDetached(customCommand, arguments)) {
                    QMessageBox::warning(this, "Error",
                                        QString("Failed to open file with %1").arg(customCommand));
                }
            }
        } else {
            // Use KDE's application launcher for proper integration
            KService::Ptr service = selectedItem->data(Qt::UserRole + 1).value<KService::Ptr>();
            if (service) {
                auto *job = new KIO::ApplicationLauncherJob(service);
                job->setUrls({url});
                job->start();
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
        case 2: column = 6; break; // Type
        case 3: column = 2; break; // Date Modified
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
