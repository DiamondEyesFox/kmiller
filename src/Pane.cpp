// Local headers
#include "Pane.h"
#include "DialogUtils.h"
#include "MillerView.h"
#include "QuickLookDialog.h"
#include "ThumbCache.h"
#include "PropertiesDialog.h"
#include "FileOpsService.h"
#include "ArchiveService.h"

// Qt Core
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QFutureWatcher>
#include <QMimeData>
#include <QMimeDatabase>
#include <QProcess>
#include <QSignalBlocker>
#include <QSettings>
#include <QTimer>
#include <QStandardPaths>
#include <algorithm>
#include <functional>
#include <memory>

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
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QFrame>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QStyleOptionViewItem>

// KDE Frameworks
#include <KApplicationTrader>
#include <KDirLister>
#include <KDirModel>
#include <KDirSortFilterProxyModel>
#include <KFileItem>
#include <KJob>
#include <KIO/ApplicationLauncherJob>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/MkdirJob>
#include <KIO/SimpleJob>
#include <KService>
#include <KUrlNavigator>

// External libraries
#include <poppler-qt6.h>
#include <QtConcurrent/QtConcurrentRun>

static bool isImageFile(const QString &path) {
    const QByteArray fmt = QImageReader::imageFormat(path);
    return !fmt.isEmpty();
}

struct SearchResultEntry {
    QUrl url;
    QString name;
    QString location;
    QString kind;
    QDateTime modified;
    bool isDirectory = false;
};

static QList<SearchResultEntry> performRecursiveSearch(const QString &rootPath,
                                                       const QString &query,
                                                       bool includeHidden,
                                                       int maxResults = 500) {
    QList<SearchResultEntry> results;
    if (rootPath.isEmpty()) {
        return results;
    }

    const QString needle = query.trimmed();
    if (needle.isEmpty()) {
        return results;
    }

    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (includeHidden) {
        filters |= QDir::Hidden;
    }

    QDirIterator it(rootPath, filters, QDirIterator::Subdirectories);
    while (it.hasNext() && results.size() < maxResults) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        if (!fi.fileName().contains(needle, Qt::CaseInsensitive)) {
            continue;
        }

        SearchResultEntry result;
        result.url = QUrl::fromLocalFile(fi.absoluteFilePath());
        result.name = fi.fileName();
        result.location = fi.absolutePath();
        result.kind = fi.isDir()
            ? QStringLiteral("Folder")
            : (fi.suffix().isEmpty()
                ? QStringLiteral("File")
                : QStringLiteral("%1 File").arg(fi.suffix().toUpper()));
        result.modified = fi.lastModified();
        result.isDirectory = fi.isDir();
        results.append(result);
    }

    std::sort(results.begin(), results.end(), [](const SearchResultEntry &left, const SearchResultEntry &right) {
        if (left.isDirectory != right.isDirectory) {
            return left.isDirectory && !right.isDirectory;
        }
        return QString::localeAwareCompare(left.name, right.name) < 0;
    });

    return results;
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

static bool supportsThumbnailPreview(const QString &path) {
    QFileInfo fi(path);
    return isImageFile(path) || fi.suffix().compare("pdf", Qt::CaseInsensitive) == 0;
}

static bool isDirectorySymlinkUrl(const QUrl &url) {
    if (!url.isLocalFile()) {
        return false;
    }

    const QFileInfo fi(url.toLocalFile());
    return fi.isSymLink() && fi.isDir();
}

static QString displayNameForFileInfo(const QFileInfo &fi, bool showFileExtensions) {
    const QString fileName = fi.fileName();
    if (showFileExtensions || fi.isDir()) {
        return fileName;
    }

    const int lastDot = fileName.lastIndexOf('.');
    if (lastDot <= 0) {
        return fileName;
    }

    return fileName.left(lastDot);
}

class PaneItemDelegate final : public QStyledItemDelegate {
public:
    using UrlResolver = std::function<QUrl(const QModelIndex &)>;
    using IconResolver = std::function<QIcon(const QUrl &)>;
    using BoolResolver = std::function<bool()>;

