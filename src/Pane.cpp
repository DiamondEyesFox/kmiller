// Local headers
#include "Pane.h"
#include "MillerView.h"
#include "QuickLookDialog.h"
#include "ThumbCache.h"
#include "DialogUtils.h"
#include "PropertiesDialog.h"
#include "FileOpsService.h"
#include "ArchiveService.h"
#include "OpenWithService.h"
#include <KFilePreviewGenerator>

// Qt Core
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QMimeData>
#include <QMimeDatabase>
#include <QProcess>
#include <QSignalBlocker>
#include <QSettings>
#include <algorithm>
#include <QTimer>
#include <QStandardPaths>
#include <QFileSystemModel>
#include <QSet>
#include <functional>
#include <memory>

// Qt GUI
#include <QClipboard>
#include <QGuiApplication>
#include <QImageReader>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QFileIconProvider>

// Qt Widgets
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
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
#include <KDirLister>
#include <KDirModel>
#include <KDirSortFilterProxyModel>
#include <KFileItem>
#include <KJob>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/MkdirJob>
#include <KUrlNavigator>
#include <KIO/EmptyTrashJob>

// External libraries
#include <poppler-qt6.h>

static bool isImageFile(const QString &path) {
    const QByteArray fmt = QImageReader::imageFormat(path);
    return !fmt.isEmpty();
}

static bool isArchiveFile(const QString &path) {
    return ArchiveService::canExtractArchive(path);
}

struct ArchiveFormatOption {
    QString label;
    QString extension;
};

static QList<ArchiveFormatOption> archiveFormatOptions() {
    return {
        {QStringLiteral("ZIP (.zip)"), QStringLiteral(".zip")},
        {QStringLiteral("7-Zip (.7z)"), QStringLiteral(".7z")},
        {QStringLiteral("Tar (.tar)"), QStringLiteral(".tar")},
        {QStringLiteral("Tar + gzip (.tar.gz)"), QStringLiteral(".tar.gz")},
        {QStringLiteral("Tar + bzip2 (.tar.bz2)"), QStringLiteral(".tar.bz2")},
        {QStringLiteral("Tar + xz (.tar.xz)"), QStringLiteral(".tar.xz")}
    };
}

static QStringList knownArchiveExtensions() {
    QStringList extensions;
    const QList<ArchiveFormatOption> options = archiveFormatOptions();
    extensions.reserve(options.size());
    for (const ArchiveFormatOption &option : options) {
        extensions.append(option.extension);
    }
    extensions << QStringLiteral(".tgz")
               << QStringLiteral(".tbz2")
               << QStringLiteral(".tbz")
               << QStringLiteral(".txz")
               << QStringLiteral(".rar");
    std::sort(extensions.begin(), extensions.end(), [](const QString &left, const QString &right) {
        return left.size() > right.size();
    });
    return extensions;
}

static QString stripKnownArchiveExtension(const QString &name) {
    const QString lowerName = name.toLower();
    const QStringList extensions = knownArchiveExtensions();
    for (const QString &extension : extensions) {
        if (lowerName.endsWith(extension)) {
            return name.left(name.size() - extension.size());
        }
    }
    return name;
}

static int archiveNameSelectionLength(const QString &archiveName) {
    return stripKnownArchiveExtension(archiveName).size();
}

static QProgressDialog *createBusyProgressDialog(QWidget *parent,
                                                 const QString &title,
                                                 const QString &labelText) {
    auto *dialog = new QProgressDialog(labelText, QString(), 0, 0, parent);
    dialog->setWindowTitle(title);
    dialog->setCancelButton(nullptr);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->setMinimumDuration(0);
    dialog->setAutoClose(false);
    dialog->setAutoReset(false);
    dialog->show();
    return dialog;
}

static QString uniqueChildPath(const QString &parentDirPath, const QString &preferredName) {
    QDir parentDir(parentDirPath);
    QString candidate = preferredName.trimmed();
    if (candidate.isEmpty()) {
        candidate = QStringLiteral("Extracted");
    }

    QString childPath = parentDir.filePath(candidate);
    int counter = 2;
    while (QFileInfo::exists(childPath)) {
        childPath = parentDir.filePath(QStringLiteral("%1 %2").arg(candidate).arg(counter++));
    }
    return childPath;
}

static QString summarizeConflictPaths(const QStringList &relativePaths, int maxItems = 5) {
    QStringList lines;
    const int previewCount = qMin(maxItems, relativePaths.size());
    for (int i = 0; i < previewCount; ++i) {
        lines.append(relativePaths.at(i));
    }
    if (relativePaths.size() > previewCount) {
        lines.append(QStringLiteral("..."));
    }
    return lines.join(QLatin1Char('\n'));
}

static QPixmap getFileTypeIcon(const QFileInfo &fi, int size = 128) {
    static const QMimeDatabase db;
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

static void refreshPaneAfterJob(Pane *pane, KJob *job, const QString &successMessage = QString()) {
    if (!pane || !job) {
        return;
    }

    QObject::connect(job, &KJob::result, pane, [pane, successMessage](KJob *finishedJob) {
        pane->refreshCurrentLocation();
        if (finishedJob->error()) {
            emit pane->statusChanged(0, 0, 0, finishedJob->errorString());
        } else if (!successMessage.isEmpty()) {
            emit pane->statusChanged(0, 0, 0, successMessage);
        }
    });
}

static QList<QUrl> parentDirectoriesForUrls(const QList<QUrl> &urls) {
    QList<QUrl> parents;
    QSet<QString> seen;

    for (const QUrl &url : urls) {
        QUrl parentUrl;
        if (url.isLocalFile()) {
            const QString parentPath = QFileInfo(url.toLocalFile()).absolutePath();
            parentUrl = QUrl::fromLocalFile(parentPath);
        } else {
            parentUrl = url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
        }

        const QString key = parentUrl.toString();
        if (parentUrl.isValid() && !seen.contains(key)) {
            seen.insert(key);
            parents.append(parentUrl);
        }
    }

    return parents;
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
    searchBox->setPlaceholderText("Search current folder tree...");
    searchBox->setToolTip("Recursive search. Clear the field to return to the previous folder.");
    searchBox->setFixedWidth(150);
    searchBox->setClearButtonEnabled(true);
    tb->addWidget(searchBox);

    // Debounce timer for recursive search
    m_searchDebounce = new QTimer(this);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(300);

    root->addWidget(tb);

    nav = new KUrlNavigator(this);
    // Finder-like default: always start in clickable breadcrumb mode.
    nav->setUrlEditable(false);
    nav->setShowFullPath(true);
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

    // Empty folder overlay label
    m_emptyFolderLabel = new QLabel("This folder is empty", stack);
    m_emptyFolderLabel->setAlignment(Qt::AlignCenter);
    m_emptyFolderLabel->setStyleSheet(
        "QLabel { color: rgba(128, 128, 128, 180); font-size: 16px; font-style: italic; }");
    m_emptyFolderLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_emptyFolderLabel->hide();

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
    
    // Connect to model changes for status updates and empty folder overlay
    connect(proxy, &QAbstractItemModel::rowsInserted, this, &Pane::updateStatus);
    connect(proxy, &QAbstractItemModel::rowsRemoved, this, &Pane::updateStatus);
    connect(proxy, &QAbstractItemModel::modelReset, this, &Pane::updateStatus);
    connect(proxy, &QAbstractItemModel::rowsInserted, this, &Pane::updateEmptyFolderOverlay);
    connect(proxy, &QAbstractItemModel::rowsRemoved, this, &Pane::updateEmptyFolderOverlay);
    connect(proxy, &QAbstractItemModel::modelReset, this, &Pane::updateEmptyFolderOverlay);
    connect(dirModel->dirLister(), &KDirLister::completed, this, &Pane::updateEmptyFolderOverlay);

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
    iconView->setMovement(QListView::Snap);  // Snap to grid so arrow keys navigate in grid pattern
    iconView->setUniformItemSizes(true);
    iconView->setGridSize(QSize(100, 100)); // Finder-like grid spacing
    iconView->setSpacing(8); // Space between items
    iconView->setWordWrap(true);
    iconView->setTextElideMode(Qt::ElideMiddle);
    iconView->setContextMenuPolicy(Qt::CustomContextMenu);
    
    stack->addWidget(iconView);

    // Dolphin-style async thumbnail generation for icon view.
    // KFilePreviewGenerator handles visible-first priority, scroll pausing,
    // and cut-item dimming automatically.
    m_previewGenerator = new KFilePreviewGenerator(iconView);
    m_previewGenerator->setPreviewShown(true);

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
        connect(sc, &QShortcut::activated, v, [v]{ v->selectAll(); });
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
    connect(miller, &MillerView::navigatedTo, this, [this](const QUrl &url){
        if (!url.isValid()) return;
        currentRoot = url;
        syncNavigatorLocation(url);
        emit urlChanged(url);
    });

    ql = new QuickLookDialog(this);
    thumbs = new ThumbCache(this);

    connect(viewBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &Pane::onViewModeChanged);
    connect(zoom, &QSlider::valueChanged, this, &Pane::onZoomChanged);
    connect(nav, &KUrlNavigator::urlChanged, this, &Pane::onNavigatorUrlChanged);
    // Keep breadcrumb mode locked UNLESS user explicitly requested edit via Ctrl+L/F6.
    connect(nav, &KUrlNavigator::editableStateChanged, this, [this](bool editable) {
        if (editable && nav && !m_pathBarEditRequested) {
            nav->setUrlEditable(false);
        }
    });

    // Ctrl+L / F6: toggle path bar to editable mode (Dolphin convention)
    auto *editPathShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this);
    connect(editPathShortcut, &QShortcut::activated, this, [this]() {
        if (!nav) return;
        m_pathBarEditRequested = true;
        nav->setUrlEditable(true);
        // Focus the line edit inside KUrlNavigator
        if (auto *le = nav->findChild<QLineEdit*>()) {
            le->setFocus();
            le->selectAll();
        }
    });
    auto *editPathShortcutF6 = new QShortcut(QKeySequence(Qt::Key_F6), this);
    connect(editPathShortcutF6, &QShortcut::activated, this, [this]() {
        if (!nav) return;
        m_pathBarEditRequested = true;
        nav->setUrlEditable(true);
        if (auto *le = nav->findChild<QLineEdit*>()) {
            le->setFocus();
            le->selectAll();
        }
    });

    // When Enter is pressed in the editable path bar, navigate and revert to breadcrumb mode.
    connect(nav, &KUrlNavigator::returnPressed, this, [this]() {
        m_pathBarEditRequested = false;
        nav->setUrlEditable(false);
        focusView();
    });

    // Escape in the path bar line edit: cancel editing, revert to breadcrumb mode.
    if (auto *le = nav->findChild<QLineEdit*>()) {
        auto *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), le);
        escShortcut->setContext(Qt::WidgetShortcut);
        connect(escShortcut, &QShortcut::activated, this, [this]() {
            m_pathBarEditRequested = false;
            nav->setUrlEditable(false);
            focusView();
        });
    }
    
    // Recursive search via KIO filenamesearch:// protocol
    connect(searchBox, &QLineEdit::textChanged, this, [this](const QString &text){
        m_searchDebounce->stop();
        if (text.isEmpty()) {
            searchBox->setStyleSheet(QString());
            // Clear search: navigate back to pre-search URL
            if (m_preSearchUrl.isValid()) {
                QUrl restoreUrl = m_preSearchUrl;
                m_preSearchUrl = QUrl();  // clear before navigating to avoid re-saving
                setRoot(restoreUrl);
            }
        } else {
            searchBox->setStyleSheet("QLineEdit { border: 1px solid #6db3ff; border-radius: 4px; }");
            m_searchDebounce->start();
        }
    });
    connect(m_searchDebounce, &QTimer::timeout, this, [this](){
        const QString text = searchBox->text().trimmed();
        if (text.isEmpty()) return;
        // Save current location before first search
        if (!m_preSearchUrl.isValid()) {
            m_preSearchUrl = currentRoot;
        }
        QUrl searchUrl(QStringLiteral("filenamesearch:/?search=%1&url=%2&checkContent=no")
            .arg(QString::fromUtf8(QUrl::toPercentEncoding(text)),
                 QString::fromUtf8(QUrl::toPercentEncoding(m_preSearchUrl.toString()))));
        emit statusChanged(0, 0, 0, QString("Searching for \"%1\"").arg(text));
        setRoot(searchUrl);
    });
    connect(searchBox, &QLineEdit::returnPressed, this, [this](){
        // Immediate search on Enter
        m_searchDebounce->stop();
        const QString text = searchBox->text().trimmed();
        if (text.isEmpty()) return;
        if (!m_preSearchUrl.isValid()) {
            m_preSearchUrl = currentRoot;
        }
        QUrl searchUrl(QStringLiteral("filenamesearch:/?search=%1&url=%2&checkContent=no")
            .arg(QString::fromUtf8(QUrl::toPercentEncoding(text)),
                 QString::fromUtf8(QUrl::toPercentEncoding(m_preSearchUrl.toString()))));
        searchBox->setStyleSheet("QLineEdit { border: 1px solid #6db3ff; border-radius: 4px; }");
        emit statusChanged(0, 0, 0, QString("Searching for \"%1\"").arg(text));
        setRoot(searchUrl);
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
                    showEmptySpaceContextMenu(v->viewport()->mapToGlobal(pos));
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
    // Check readability for local directories before navigating
    if (url.isLocalFile()) {
        QFileInfo fi(url.toLocalFile());
        if (fi.exists() && !fi.isReadable()) {
            emit statusChanged(0, 0, 0, QString("Permission denied: %1").arg(url.toLocalFile()));
            return;
        }
    }

    // When navigating away from a search URL via non-search means, clear search state
    if (url.scheme() != QLatin1String("filenamesearch") && m_preSearchUrl.isValid()) {
        m_preSearchUrl = QUrl();
        if (searchBox && !searchBox->text().isEmpty()) {
            const QSignalBlocker blocker(searchBox);
            searchBox->clear();
        }
    }

    m_navigationState.navigateTo(url);
    applyLocation(url);
}

void Pane::applyLocation(const QUrl &url) {
    currentRoot = url;
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }

    // Miller view only supports local filesystem paths (QFileSystemModel).
    // For non-local URLs (trash:/, filenamesearch://, etc.), auto-switch to
    // Details view which uses KDirModel and handles all KIO protocols.
    if (!url.isLocalFile() && stack->currentWidget() == miller) {
        viewBox->setCurrentIndex(Details);
        setViewMode(Details);
    } else if (miller) {
        miller->setRootUrl(url);
    }

    syncNavigatorLocation(url);
    emit urlChanged(url);
}

void Pane::setUrl(const QUrl &url) { setRoot(url); }

void Pane::syncNavigatorLocation(const QUrl &url) {
    if (!nav || nav->locationUrl() == url) return;
    const QSignalBlocker blocker(nav);
    nav->setLocationUrl(url);
}