    PaneItemDelegate(UrlResolver urlResolver,
                     IconResolver iconResolver,
                     BoolResolver showThumbnailsResolver,
                     BoolResolver showFileExtensionsResolver,
                     QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_urlResolver(std::move(urlResolver))
        , m_iconResolver(std::move(iconResolver))
        , m_showThumbnailsResolver(std::move(showThumbnailsResolver))
        , m_showFileExtensionsResolver(std::move(showFileExtensionsResolver)) {}

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);

        if (!option || index.column() != 0) {
            return;
        }

        const QUrl url = m_urlResolver ? m_urlResolver(index) : QUrl();
        if (!url.isLocalFile()) {
            return;
        }

        const QFileInfo fi(url.toLocalFile());
        if (m_showFileExtensionsResolver && !m_showFileExtensionsResolver()) {
            option->text = displayNameForFileInfo(fi, false);
        }

        if (m_showThumbnailsResolver && m_showThumbnailsResolver() && supportsThumbnailPreview(fi.absoluteFilePath())) {
            option->icon = m_iconResolver ? m_iconResolver(url) : option->icon;
        }
    }

private:
    UrlResolver m_urlResolver;
    IconResolver m_iconResolver;
    BoolResolver m_showThumbnailsResolver;
    BoolResolver m_showFileExtensionsResolver;
};

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