void Pane::setViewMode(int idx) {
    if (!stack) return;
    if (idx < Icons || idx > Miller) return;

    bool leavingMiller = (stack->currentWidget() == miller && idx != Miller);

    // --- Save selection from the outgoing view ---
    m_savedSelectionUrls.clear();
    m_savedCurrentUrl = QUrl();

    if (stack->currentWidget() == miller) {
        m_savedSelectionUrls = miller->getSelectedUrls();
        // The current (focused) item is typically the first selected item in Miller
        if (!m_savedSelectionUrls.isEmpty())
            m_savedCurrentUrl = m_savedSelectionUrls.first();
    } else {
        auto *oldView = qobject_cast<QAbstractItemView*>(stack->currentWidget());
        if (oldView && oldView->selectionModel()) {
            const auto selection = oldView->selectionModel()->selectedIndexes();
            for (const auto &si : selection) {
                if (si.column() == 0) {
                    QUrl u = urlForIndex(si);
                    if (u.isValid()) m_savedSelectionUrls.append(u);
                }
            }
            QUrl cur = urlForIndex(oldView->currentIndex());
            if (cur.isValid())
                m_savedCurrentUrl = cur;
        }
    }

    // When switching FROM Miller to a classic view, reload the dir model
    // at the current location so the classic view shows the right folder.
    if (leavingMiller) {
        if (auto *l = dirModel->dirLister()) {
            l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
        }
    }

    stack->setCurrentIndex(idx);

    // --- Restore selection in the new view ---
    if (idx != Miller) {
        // Lambda to restore selection once the model has loaded
        auto restoreSelection = [this, idx]() {
            auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
            if (!v) return;

            QModelIndex rootIdx = v->rootIndex();
            if (!rootIdx.isValid()) {
                QModelIndex pr = proxy->mapFromSource(dirModel->indexForUrl(currentRoot));
                rootIdx = pr;
                v->setRootIndex(rootIdx);
            }

            if (!m_savedSelectionUrls.isEmpty()) {
                auto *sm = v->selectionModel();
                if (sm) sm->clearSelection();

                QModelIndex currentIdx;
                for (const QUrl &url : std::as_const(m_savedSelectionUrls)) {
                    QModelIndex srcIdx = dirModel->indexForUrl(url);
                    if (!srcIdx.isValid()) continue;
                    QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
                    if (!proxyIdx.isValid()) continue;
                    if (sm)
                        sm->select(proxyIdx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    if (url == m_savedCurrentUrl)
                        currentIdx = proxyIdx;
                }

                if (currentIdx.isValid()) {
                    v->setCurrentIndex(currentIdx);
                    v->scrollTo(currentIdx);
                    onCurrentChanged(currentIdx, QModelIndex());
                } else if (sm && sm->hasSelection()) {
                    // Current URL not found, use first selected
                    QModelIndex first = sm->selectedIndexes().first();
                    v->setCurrentIndex(first);
                    v->scrollTo(first);
                    onCurrentChanged(first, QModelIndex());
                }
            } else {
                // No saved selection — fall back to selecting the first item
                if (!v->currentIndex().isValid() && proxy->rowCount(rootIdx) > 0) {
                    QModelIndex first = proxy->index(0, 0, rootIdx);
                    v->setCurrentIndex(first);
                    if (auto *sm = v->selectionModel())
                        sm->select(first, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    v->scrollTo(first);
                    onCurrentChanged(first, QModelIndex());
                }
            }

            m_savedSelectionUrls.clear();
            m_savedCurrentUrl = QUrl();
        };

        if (leavingMiller) {
            // Model loads asynchronously after openUrl — restore once it finishes
            auto *lister = dirModel->dirLister();
            if (lister) {
                auto conn = std::make_shared<QMetaObject::Connection>();
                *conn = connect(lister, QOverload<>::of(&KDirLister::completed), this,
                    [this, restoreSelection, conn]() {
                        disconnect(*conn);
                        restoreSelection();
                    });
            }
        } else {
            // Model is already loaded for classic→classic switches
            restoreSelection();
        }
    } else {
        // Switching TO Miller — Miller handles its own navigation via setRootUrl.
        // currentRoot is already synced, so nothing extra needed.
        m_savedSelectionUrls.clear();
        m_savedCurrentUrl = QUrl();
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
void Pane::onNavigatorUrlChanged(const QUrl &url) {
    setRoot(url);
    // Breadcrumb clicks should hand focus back to the active file view so arrows work.
    QTimer::singleShot(0, this, [this]() { focusView(); });
}

void Pane::applyIconSize(int px) {
    if (iconView) iconView->setIconSize(QSize(px,px));
    if (m_previewGenerator) m_previewGenerator->updateIcons();
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
        if (shouldNavigateIntoDirectory(url)) {
            setRoot(url);
        } else {
            emit statusChanged(0, 0, 0, "Link folder not opened. Enable Follow symbolic links in Settings to navigate into it.");
        }
    } else {
        FileOpsService::openUrl(url, this);
    }
    // Update preview to match activation as well (helps when toggled on)
    if (m_previewVisible) updatePreviewForUrl(url);
}

void Pane::onCurrentChanged(const QModelIndex &current, const QModelIndex &) {
    const QUrl url = urlForIndex(current);
    if (!url.isValid()) return;
    if (m_previewVisible) updatePreviewForUrl(url);
    // Update Quick Look if it's open (mirrors Miller's selectionChanged handler)
    if (ql && ql->isVisible() && url.isLocalFile()) {
        ql->showFile(url.toLocalFile());
    }
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

void Pane::setShowThumbnails(bool show) {
    if (m_previewGenerator) {
        m_previewGenerator->setPreviewShown(show);
    }
}

void Pane::setShowFileExtensions(bool show) {
    m_showFileExtensions = show;
}

void Pane::setMillerColumnWidth(int width) {
    if (miller) {
        miller->setColumnWidth(width);
    }
}

void Pane::setFollowSymlinks(bool follow) {
    m_followSymlinks = follow;
    if (miller) {
        miller->setFollowSymlinks(follow);
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

        const QString displayName = fi.fileName().isEmpty() ? path : fi.fileName();
        const int itemCount = QDir(path).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).size();
        previewText->setPlainText(QString("%1\n%2 items")
                                  .arg(displayName)
                                  .arg(itemCount));
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

    QAction *actReveal = menu.addAction("Show in Parent Folder");
    actReveal->setEnabled(!multiSelect && firstUrl.isLocalFile());

    QAction *actQL = menu.addAction("Quick Look");
    actQL->setShortcut(Qt::Key_Space);

    menu.addSeparator();

    // --- Edit actions ---
    QAction *actCut = menu.addAction(multiSelect ? QString("Cut %1 Items").arg(count) : "Cut");
    actCut->setShortcut(QKeySequence::Cut);

    QAction *actCopy = menu.addAction(multiSelect ? QString("Copy %1 Items").arg(count) : "Copy");
    actCopy->setShortcut(QKeySequence::Copy);

    QAction *actCopyPath = menu.addAction("Copy Path");
    QAction *actCopyName = menu.addAction("Copy Name");

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

    QAction *actCreateSymlink = menu.addAction("Create Symlink...");
    actCreateSymlink->setEnabled(!multiSelect);

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
    QAction *actExtractHere = nullptr;
    QAction *actExtractToFolder = nullptr;
    QAction *actExtractTo = nullptr;

    // Show Extract only for single archive selection
    if (!multiSelect && isArchiveFile(firstUrl.toLocalFile())) {
        actExtractHere = menu.addAction("Extract Here");
        actExtractToFolder = menu.addAction("Extract to New Folder");
        actExtractTo = menu.addAction("Extract To...");
    } else {
        // Show Compress for any selection (including multiple files)
        actCompress = menu.addAction(multiSelect ? QString("Compress %1 Items...").arg(count) : "Compress...");
    }

    menu.addSeparator();

    // --- Folder operations ---
    const bool inTrash = currentRoot.scheme() == QLatin1String("trash");
    QAction *actNewFolder = menu.addAction("New Folder");
    actNewFolder->setShortcut(QKeySequence("Ctrl+Shift+N"));
    actNewFolder->setEnabled(!inTrash);

    QAction *actOpenTerminal = nullptr;
    if (!inTrash) {
        if (!multiSelect && firstFi.isDir()) {
            actOpenTerminal = menu.addAction(QString("Open Terminal in \"%1\"").arg(firstFi.fileName()));
        } else {
            actOpenTerminal = menu.addAction("Open Terminal Here");
        }
    }

    menu.addSeparator();

    // --- Broken symlink indicator ---
    if (!multiSelect && QFileInfo(firstUrl.toLocalFile()).isSymLink()) {
        QString symlinkTarget = QFileInfo(firstUrl.toLocalFile()).symLinkTarget();
        if (!QFileInfo::exists(symlinkTarget)) {
            QAction *brokenLabel = menu.addAction("(Broken Link)");
            brokenLabel->setEnabled(false);
        }
    }

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
    if (chosen == actReveal) {
        const QFileInfo fi(firstUrl.toLocalFile());
        const QUrl parentUrl = QUrl::fromLocalFile(fi.absolutePath());
        const QString selectedPath = fi.absoluteFilePath();
        setRoot(parentUrl);
        QTimer::singleShot(0, this, [this, selectedPath]() {
            selectFileInView(selectedPath);
        });
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
    if (chosen == actCopyPath) {
        QStringList paths;
        for (const QUrl &url : selectedUrls)
            paths.append(url.toLocalFile());
        QGuiApplication::clipboard()->setText(paths.join(QLatin1Char('\n')));
        return;
    }
    if (chosen == actCopyName) {
        QStringList names;
        for (const QUrl &url : selectedUrls)
            names.append(QFileInfo(url.toLocalFile()).fileName());
        QGuiApplication::clipboard()->setText(names.join(QLatin1Char('\n')));
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
    if (chosen == actCreateSymlink) {
        const QString sourcePath = firstUrl.toLocalFile();
        const QFileInfo fi(sourcePath);
        const QString baseName = fi.completeBaseName();
        const QString suffix = fi.suffix();
        QString linkName = baseName + " - link";
        if (!suffix.isEmpty())
            linkName += QLatin1Char('.') + suffix;
        const QString linkPath = fi.absolutePath() + QLatin1Char('/') + linkName;
        if (!QFile::link(sourcePath, linkPath)) {
            QMessageBox::warning(this, "Create Symlink",
                QString("Failed to create symlink:\n%1").arg(linkPath));
        }
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
    if (chosen == actExtractHere) {
        extractArchiveHere(firstUrl);
        return;
    }
    if (chosen == actExtractToFolder) {
        extractArchiveToNewFolder(firstUrl);
        return;
    }
    if (chosen == actExtractTo) {
        extractArchive(firstUrl);
        return;
    }
    if (chosen == actNewFolder) {
        createNewFolder();
        return;
    }
    if (chosen == actOpenTerminal) {
        openTerminalAt(firstFi.isDir() && !multiSelect ? firstUrl : QUrl::fromLocalFile(firstFi.absolutePath()));
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
    QPoint globalPos = pos;

    // Determine paste destination - use targetFolder if provided, otherwise currentRoot
    QUrl pasteDestination = targetFolder.isValid() ? targetFolder : currentRoot;
    const bool inTrash = currentRoot.scheme() == QLatin1String("trash");

    QMenu menu(this);

    if (inTrash) {
        // Trash-specific context menu
        QAction *emptyTrashAction = menu.addAction("Empty Trash");
        connect(emptyTrashAction, &QAction::triggered, this, [this]() {
            auto result = QMessageBox::question(this, "Empty Trash",
                "Permanently delete all items in the Trash?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (result == QMessageBox::Yes) {
                KIO::emptyTrash();
            }
        });
    } else {
        // Add basic folder operations
        QAction *newFolderAction = menu.addAction("New Folder");
        newFolderAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
        connect(newFolderAction, &QAction::triggered, this, [this, pasteDestination]() {
            createNewFolderIn(pasteDestination);
        });

        QAction *newFileAction = menu.addAction("New File...");
        connect(newFileAction, &QAction::triggered, this, [this, pasteDestination]() {
            createNewFileIn(pasteDestination);
        });

        QAction *openTerminalAction = menu.addAction("Open Terminal Here");
        connect(openTerminalAction, &QAction::triggered, this, [this, pasteDestination]() {
            openTerminalAt(pasteDestination);
        });
    }

    menu.addSeparator();

    // Add paste if clipboard has URLs (not in trash)
    const QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    bool canPaste = !inTrash && mimeData && mimeData->hasUrls();
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
    viewMenu->addAction("Icons", [this]() { setViewMode(Icons); });
    viewMenu->addAction("Details", [this]() { setViewMode(Details); });
    viewMenu->addAction("Compact", [this]() { setViewMode(Compact); });
    viewMenu->addAction("Miller Columns", [this]() { setViewMode(Miller); });

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

    FileOpsService::setClipboardUrls(urls, false);
}

void Pane::cutSelected() {
    QList<QUrl> urls = getSelectedUrls();
    if (urls.isEmpty()) return;

    FileOpsService::setClipboardUrls(urls, true);
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

    const bool isCut = FileOpsService::isClipboardCutOperation(mimeData);

    if (isCut) {
        const QList<QUrl> sourceParents = parentDirectoriesForUrls(urls);
        KIO::CopyJob *job = FileOpsService::move(urls, destination, this);
        connect(job, &KJob::result, this, [this, destination, sourceParents, urls](KJob *finishedJob) {
            refreshLocation(destination);
            for (const QUrl &sourceParent : sourceParents) {
                refreshLocation(sourceParent);
            }
            if (finishedJob->error()) {
                emit statusChanged(0, 0, 0, finishedJob->errorString());
            } else {
                emit statusChanged(0, 0, 0, QString("Moved %1 item(s).").arg(urls.size()));
            }
        });
        connect(job, &KJob::result, this, [](KJob *finishedJob) {
            if (!finishedJob->error()) {
                QGuiApplication::clipboard()->clear();
            }
        });
    } else {
        KIO::CopyJob *job = FileOpsService::copy(urls, destination, this);
        connect(job, &KJob::result, this, [this, destination, urls](KJob *finishedJob) {
            refreshLocation(destination);
            if (finishedJob->error()) {
                emit statusChanged(0, 0, 0, finishedJob->errorString());
            } else {
                emit statusChanged(0, 0, 0, QString("Copied %1 item(s).").arg(urls.size()));
            }
        });
    }
}

bool Pane::isClipboardCutOperation() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    return FileOpsService::isClipboardCutOperation(mimeData);
}

void Pane::openTerminalAt(const QUrl &targetFolder) {
    if (!targetFolder.isValid() || !targetFolder.isLocalFile()) {
        return;
    }

    const QString workingDirectory = targetFolder.toLocalFile();
    if (workingDirectory.isEmpty()) {
        return;
    }

    QSettings settings;
    QString terminalCommand = settings.value("advanced/defaultTerminal", "konsole").toString().trimmed();
    if (terminalCommand.isEmpty()) {
        terminalCommand = QStringLiteral("konsole");
    }

    QStringList parts = QProcess::splitCommand(terminalCommand);
    if (parts.isEmpty()) {
        QMessageBox::warning(this, "Terminal Error", QString("Invalid terminal command: %1").arg(terminalCommand));
        return;
    }

    QString program = parts.takeFirst();
    bool injectedPath = false;
    for (QString &arg : parts) {
        if (arg == "%d" || arg == "%D" || arg == "%p" || arg == "%P") {
            arg = workingDirectory;
            injectedPath = true;
        }
    }

    const bool started = injectedPath
        ? QProcess::startDetached(program, parts)
        : QProcess::startDetached(program, parts, workingDirectory);

    if (!started) {
        QMessageBox::warning(
            this,
            "Terminal Error",
            QString("Failed to launch terminal command \"%1\" in \"%2\".")
                .arg(terminalCommand, workingDirectory));
    }
}

void Pane::deleteSelected() {
    const auto urls = selectedUrls();
    if (urls.isEmpty()) return;

    QSettings settings;
    const bool moveToTrashByDefault = settings.value("advanced/moveToTrash", true).toBool();
    const bool confirmDelete = settings.value("advanced/confirmDelete", true).toBool();

    if (moveToTrashByDefault) {
        if (confirmDelete && !confirmDeleteAction(this, urls, false)) return;
        refreshPaneAfterJob(
            this,
            FileOpsService::trash(urls, this),
            QString("Moved %1 item(s) to Trash.").arg(urls.size()));
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

    refreshPaneAfterJob(
        this,
        FileOpsService::del(urls, this),
        QString("Deleted %1 item(s).").arg(urls.size()));
}

void Pane::moveToTrash() {
    const auto urls = selectedUrls();
    if (urls.isEmpty()) return;

    QSettings settings;
    const bool confirmDelete = settings.value("advanced/confirmDelete", true).toBool();
    if (confirmDelete && !confirmDeleteAction(this, urls, false)) return;

    // Use KIO::trash for safe deletion (moves to trash)
    refreshPaneAfterJob(
        this,
        FileOpsService::trash(urls, this),
        QString("Moved %1 item(s) to Trash.").arg(urls.size()));
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

    m_isEditing = true;

    // Connect to delegate's closeEditor to restore no-edit state when done
    QAbstractItemDelegate *delegate = view->itemDelegate();
    if (delegate) {
        if (m_closeEditorConnection) {
            disconnect(m_closeEditorConnection);
        }
        m_closeEditorConnection = connect(delegate, &QAbstractItemDelegate::closeEditor, this, [this, view]() {
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
            m_isEditing = false;
            disconnect(m_closeEditorConnection);
            m_closeEditorConnection = {};
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
    
    QUrl newFolderUrl = QUrl::fromLocalFile(currentDir.filePath(folderName));
    auto *job = FileOpsService::mkdir(newFolderUrl, this);
    if (!job) return;

    // Select the new folder when it appears in the model (instead of a fixed timer).
    if (destinationUrl == currentRoot) {
        auto *conn = new QMetaObject::Connection;
        *conn = connect(proxy, &QAbstractItemModel::rowsInserted, this,
            [this, folderName, conn](const QModelIndex &parent, int first, int last) {
                for (int i = first; i <= last; ++i) {
                    QModelIndex idx = proxy->index(i, 0, parent);
                    if (idx.data(Qt::DisplayRole).toString() == folderName) {
                        if (auto *view = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
                            view->setCurrentIndex(idx);
                            view->scrollTo(idx);
                        }
                        disconnect(*conn);
                        delete conn;
                        return;
                    }
                }
            });
    }
    refreshPaneAfterJob(this, job, QString("Created folder \"%1\".").arg(folderName));
}

void Pane::createNewFileIn(const QUrl &targetFolder) {
    QUrl destinationUrl = targetFolder;
    if (!destinationUrl.isValid() || !destinationUrl.isLocalFile()) {
        destinationUrl = currentRoot;
    }
    if (!destinationUrl.isValid() || !destinationUrl.isLocalFile()) {
        return;
    }

    bool ok = false;
    QString fileName = QInputDialog::getText(this, "New File", "File name:", QLineEdit::Normal, QString(), &ok);
    if (!ok || fileName.trimmed().isEmpty()) return;

    QString filePath = QDir(destinationUrl.toLocalFile()).filePath(fileName.trimmed());
    QFile file(filePath);
    if (file.exists()) {
        QMessageBox::warning(this, "New File", QString("A file named \"%1\" already exists.").arg(fileName));
        return;
    }
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "New File", QString("Failed to create file:\n%1").arg(filePath));
        return;
    }
    file.close();

    // Refresh directory listing
    if (dirModel) {
        if (auto *lister = dirModel->dirLister()) {
            lister->updateDirectory(destinationUrl);
        }
    }
    if (proxy) {
        proxy->invalidate();
    }
    updateStatus();
}

void Pane::goBack() {
    if (!canGoBack()) return;
    applyLocation(m_navigationState.goBack());
}

void Pane::goForward() {
    if (!canGoForward()) return;
    applyLocation(m_navigationState.goForward());
}

bool Pane::canGoBack() const {
    return m_navigationState.canGoBack();
}

bool Pane::canGoForward() const {
    return m_navigationState.canGoForward();
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
    QString extraInfo;

    auto checkBrokenSymlink = [&extraInfo](const QList<QUrl> &urls) {
        if (urls.size() == 1 && urls.first().isLocalFile()) {
            QFileInfo fi(urls.first().toLocalFile());
            if (fi.isSymLink() && !QFileInfo::exists(fi.symLinkTarget())) {
                extraInfo = QStringLiteral(" (broken link)");
            }
        }
    };

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
        checkBrokenSymlink(selected);
        emit statusChanged(totalFiles, selectedFiles, selectedSize, extraInfo);
        return;
    }

    QList<QUrl> selectedUrlsList;
    auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (v && v->selectionModel()) {
        const auto selectedRows = v->selectionModel()->selectedRows();
        selectedFiles = selectedRows.count();

        // Calculate total size of selected files
        for (const auto &idx : selectedRows) {
            QUrl url = urlForIndex(idx);
            selectedUrlsList.append(url);
            if (url.isLocalFile()) {
                QFileInfo fi(url.toLocalFile());
                if (fi.isFile()) {
                    selectedSize += fi.size();
                }
            }
        }
    }

    checkBrokenSymlink(selectedUrlsList);
    emit statusChanged(totalFiles, selectedFiles, selectedSize, extraInfo);
}

void Pane::updateEmptyFolderOverlay() {
    if (!m_emptyFolderLabel || !proxy) return;
    bool empty = (proxy->rowCount() == 0);
    m_emptyFolderLabel->setVisible(empty);
    if (empty) {
        // Position the label centered within the stack widget
        m_emptyFolderLabel->setGeometry(stack->rect());
        m_emptyFolderLabel->raise();
    }
}

bool Pane::shouldNavigateIntoDirectory(const QUrl &url) const {
    if (!url.isLocalFile()) {
        return true;
    }

    const QFileInfo fi(url.toLocalFile());
    return !(fi.isSymLink() && !m_followSymlinks);
}

void Pane::openSelected() {
    if (stack->currentWidget() == miller) {
        const QList<QUrl> urls = miller->getSelectedUrls();
        if (urls.isEmpty()) return;

        const QUrl url = urls.first();
        if (!url.isValid()) return;

        QFileInfo fi(url.toLocalFile());
        if (fi.isDir()) {
            if (shouldNavigateIntoDirectory(url)) {
                setRoot(url);
            } else {
                emit statusChanged(0, 0, 0, "Link folder not opened. Enable Follow symbolic links in Settings to navigate into it.");
            }
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

void Pane::refreshCurrentLocation() {
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }
}

void Pane::refreshLocation(const QUrl &url) {
    if (!url.isValid() || url == currentRoot) {
        refreshCurrentLocation();
        return;
    }

    if (auto *l = dirModel->dirLister()) {
        l->updateDirectory(url);
    }
    if (proxy) {
        proxy->invalidate();
    }
    updateStatus();
}

QString Pane::adjacentFilePath(const QString &currentPath, int offset) const {
    if (currentPath.isEmpty()) return {};

    // Miller view: navigate within the active column's QFileSystemModel
    if (stack->currentWidget() == miller) {
        const auto columns = miller->findChildren<QListView*>();
        // Find the column whose selection contains the current file
        for (auto *col : columns) {
            auto *fsModel = qobject_cast<QFileSystemModel*>(col->model());
            if (!fsModel) continue;
            QModelIndex current = col->currentIndex();
            if (!current.isValid()) continue;
            if (fsModel->filePath(current) == currentPath) {
                int targetRow = current.row() + offset;
                QModelIndex root = col->rootIndex();
                if (targetRow < 0 || targetRow >= fsModel->rowCount(root)) return {};
                QModelIndex target = fsModel->index(targetRow, 0, root);
                return fsModel->filePath(target);
            }
        }
        return {};
    }

    // Classic views: navigate within the KDirModel proxy
    if (!proxy) return {};
    const int rows = proxy->rowCount();
    int currentRow = -1;
    for (int i = 0; i < rows; ++i) {
        QModelIndex idx = proxy->index(i, 0);
        QModelIndex src = proxy->mapToSource(idx);
        KFileItem item = dirModel->itemForIndex(src);
        if (item.localPath() == currentPath) {
            currentRow = i;
            break;
        }
    }

    if (currentRow < 0) return {};

    int targetRow = currentRow + offset;
    if (targetRow < 0 || targetRow >= rows) return {};

    QModelIndex targetIdx = proxy->index(targetRow, 0);
    QModelIndex targetSrc = proxy->mapToSource(targetIdx);
    KFileItem targetItem = dirModel->itemForIndex(targetSrc);
    return targetItem.localPath();
}

bool Pane::isMillerViewActive() const {
    return stack && stack->currentWidget() == miller;
}



void Pane::selectFileInView(const QString &filePath) {
    if (filePath.isEmpty()) return;
    QUrl fileUrl = QUrl::fromLocalFile(filePath);

    if (stack->currentWidget() == miller) {
        // Miller: find the column whose root is the file's parent directory
        QFileInfo fi(filePath);
        QString parentPath = fi.absolutePath();
        for (auto *col : miller->findChildren<QListView*>()) {
            auto *fsModel = qobject_cast<QFileSystemModel*>(col->model());
            if (!fsModel || fsModel->rootPath() != parentPath) continue;
            QModelIndex idx = fsModel->index(filePath);
            if (idx.isValid()) {
                col->setCurrentIndex(idx);
                col->scrollTo(idx);
                return;
            }
        }
    } else if (proxy && dirModel) {
        // Classic views: find in the proxy model
        QModelIndex srcIdx = dirModel->indexForUrl(fileUrl);
        if (!srcIdx.isValid()) return;
        QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
        if (!proxyIdx.isValid()) return;
        if (auto *v = qobject_cast<QAbstractItemView*>(stack->currentWidget())) {
            v->setCurrentIndex(proxyIdx);
            v->scrollTo(proxyIdx);
        }
    }
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

        // When a rename editor is open, let keys go to the editor.
        // Only handle Escape (cancel) ourselves.
        if (m_isEditing) {
            if (keyEvent->key() == Qt::Key_Escape) {
                view->closePersistentEditor(view->currentIndex());
                view->setEditTriggers(QAbstractItemView::NoEditTriggers);
                m_isEditing = false;
                return true;
            }
            return false;  // Pass all other keys (Enter, Space, letters) to the editor
        }

        if (keyEvent->key() == Qt::Key_Space) {
            // Handle spacebar for QuickLook in all view modes
            QModelIndex current = view->currentIndex();
            if (current.isValid()) {
                QUrl url = urlForIndex(current);
                if (url.isValid()) {
                    quickLookSelected();
                    return true;
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

    auto *dialog = new QDialog;
    DialogUtils::prepareModalDialog(
        dialog,
        this,
        QString("Open %1 with...").arg(fi.fileName()),
        QSize(520, 420));
    dialog->setStyleSheet(
        DialogUtils::finderDialogStyleSheet() +
        "QListWidget { "
            "background-color: rgba(34, 36, 41, 240); "
            "color: #f5f7fa; "
            "border: 1px solid #6f7785; "
            "border-radius: 6px; "
            "outline: none; "
        "}"
        "QListWidget::item { padding: 6px 8px; }"
        "QListWidget::item:selected { "
            "background-color: rgba(74, 144, 226, 210); "
            "color: #f5f7fa; "
        "}"
    );

    auto *layout = new QVBoxLayout(dialog);

    auto *label = new QLabel(QString("Choose an application to open <b>%1</b>:").arg(fi.fileName()), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto *listWidget = new QListWidget(dialog);
    listWidget->setIconSize(QSize(24, 24));
    layout->addWidget(listWidget);

    const QList<OpenWithApp> apps = OpenWithService::applicationsForFile(filePath);
    for (const OpenWithApp &app : apps) {
        auto *item = new QListWidgetItem(app.icon, app.name);
        item->setData(Qt::UserRole, app.desktopId);
        item->setToolTip(app.comment);
        listWidget->addItem(item);
    }

    // Add custom command option
    auto *customItem = new QListWidgetItem(QIcon::fromTheme("system-run"), "Run custom command...");
    customItem->setData(Qt::UserRole, "custom");
    listWidget->addItem(customItem);

    // Select first item
    if (listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
    }

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, dialog);
    layout->addWidget(buttonBox);

    auto runSelection = [this, dialog, listWidget, filePath, fi, url]() {
        auto *selectedItem = listWidget->currentItem();
        if (!selectedItem) {
            dialog->close();
            return;
        }

        QString appId = selectedItem->data(Qt::UserRole).toString();

        if (appId == "custom") {
            dialog->close();

            QString customCommand;
            if (!DialogUtils::promptTextInput(
                this,
                "Custom Command",
                "Enter command to open file with:",
                "",
                &customCommand)) {
                return;
            }
            customCommand = customCommand.trimmed();
            if (customCommand.isEmpty()) return;

            QStringList parts = QProcess::splitCommand(customCommand);
            if (parts.isEmpty()) {
                QMessageBox::warning(this, "Error", QString("Invalid command: %1").arg(customCommand));
                return;
            }

            QString program = parts.takeFirst();
            bool injectedPath = false;
            for (QString &arg : parts) {
                if (arg == "%f" || arg == "%F") {
                    arg = filePath;
                    injectedPath = true;
                } else if (arg == "%u" || arg == "%U") {
                    arg = url.toString();
                    injectedPath = true;
                }
            }
            if (!injectedPath) {
                parts << filePath;
            }

            if (!QProcess::startDetached(program, parts)) {
                QMessageBox::warning(this, "Error", QString("Failed to open file with %1").arg(customCommand));
            }
            return;
        }

        const QString desktopId = selectedItem->data(Qt::UserRole).toString();
        dialog->close();
        if (!OpenWithService::launch(desktopId, {url}, this)) {
            QMessageBox::warning(this, "Error", QString("Failed to open file with %1").arg(selectedItem->text()));
        }
    };

    connect(buttonBox, &QDialogButtonBox::accepted, dialog, runSelection);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    connect(listWidget, &QListWidget::itemDoubleClicked, dialog, runSelection);

    DialogUtils::presentDialog(dialog, listWidget);
}

void Pane::compressSelected() {
    QList<QUrl> urls = getSelectedUrls();
    if (urls.isEmpty()) return;

    const QString outputDir = currentRoot.toLocalFile();
    if (outputDir.isEmpty()) {
        return;
    }

    QString suggestedBaseName;
    if (urls.size() == 1) {
        QFileInfo fi(urls.first().toLocalFile());
        suggestedBaseName = stripKnownArchiveExtension(fi.fileName());
        if (suggestedBaseName.isEmpty()) {
            suggestedBaseName = fi.completeBaseName();
        }
    } else {
        suggestedBaseName = QString("%1_files").arg(urls.size());
    }

    const QList<ArchiveFormatOption> formatOptions = archiveFormatOptions();
    const QString defaultExtension = ArchiveService::defaultArchiveExtension();
    QString archiveName = suggestedBaseName + defaultExtension;

    QDialog dialog(this);
    dialog.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(tr("Compress Files"));
    dialog.setModal(true);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setFixedSize(460, 170);
    dialog.setStyleSheet(
        DialogUtils::finderDialogStyleSheet() +
        QStringLiteral(
            "QLineEdit, QComboBox { "
                "background-color: rgba(74, 78, 86, 240); "
                "color: #f5f7fa; "
                "border: 1px solid #6f7785; "
                "border-radius: 6px; "
                "padding: 4px 8px; "
            "}"
            "QLineEdit:focus, QComboBox:focus { border-color: #6db3ff; }"
            "QFormLayout QLabel { min-width: 110px; }"));

    auto *layout = new QVBoxLayout(&dialog);
    auto *label = new QLabel(
        tr("Create the archive in %1.\nSupported formats: %2")
            .arg(QFileInfo(outputDir).fileName(), ArchiveService::supportedCreateFormatsHint()),
        &dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto *formLayout = new QFormLayout();
    auto *nameEdit = new QLineEdit(archiveName, &dialog);
    nameEdit->setSelection(0, archiveNameSelectionLength(archiveName));
    auto *formatCombo = new QComboBox(&dialog);
    for (const ArchiveFormatOption &option : formatOptions) {
        formatCombo->addItem(option.label, option.extension);
    }
    const int defaultFormatIndex = qMax(0, formatCombo->findData(defaultExtension));
    formatCombo->setCurrentIndex(defaultFormatIndex);

    formLayout->addRow(tr("Name:"), nameEdit);
    formLayout->addRow(tr("Format:"), formatCombo);
    layout->addLayout(formLayout);

    auto updateArchiveNameForFormat = [nameEdit, formatCombo]() {
        const QString extension = formatCombo->currentData().toString();
        const QString baseName = stripKnownArchiveExtension(nameEdit->text().trimmed());
        nameEdit->setText(baseName + extension);
        nameEdit->setSelection(0, archiveNameSelectionLength(nameEdit->text()));
    };
    connect(formatCombo, &QComboBox::currentIndexChanged, &dialog, updateArchiveNameForFormat);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(nameEdit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);
    layout->addWidget(buttonBox);

    if (QWidget *owner = dialog.parentWidget()) {
        const QRect parentFrame = owner->window()->frameGeometry();
        dialog.move(parentFrame.center() - QPoint(dialog.width() / 2, dialog.height() / 2));
    }

    QTimer::singleShot(0, &dialog, [nameEdit]() {
        nameEdit->setFocus(Qt::OtherFocusReason);
    });

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString selectedExtension = formatCombo->currentData().toString();
    const QString archiveBaseName = stripKnownArchiveExtension(nameEdit->text().trimmed()).trimmed();
    if (archiveBaseName.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Name"), tr("That archive name is not valid."));
        return;
    }

    archiveName = archiveBaseName + selectedExtension;
    archiveName = archiveName.trimmed();
    if (archiveName.isEmpty()) {
        return;
    }

    if (archiveName == QStringLiteral(".") || archiveName == QStringLiteral("..")
        || archiveName.contains(QLatin1Char('/')) || archiveName.contains(QLatin1Char('\\'))) {
        QMessageBox::warning(this, tr("Invalid Name"), tr("That archive name is not valid."));
        return;
    }

    const QString archivePath = QDir(outputDir).filePath(archiveName);
    if (!ArchiveService::canCreateArchiveAtPath(archivePath)) {
        QMessageBox::warning(
            this,
            tr("Unsupported Format"),
            tr("Use one of these archive formats: %1.").arg(ArchiveService::supportedCreateFormatsHint()));
        return;
    }

    if (QFile::exists(archivePath)) {
        int ret = QMessageBox::question(this, "File Exists",
                                      QString("Archive '%1' already exists. Overwrite?").arg(archiveName),
                                      QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }

    QProgressDialog *progress = createBusyProgressDialog(
        this,
        tr("Compressing"),
        tr("Creating \"%1\"...").arg(archiveName));
    ArchiveService *service = ArchiveService::createArchive(urls, archivePath, this);
    connect(service, &ArchiveService::finished, this, [this, progress, archiveName](bool success, const QString &errorMessage) {
        progress->close();
        progress->deleteLater();

        if (!success) {
            QMessageBox::warning(
                this,
                tr("Compression Failed"),
                errorMessage.isEmpty()
                    ? tr("Failed to create archive \"%1\".").arg(archiveName)
                    : errorMessage);
            return;
        }

        refreshCurrentLocation();
    });
}

void Pane::extractArchive(const QUrl &archiveUrl) {
    if (!archiveUrl.isValid() || !archiveUrl.isLocalFile()) return;

    const QString archivePath = archiveUrl.toLocalFile();
    const QFileInfo fi(archivePath);
    const QString outputDir = fi.absolutePath();

    QString extractDir = QFileDialog::getExistingDirectory(this, "Extract to Directory",
                                                           outputDir,
                                                           QFileDialog::ShowDirsOnly);
    if (extractDir.isEmpty()) return;

    runArchiveExtraction(archiveUrl, extractDir);
}

void Pane::extractArchiveHere(const QUrl &archiveUrl) {
    if (!archiveUrl.isValid() || !archiveUrl.isLocalFile()) {
        return;
    }

    const QFileInfo fi(archiveUrl.toLocalFile());
    runArchiveExtraction(archiveUrl, fi.absolutePath());
}

void Pane::extractArchiveToNewFolder(const QUrl &archiveUrl) {
    if (!archiveUrl.isValid() || !archiveUrl.isLocalFile()) {
        return;
    }

    const QFileInfo fi(archiveUrl.toLocalFile());
    const QString baseFolderName = stripKnownArchiveExtension(fi.fileName());
    const QString extractDir = uniqueChildPath(fi.absolutePath(), baseFolderName);
    runArchiveExtraction(archiveUrl, extractDir);
}

void Pane::runArchiveExtraction(const QUrl &archiveUrl, const QString &extractDir, const QUrl &selectUrlOnSuccess) {
    if (!archiveUrl.isValid() || !archiveUrl.isLocalFile()) {
        return;
    }

    const QString archivePath = archiveUrl.toLocalFile();
    const QFileInfo fi(archivePath);

    if (!ArchiveService::canExtractArchive(archivePath)) {
        QMessageBox::warning(this, "Unsupported Format",
                           tr("That archive format is not supported here yet."));
        return;
    }

    ArchiveService::ExtractConflictPolicy conflictPolicy = ArchiveService::ExtractConflictPolicy::KeepExisting;
    if (ArchiveService::canPreviewExtractionConflicts(archivePath)) {
        QString conflictError;
        const QStringList conflicts = ArchiveService::listExtractionConflicts(archiveUrl, extractDir, &conflictError);
        if (!conflictError.isEmpty()) {
            QMessageBox::warning(
                this,
                tr("Extraction Failed"),
                conflictError);
            return;
        }

        if (!conflicts.isEmpty()) {
            QMessageBox conflictDialog(this);
            conflictDialog.setIcon(QMessageBox::Warning);
            conflictDialog.setWindowTitle(tr("Files Already Exist"));
            conflictDialog.setText(
                tr("%1 item(s) from \"%2\" already exist in \"%3\".")
                    .arg(conflicts.size())
                    .arg(fi.fileName(), QDir::toNativeSeparators(extractDir)));
            conflictDialog.setInformativeText(
                tr("Choose whether to replace the existing items or extract into a new folder instead."));
            conflictDialog.setDetailedText(conflicts.join(QLatin1Char('\n')));

            QPushButton *newFolderButton = qobject_cast<QPushButton*>(conflictDialog.addButton(tr("Extract to New Folder"), QMessageBox::AcceptRole));
            QPushButton *replaceButton = qobject_cast<QPushButton*>(conflictDialog.addButton(tr("Replace Existing"), QMessageBox::DestructiveRole));
            QPushButton *cancelButton = qobject_cast<QPushButton*>(conflictDialog.addButton(QMessageBox::Cancel));
            conflictDialog.setDefaultButton(newFolderButton);

            const QString previewText = summarizeConflictPaths(conflicts);
            if (!previewText.isEmpty()) {
                conflictDialog.setInformativeText(
                    tr("Choose whether to replace the existing items or extract into a new folder instead.\n\n%1")
                        .arg(previewText));
            }

            conflictDialog.exec();

            if (conflictDialog.clickedButton() == cancelButton) {
                return;
            }
            if (conflictDialog.clickedButton() == newFolderButton) {
                const QString baseFolderName = stripKnownArchiveExtension(fi.fileName());
                const QString alternateExtractDir = uniqueChildPath(extractDir, baseFolderName);
                runArchiveExtraction(archiveUrl, alternateExtractDir);
                return;
            }
            if (conflictDialog.clickedButton() == replaceButton) {
                conflictPolicy = ArchiveService::ExtractConflictPolicy::ReplaceExisting;
            } else {
                return;
            }
        }
    }

    QProgressDialog *progress = createBusyProgressDialog(
        this,
        tr("Extracting"),
        tr("Extracting \"%1\"...").arg(fi.fileName()));
    ArchiveService *service = ArchiveService::extractArchive(archiveUrl, extractDir, conflictPolicy, this);
    connect(service, &ArchiveService::finished, this, [this, progress, fi, extractDir](bool success, const QString &errorMessage) {
        progress->close();
        progress->deleteLater();

        if (!success) {
            QMessageBox::warning(
                this,
                tr("Extraction Failed"),
                errorMessage.isEmpty()
                    ? tr("Failed to extract archive \"%1\".").arg(fi.fileName())
                    : errorMessage);
            return;
        }

        const QString cleanExtractDir = QDir::cleanPath(extractDir);
        const QString cleanCurrentRoot = QDir::cleanPath(currentRoot.toLocalFile());
        if (cleanExtractDir == cleanCurrentRoot) {
            refreshCurrentLocation();
            return;
        }

        QMessageBox::information(
            this,
            tr("Extraction Complete"),
            tr("Extracted \"%1\" to \"%2\".").arg(fi.fileName(), extractDir));
    });
}

void Pane::duplicateSelected() {
    QList<QUrl> urls = getSelectedUrls();
    if (urls.isEmpty()) return;

    QList<QPair<QUrl, QUrl>> duplicateOps;
    duplicateOps.reserve(urls.size());

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

        duplicateOps.append({QUrl::fromLocalFile(sourcePath), QUrl::fromLocalFile(duplicatePath)});
    }

    if (duplicateOps.isEmpty()) {
        return;
    }

    struct DuplicateState {
        QList<QPair<QUrl, QUrl>> ops;
        int index = 0;
        int failures = 0;
        QString firstError;
    };
    auto state = std::make_shared<DuplicateState>();
    state->ops = duplicateOps;

    auto runNext = std::make_shared<std::function<void()>>();
    *runNext = [this, state, runNext]() {
        if (state->index >= state->ops.size()) {
            refreshCurrentLocation();

            const int successCount = state->ops.size() - state->failures;
            if (state->failures == 0) {
                emit statusChanged(0, 0, 0, QString("Duplicated %1 item(s).").arg(successCount));
            } else {
                QMessageBox::warning(this, "Duplicate Completed With Errors",
                    QString("Duplicated %1 of %2 item(s).\n\nFirst error: %3")
                        .arg(successCount)
                        .arg(state->ops.size())
                        .arg(state->firstError.isEmpty() ? QString("Unknown error") : state->firstError));
            }
            return;
        }

        const auto op = state->ops.at(state->index++);
        KIO::CopyJob *job = FileOpsService::copyAs(op.first, op.second, this);
        connect(job, &KJob::result, this, [state, runNext](KJob *finishedJob) {
            if (finishedJob->error()) {
                state->failures++;
                if (state->firstError.isEmpty()) {
                    state->firstError = finishedJob->errorString();
                }
            }
            (*runNext)();
        });
    };

    (*runNext)();
}

void Pane::setSortCriteria(int criteria) {
    if (!proxy) return;
    
    int proxyColumn = 0;
    int millerColumn = 0;
    switch (criteria) {
        case 0: // Name
            proxyColumn = 0;
            millerColumn = 0;
            break;
        case 1: // Size
            proxyColumn = 1;
            millerColumn = 1;
            break;
        case 2: // Type
            proxyColumn = 6;
            millerColumn = 2;
            break;
        case 3: // Date Modified
            proxyColumn = 2;
            millerColumn = 3;
            break;
        default:
            proxyColumn = 0;
            millerColumn = 0;
            break;
    }
    
    proxy->sort(proxyColumn, proxy->sortOrder());
    if (miller) {
        miller->setSort(millerColumn, proxy->sortOrder());
    }
}

void Pane::setSortOrder(Qt::SortOrder order) {
    if (!proxy) return;
    
    int currentColumn = proxy->sortColumn();
    if (currentColumn < 0) currentColumn = 0; // Default to name
    
    proxy->sort(currentColumn, order);

    int millerColumn = 0;
    switch (currentColumn) {
        case 1: millerColumn = 1; break; // Size
        case 6: millerColumn = 2; break; // Type
        case 2: millerColumn = 3; break; // Date Modified
        case 0:
        default:
            millerColumn = 0; // Name
            break;
    }

    if (miller) {
        miller->setSort(millerColumn, order);
    }
}