static void refreshPaneAfterJob(Pane *pane, KJob *job) {
    if (!pane || !job) {
        return;
    }

    QObject::connect(job, &KJob::result, pane, [pane](KJob *) {
        pane->refreshCurrentLocation();
    });
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
    searchBox->setPlaceholderText("Search this folder...");
    searchBox->setFixedWidth(150);
    tb->addWidget(searchBox);

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
    iconView->setItemDelegate(new PaneItemDelegate(
        [this](const QModelIndex &idx) { return urlForIndex(idx); },
        [this](const QUrl &url) { return getIconForFile(url); },
        [this]() { return m_showThumbnails; },
        [this]() { return m_showFileExtensions; },
        iconView
    ));
    
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
    detailsView->setItemDelegate(new PaneItemDelegate(
        [this](const QModelIndex &idx) { return urlForIndex(idx); },
        [this](const QUrl &url) { return getIconForFile(url); },
        [this]() { return m_showThumbnails; },
        [this]() { return m_showFileExtensions; },
        detailsView
    ));
    
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
    compactView->setItemDelegate(new PaneItemDelegate(
        [this](const QModelIndex &idx) { return urlForIndex(idx); },
        [this](const QUrl &url) { return getIconForFile(url); },
        [this]() { return m_showThumbnails; },
        [this]() { return m_showFileExtensions; },
        compactView
    ));
    stack->addWidget(compactView);

    searchResultsModel = new QStandardItemModel(this);
    searchResultsModel->setHorizontalHeaderLabels({tr("Name"), tr("Location"), tr("Type"), tr("Modified")});

    searchResultsView = new QTreeView(this);
    searchResultsView->setModel(searchResultsModel);
    searchResultsView->setRootIsDecorated(false);
    searchResultsView->setAlternatingRowColors(true);
    searchResultsView->header()->setStretchLastSection(true);
    searchResultsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    searchResultsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    searchResultsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    searchResultsView->setContextMenuPolicy(Qt::CustomContextMenu);
    stack->addWidget(searchResultsView);

    miller = new MillerView(this);
    m_showThumbnails = settings.value("view/showThumbnails", true).toBool();
    m_showFileExtensions = settings.value("view/showFileExtensions", true).toBool();
    m_millerColumnWidth = settings.value("view/millerColumnWidth", 200).toInt();
    m_followSymlinks = settings.value("advanced/followSymlinks", false).toBool();
    miller->setShowThumbnails(m_showThumbnails);
    miller->setShowFileExtensions(m_showFileExtensions);
    miller->setColumnWidth(m_millerColumnWidth);
    miller->setFollowSymlinks(m_followSymlinks);
    stack->addWidget(miller);
    // Ensure Ctrl+A selects all in classic views
    for (QAbstractItemView* v : { static_cast<QAbstractItemView*>(iconView), static_cast<QAbstractItemView*>(detailsView), static_cast<QAbstractItemView*>(compactView), static_cast<QAbstractItemView*>(searchResultsView) }) {
        if (!v) continue;
        auto *sc = new QShortcut(QKeySequence::SelectAll, v);
        connect(sc, &QShortcut::activated, v, [v]{ v->selectAll(); });
    }
    // Miller: multi-item context menu + Quick Look
    connect(miller, &MillerView::quickLookRequested, this, [this](const QString &p){ if (ql && ql->isVisible()) { ql->close(); } else { if (!ql) ql = new QuickLookDialog(this); ql->showFile(p); } });
    connect(miller, &MillerView::contextMenuRequested, this, [this](const QList<QUrl> &urls, const QPoint &g){ showContextMenu(g, urls); });
    connect(miller, &MillerView::emptySpaceContextMenuRequested, this, [this](const QUrl &folderUrl, const QPoint &g){ showEmptySpaceContextMenu(g, folderUrl); });
    connect(miller, &MillerView::renameRequested, this, &Pane::renameSelected);
    connect(miller, &MillerView::selectionChanged, this, [this](const QUrl &url){
        if (m_previewVisible) updatePreviewForUrl(url);
        // Update QuickLook if it's open
        if (ql && ql->isVisible() && url.isLocalFile()) {
            ql->showFile(url.toLocalFile());
        }
    });
    connect(miller, &MillerView::navigatedTo, this, [this](const QUrl &url){
        if (!url.isValid()) return;
        syncNavigatorLocation(url);
        emit urlChanged(url);
    });

    ql = new QuickLookDialog(this);
    thumbs = new ThumbCache(this);

    connect(viewBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &Pane::onViewModeChanged);
    connect(zoom, &QSlider::valueChanged, this, &Pane::onZoomChanged);
    connect(nav, &KUrlNavigator::urlChanged, this, &Pane::onNavigatorUrlChanged);
    connect(nav, &KUrlNavigator::editableStateChanged, this, [this](bool editable) {
        // Keep Finder-style behavior: breadcrumb segments stay visible/clickable.
        if (!editable || !nav) return;
        QTimer::singleShot(0, this, [this]() {
            if (nav && nav->isUrlEditable()) {
                nav->setUrlEditable(false);
            }
        });
    });
    
    // Search/filter functionality
    connect(searchBox, &QLineEdit::textChanged, this, [this](const QString &text){
        if (text.trimmed().isEmpty()) {
            clearSearchMode();
        } else {
            beginSearch(text);
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
                v->setCurrentIndex(idx);
                renameSelected();
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
    hookSel(searchResultsView);

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
    m_navigationState.navigateTo(url);
    applyLocation(m_navigationState.current());
}

void Pane::setUrl(const QUrl &url) { setRoot(url); }

void Pane::applyLocation(const QUrl &url, bool emitChangeSignal) {
    currentRoot = url;
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }
    if (miller) miller->setRootUrl(url);
    if (searchBox && !searchBox->text().trimmed().isEmpty()) {
        beginSearch(searchBox->text());
    }
    syncNavigatorLocation(url);
    if (emitChangeSignal) {
        emit urlChanged(url);
    }
}

void Pane::beginSearch(const QString &query) {
    if (!searchResultsModel || !searchResultsView || !stack) {
        return;
    }

    const QString trimmedQuery = query.trimmed();
    if (trimmedQuery.isEmpty()) {
        clearSearchMode();
        return;
    }

    if (!m_searchModeActive && stack->currentWidget() != searchResultsView) {
        m_savedViewModeBeforeSearch = currentViewMode();
    }
    m_searchModeActive = true;
    stack->setCurrentWidget(searchResultsView);

    ++m_searchRequestId;
    const quint64 requestId = m_searchRequestId;

    searchResultsModel->clear();
    searchResultsModel->setHorizontalHeaderLabels({tr("Name"), tr("Location"), tr("Type"), tr("Modified")});

    auto *watcher = new QFutureWatcher<QList<SearchResultEntry>>(this);
    connect(watcher, &QFutureWatcher<QList<SearchResultEntry>>::finished, this, [this, watcher, requestId]() {
        const QList<SearchResultEntry> results = watcher->result();
        watcher->deleteLater();

        if (requestId != m_searchRequestId || !searchResultsModel || !searchResultsView) {
            return;
        }

        searchResultsModel->clear();
        searchResultsModel->setHorizontalHeaderLabels({tr("Name"), tr("Location"), tr("Type"), tr("Modified")});

        QFileIconProvider iconProvider;
        for (const SearchResultEntry &result : results) {
            auto *nameItem = new QStandardItem(result.name);
            nameItem->setData(result.url, Qt::UserRole);

            QIcon icon = result.isDirectory ? QIcon::fromTheme("folder") : getIconForFile(result.url);
            if (icon.isNull() && result.url.isLocalFile()) {
                icon = iconProvider.icon(QFileInfo(result.url.toLocalFile()));
            }
            if (!icon.isNull()) {
                nameItem->setIcon(icon);
            }

            auto *locationItem = new QStandardItem(result.location);
            auto *typeItem = new QStandardItem(result.kind);
            auto *modifiedItem = new QStandardItem(
                result.modified.isValid()
                    ? result.modified.toString(QStringLiteral("yyyy-MM-dd hh:mm"))
                    : QString());

            searchResultsModel->appendRow({nameItem, locationItem, typeItem, modifiedItem});
        }

        if (searchResultsModel->rowCount() > 0) {
            const QModelIndex first = searchResultsModel->index(0, 0);
            searchResultsView->setCurrentIndex(first);
            searchResultsView->scrollTo(first);
        }

        updateStatus();
    });

    const QString rootPath = currentRoot.toLocalFile();
    const bool includeHidden = m_showHiddenFiles;
    watcher->setFuture(QtConcurrent::run([rootPath, trimmedQuery, includeHidden]() {
        return performRecursiveSearch(rootPath, trimmedQuery, includeHidden);
    }));
}

void Pane::clearSearchMode() {
    ++m_searchRequestId;

    if (searchResultsModel) {
        searchResultsModel->clear();
        searchResultsModel->setHorizontalHeaderLabels({tr("Name"), tr("Location"), tr("Type"), tr("Modified")});
    }

    const bool wasSearchView = m_searchModeActive && stack && stack->currentWidget() == searchResultsView;
    m_searchModeActive = false;

    if (wasSearchView) {
        setViewMode(m_savedViewModeBeforeSearch);
    }

    updateStatus();
}

void Pane::syncNavigatorLocation(const QUrl &url) {
    if (!nav || nav->locationUrl() == url) return;
    const QSignalBlocker blocker(nav);
    nav->setLocationUrl(url);
}

void Pane::setViewMode(int idx) {
    if (!stack) return;
    if (idx < 0 || idx > 3) return;

    if (m_searchModeActive) {
        m_savedViewModeBeforeSearch = idx;
        if (stack->currentWidget() == searchResultsView) {
            return;
        }
    }

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
    if (!stack) {
        return 0;
    }
    if (stack->currentWidget() == searchResultsView) {
        return m_savedViewModeBeforeSearch;
    }
    return stack->currentIndex();
}
void Pane::onZoomChanged(int val) { applyIconSize(val); }
void Pane::onNavigatorUrlChanged(const QUrl &url) {
    setRoot(url);
    // Breadcrumb clicks should hand focus back to the active file view so arrows work.
    QTimer::singleShot(0, this, [this]() { focusView(); });
}

void Pane::applyIconSize(int px) { if (iconView) iconView->setIconSize(QSize(px,px)); }

QUrl Pane::urlForIndex(const QModelIndex &proxyIndex) const {
    if (searchResultsView && searchResultsModel && proxyIndex.isValid() && proxyIndex.model() == searchResultsModel) {
        const QModelIndex nameIndex = proxyIndex.sibling(proxyIndex.row(), 0);
        return nameIndex.data(Qt::UserRole).toUrl();
    }

    QModelIndex src = proxy->mapToSource(proxyIndex);
    KFileItem item = dirModel->itemForIndex(src);
    return item.url();
}

QModelIndex Pane::currentSelectionIndexForView(QAbstractItemView *view) const {
    if (!view || !view->selectionModel()) {
        return QModelIndex();
    }

    const QModelIndex current = view->selectionModel()->currentIndex();
    if (current.isValid()) {
        return current;
    }

    const auto selectedRows = view->selectionModel()->selectedRows();
    if (!selectedRows.isEmpty()) {
        return selectedRows.first();
    }

    return QModelIndex();
}

QModelIndex Pane::currentSelectionIndex() const {
    if (stack->currentWidget() == miller) {
        return QModelIndex();
    }

    auto *view = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    return currentSelectionIndexForView(view);
}

QUrl Pane::primarySelectionUrl() const {
    if (stack->currentWidget() == miller) {
        const QList<QUrl> urls = miller->getSelectedUrls();
        if (!urls.isEmpty()) {
            return urls.first();
        }
        return QUrl();
    }

    const QModelIndex index = currentSelectionIndex();
    if (!index.isValid()) {
        return QUrl();
    }

    return urlForIndex(index);
}

void Pane::selectUrlInActiveView(const QUrl &targetUrl) {
    if (!targetUrl.isValid() || stack->currentWidget() == miller) {
        return;
    }

    QTimer::singleShot(100, this, [this, targetUrl]() {
        auto *view = qobject_cast<QAbstractItemView *>(stack->currentWidget());
        if (!view || !proxy) {
            return;
        }

        for (int i = 0; i < proxy->rowCount(); ++i) {
            const QModelIndex index = proxy->index(i, 0);
            if (!index.isValid() || urlForIndex(index) != targetUrl) {
                continue;
            }

            view->setCurrentIndex(index);
            view->scrollTo(index);
            if (QItemSelectionModel *selection = view->selectionModel()) {
                selection->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            break;
        }
    });
}

void Pane::onActivated(const QModelIndex &idx) {
    QUrl url = urlForIndex(idx);
    if (!url.isValid()) return;

    if (stack && stack->currentWidget() == searchResultsView && searchBox && !searchBox->text().isEmpty()) {
        searchBox->clear();
    }

    const QFileInfo fi(url.toLocalFile());
    if (url.isLocalFile() && fi.isDir()) {
        if (!tryNavigateToDirectoryUrl(url, true)) {
            return;
        }
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
    const QUrl url = primarySelectionUrl();
    if (!url.isValid() || !url.isLocalFile()) return;

    if (!ql) ql = new QuickLookDialog(this);
    ql->showFile(url.toLocalFile());
}

void Pane::setPreviewVisible(bool on) {
    m_previewVisible = on;
    if (preview) preview->setVisible(on);
    if (on) {
        // seed with current selection if any
        const QUrl url = primarySelectionUrl();
        if (url.isValid()) {
            updatePreviewForUrl(url);
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

    if (searchBox && !searchBox->text().trimmed().isEmpty()) {
        beginSearch(searchBox->text());
    }
}

void Pane::setShowThumbnails(bool show) {
    m_showThumbnails = show;

    if (miller) {
        miller->setShowThumbnails(show);
    }

    for (QAbstractItemView *view : {static_cast<QAbstractItemView *>(iconView),
                                    static_cast<QAbstractItemView *>(detailsView),
                                    static_cast<QAbstractItemView *>(compactView),
                                    static_cast<QAbstractItemView *>(searchResultsView)}) {
        if (view && view->viewport()) {
            view->viewport()->update();
        }
    }

    if (searchBox && !searchBox->text().trimmed().isEmpty()) {
        beginSearch(searchBox->text());
    }
}

void Pane::setShowFileExtensions(bool show) {
    m_showFileExtensions = show;

    if (miller) {
        miller->setShowFileExtensions(show);
    }

    for (QAbstractItemView *view : {static_cast<QAbstractItemView *>(iconView),
                                    static_cast<QAbstractItemView *>(detailsView),
                                    static_cast<QAbstractItemView *>(compactView),
                                    static_cast<QAbstractItemView *>(searchResultsView)}) {
        if (view && view->viewport()) {
            view->viewport()->update();
        }
    }

    if (m_previewVisible) {
        const QUrl url = primarySelectionUrl();
        if (url.isValid()) {
            updatePreviewForUrl(url);
        }
    }

    if (searchBox && !searchBox->text().trimmed().isEmpty()) {
        beginSearch(searchBox->text());
    }
}

void Pane::setMillerColumnWidth(int width) {
    if (width < 150) width = 150;
    if (width > 400) width = 400;

    m_millerColumnWidth = width;
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

bool Pane::tryNavigateToDirectoryUrl(const QUrl &url, bool showBlockedMessage) {
    if (!url.isValid()) {
        return false;
    }

    if (!m_followSymlinks && isDirectorySymlinkUrl(url)) {
        if (showBlockedMessage) {
            const QFileInfo fi(url.toLocalFile());
            const QString displayName = fi.fileName().isEmpty() ? url.toLocalFile() : fi.fileName();
            QMessageBox::information(
                this,
                tr("Symlink Navigation Disabled"),
                tr("'%1' is a symbolic link to a directory.\n\nEnable 'Follow symbolic links' in Preferences to navigate into it.")
                    .arg(displayName));
        }
        return false;
    }

    setRoot(url);
    return true;
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
        const QString visibleName = fi.fileName().isEmpty()
            ? path
            : displayNameForFileInfo(fi, m_showFileExtensions);
        const int itemCount = QDir(path).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).size();
        previewText->setPlainText(QString("%1\n%2 items")
                                  .arg(visibleName)
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
                                  .arg(displayNameForFileInfo(fi, m_showFileExtensions))
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
            previewText->setPlainText(displayNameForFileInfo(fi, m_showFileExtensions));
            return;
        }
    }

    // Fallback - show file type icon
    QPixmap fileIcon = getFileTypeIcon(fi, 128);
    if (!fileIcon.isNull()) {
        previewImage->setPixmap(fileIcon);
    }
    
    previewText->setPlainText(QString("%1 — %2 KB")
                              .arg(displayNameForFileInfo(fi, m_showFileExtensions))
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
        const QUrl current = primarySelectionUrl();
        if (current.isValid()) {
            selectedUrls.append(current);
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
    QAction *actNewFolder = menu.addAction("New Folder");
    actNewFolder->setShortcut(QKeySequence("Ctrl+Shift+N"));

    QAction *actOpenTerminal = nullptr;
    if (!multiSelect && firstFi.isDir()) {
        actOpenTerminal = menu.addAction(QString("Open Terminal in \"%1\"").arg(firstFi.fileName()));
    } else {
        actOpenTerminal = menu.addAction("Open Terminal Here");
    }

    menu.addSeparator();

    // --- Properties ---
    QAction *actProperties = menu.addAction(multiSelect ? QString("Properties (%1 Items)").arg(count) : "Properties");
    actProperties->setShortcut(QKeySequence("Alt+Return"));

    // Execute menu
    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;

    // Handle actions
    if (chosen == actOpen) {
        if (!multiSelect && firstFi.isDir()) {
            tryNavigateToDirectoryUrl(firstUrl, true);
        } else {
            for (const QUrl &url : selectedUrls) {
                FileOpsService::openUrl(url, this);
            }
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

    QMenu menu(this);

    // Add basic folder operations
    QAction *newFolderAction = menu.addAction("New Folder");
    newFolderAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    connect(newFolderAction, &QAction::triggered, this, [this, pasteDestination]() {
        createNewFolderIn(pasteDestination);
    });

    QAction *openTerminalAction = menu.addAction("Open Terminal Here");
    connect(openTerminalAction, &QAction::triggered, this, [this, pasteDestination]() {
        openTerminalAt(pasteDestination);
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

    const QUrl current = primarySelectionUrl();
    if (current.isValid()) {
        urls.append(current);
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
        KIO::CopyJob *job = FileOpsService::move(urls, destination, this);
        refreshPaneAfterJob(this, job);
        connect(job, &KJob::result, this, [](KJob *finishedJob) {
            if (!finishedJob->error()) {
                QGuiApplication::clipboard()->clear();
            }
        });
    } else {
        refreshPaneAfterJob(this, FileOpsService::copy(urls, destination, this));
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
        refreshPaneAfterJob(this, FileOpsService::trash(urls, this));
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

    refreshPaneAfterJob(this, FileOpsService::del(urls, this));
}

void Pane::moveToTrash() {
    const auto urls = selectedUrls();
    if (urls.isEmpty()) return;

    QSettings settings;
    const bool confirmDelete = settings.value("advanced/confirmDelete", true).toBool();
    if (confirmDelete && !confirmDeleteAction(this, urls, false)) return;

    // Use KIO::trash for safe deletion (moves to trash)
    refreshPaneAfterJob(this, FileOpsService::trash(urls, this));
}

void Pane::renameSelected() {
    const QUrl sourceUrl = primarySelectionUrl();
    if (!sourceUrl.isValid() || !sourceUrl.isLocalFile()) {
        return;
    }

    const QFileInfo sourceInfo(sourceUrl.toLocalFile());
    const QString currentName = sourceInfo.fileName();
    if (currentName.isEmpty()) {
        return;
    }

    int selectionLength = currentName.size();
    if (!sourceInfo.isDir()) {
        const int extensionDot = currentName.lastIndexOf('.');
        if (extensionDot > 0) {
            selectionLength = extensionDot;
        }
    }

    QString newName;
    if (!DialogUtils::promptTextInput(
            this,
            tr("Rename"),
            tr("Enter a new name for \"%1\":").arg(currentName),
            currentName,
            &newName,
            0,
            selectionLength)) {
        return;
    }

    if (newName.isEmpty() || newName == currentName) {
        return;
    }

    if (newName == QStringLiteral(".") || newName == QStringLiteral("..") || newName.contains(QLatin1Char('/'))) {
        QMessageBox::warning(this, tr("Invalid Name"), tr("That name is not valid for a file or folder."));
        return;
    }

    const QUrl destinationUrl = QUrl::fromLocalFile(sourceInfo.dir().filePath(newName));
    if (destinationUrl == sourceUrl) {
        return;
    }

    KJob *job = FileOpsService::rename(sourceUrl, destinationUrl, this);
    refreshPaneAfterJob(this, job);
    connect(job, &KJob::result, this, [this, destinationUrl](KJob *finishedJob) {
        if (!finishedJob->error()) {
            selectUrlInActiveView(destinationUrl);
        }
    });
}

void Pane::beginInlineRename(QAbstractItemView *view, const QModelIndex &index) {
    Q_UNUSED(view)
    Q_UNUSED(index)
    renameSelected();
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

    QString baseName = "New Folder";
    QString folderName = baseName;
    int counter = 1;

    QDir currentDir(destinationPath);
    while (currentDir.exists(folderName)) {
        folderName = QString("%1 %2").arg(baseName).arg(counter++);
    }

    const QUrl newFolderUrl = QUrl::fromLocalFile(currentDir.filePath(folderName));
    KJob *job = FileOpsService::mkdir(newFolderUrl, this);
    refreshPaneAfterJob(this, job);
    connect(job, &KJob::result, this, [this, destinationUrl, newFolderUrl](KJob *finishedJob) {
        if (!finishedJob->error() && destinationUrl == currentRoot) {
            selectUrlInActiveView(newFolderUrl);
        }
    });
}

void Pane::goBack() {
    if (!canGoBack()) return;
    applyLocation(m_navigationState.goBack(), true);
}

void Pane::goForward() {
    if (!canGoForward()) return;
    applyLocation(m_navigationState.goForward(), true);
}

bool Pane::canGoBack() const {
    return m_navigationState.canGoBack();
}

bool Pane::canGoForward() const {
    return m_navigationState.canGoForward();
}

void Pane::refreshCurrentLocation() {
    if (dirModel) {
        if (auto *lister = dirModel->dirLister()) {
            lister->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
        }
    }
    if (proxy) {
        proxy->invalidate();
    }
    updateStatus();
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

    if (!m_showThumbnails) {
        return QIcon();
    }
    
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
    int selectedFiles = 0;
    qint64 selectedSize = 0;
    int totalFiles = 0;

    if (stack->currentWidget() == miller) {
        if (!proxy) return;
        totalFiles = proxy->rowCount();
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

    if (stack->currentWidget() == searchResultsView) {
        totalFiles = searchResultsModel ? searchResultsModel->rowCount() : 0;
    } else {
        if (!proxy) return;
        totalFiles = proxy->rowCount();
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
        const QUrl url = primarySelectionUrl();
        if (!url.isValid()) return;

        QFileInfo fi(url.toLocalFile());
        if (fi.isDir()) {
            tryNavigateToDirectoryUrl(url, true);
        } else {
            FileOpsService::openUrl(url, this);
        }

        if (m_previewVisible) updatePreviewForUrl(url);
        return;
    }

    const QModelIndex idx = currentSelectionIndex();
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
    } else if (stack->currentWidget() == searchResultsView && searchResultsView) {
        searchResultsView->setFocus();
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
            const QModelIndex current = currentSelectionIndexForView(view);
            if (current.isValid()) {
                QUrl url = urlForIndex(current);
                if (url.isValid()) {
                    quickLookSelected();
                    return true; // Event handled
                }
            }
        }

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            const QModelIndex current = currentSelectionIndexForView(view);
            if (!current.isValid()) {
                return true;
            }

        if (view == searchResultsView || keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
            onActivated(current);  // Ctrl+Enter opens selection.
        } else {
            renameSelected();  // Return renames selection.
        }
        return true;
    }

    if (keyEvent->key() == Qt::Key_F2) {
        const QModelIndex current = currentSelectionIndexForView(view);
        if (current.isValid()) {
            renameSelected();
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

    auto *dialog = new QDialog;
    DialogUtils::prepareModalDialog(dialog, window(), QString("Open %1 with...").arg(fi.fileName()), QSize(520, 420));
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

            bool ok;
            QString customCommand = QInputDialog::getText(
                this,
                "Custom Command",
                "Enter command to open file with:",
                QLineEdit::Normal,
                "",
                &ok
            );
            if (!ok || customCommand.isEmpty()) {
                return;
            }

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

        KService::Ptr service = selectedItem->data(Qt::UserRole + 1).value<KService::Ptr>();
        dialog->close();
        if (service) {
            auto *job = new KIO::ApplicationLauncherJob(service);
            job->setUrls({url});
            job->start();
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
        tr("Creating \"%1\"…").arg(archiveName));
    ArchiveService *service = ArchiveService::createArchive(urls, archivePath, this);
    connect(service, &ArchiveService::finished, this, [this, progress, archivePath, archiveName](bool success, const QString &errorMessage) {
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
        selectUrlInActiveView(QUrl::fromLocalFile(archivePath));
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
    runArchiveExtraction(archiveUrl, extractDir, QUrl::fromLocalFile(extractDir));
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

            auto *newFolderButton = conflictDialog.addButton(tr("Extract to New Folder"), QMessageBox::AcceptRole);
            auto *replaceButton = conflictDialog.addButton(tr("Replace Existing"), QMessageBox::DestructiveRole);
            auto *cancelButton = conflictDialog.addButton(QMessageBox::Cancel);
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
                runArchiveExtraction(archiveUrl, alternateExtractDir, QUrl::fromLocalFile(alternateExtractDir));
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
        tr("Extracting \"%1\"…").arg(fi.fileName()));
    ArchiveService *service = ArchiveService::extractArchive(archiveUrl, extractDir, conflictPolicy, this);
    connect(service, &ArchiveService::finished, this, [this, progress, fi, extractDir, selectUrlOnSuccess](bool success, const QString &errorMessage) {
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

        if (selectUrlOnSuccess.isValid() && selectUrlOnSuccess.isLocalFile()) {
            const QString selectedPath = QDir::cleanPath(selectUrlOnSuccess.toLocalFile());
            const QString selectedParent = QDir::cleanPath(QFileInfo(selectedPath).absolutePath());
            if (selectedParent == cleanCurrentRoot) {
                refreshCurrentLocation();
                selectUrlInActiveView(selectUrlOnSuccess);
                return;
            }
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
            if (auto *l = dirModel->dirLister()) {
                l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
            }

            const int successCount = state->ops.size() - state->failures;
            if (state->failures == 0) {
                QMessageBox::information(this, "Duplicate Complete",
                    QString("Successfully duplicated %1 item(s).").arg(successCount));
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
