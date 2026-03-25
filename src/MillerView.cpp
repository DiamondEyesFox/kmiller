#include "MillerView.h"
#include "FileOpsService.h"
#include "ThumbCache.h"
#include <memory>
#include <QCryptographicHash>
#include <QDir>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QKeyEvent>
#include <QPointer>
#include <QAbstractItemDelegate>
#include <QStandardPaths>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QListView>
#include <QMessageBox>

static bool isMillerImageFile(const QString &path) {
    const QByteArray fmt = QImageReader::imageFormat(path);
    return !fmt.isEmpty();
}

static bool supportsMillerThumbnailPreview(const QString &path) {
    QFileInfo fi(path);
    return isMillerImageFile(path) || fi.suffix().compare("pdf", Qt::CaseInsensitive) == 0;
}

static QString millerThumbnailPath(const QUrl &url) {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty()) cacheDir = QDir::homePath() + "/.cache";

    QDir dir(cacheDir + "/kmiller/thumbs");
    if (!dir.exists()) dir.mkpath(dir.absolutePath());

    const QString hash = QString(QCryptographicHash::hash(url.toString().toUtf8(), QCryptographicHash::Md5).toHex());
    return dir.absoluteFilePath(hash + ".png");
}

static QString millerDisplayNameForFileInfo(const QFileInfo &fi, bool showFileExtensions) {
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

class MillerItemDelegate final : public QStyledItemDelegate {
public:
    using UrlResolver = std::function<QUrl(const QModelIndex &)>;
    using IconResolver = std::function<QIcon(const QUrl &)>;
    using BoolResolver = std::function<bool()>;

    MillerItemDelegate(UrlResolver urlResolver,
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
            option->text = millerDisplayNameForFileInfo(fi, false);
        }

        if (m_showThumbnailsResolver && m_showThumbnailsResolver() && supportsMillerThumbnailPreview(fi.absoluteFilePath())) {
            option->icon = m_iconResolver ? m_iconResolver(url) : option->icon;
        }
    }

private:
    UrlResolver m_urlResolver;
    IconResolver m_iconResolver;
    BoolResolver m_showThumbnailsResolver;
    BoolResolver m_showFileExtensionsResolver;
};

MillerView::MillerView(QWidget *parent) : QWidget(parent) {
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    m_thumbs = new ThumbCache(this);
}

void MillerView::setRootUrl(const QUrl &url) {
    root = url;
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    columns.clear();
    addColumn(url);
}

void MillerView::pruneColumnsAfter(QListView *view) {
    const int pos = columns.indexOf(view);
    if (pos < 0) return;
    while (columns.size() > pos + 1) {
        QListView *last = columns.takeLast();
        layout->removeWidget(last);
        last->deleteLater();
    }
}

void MillerView::addColumn(const QUrl &url) {
    emit navigatedTo(url);

    auto *view = new QListView(this);
    auto *model = new QFileSystemModel(view);
    const QString rootPath = url.toLocalFile();
    model->setResolveSymlinks(m_followSymlinks);
    model->setRootPath(rootPath);
    model->setReadOnly(false);  // Enable drag & drop modifications

    // Set hidden file filter based on current setting
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (m_showHiddenFiles) {
        filters |= QDir::Hidden;
    }
    model->setFilter(filters);

    view->setModel(model);
    view->setRootIndex(model->index(rootPath));
    view->setMinimumWidth(m_columnWidth);
    view->setMaximumWidth(m_columnWidth);
    view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection); // allow multi
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);     // no rename on dblclick
    view->setItemDelegate(new MillerItemDelegate(
        [model](const QModelIndex &idx) {
            const QString path = model->filePath(idx);
            return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path);
        },
        [this](const QUrl &fileUrl) { return getIconForFile(fileUrl); },
        [this]() { return m_showThumbnails; },
        [this]() { return m_showFileExtensions; },
        view
    ));

    // Enable drag and drop between columns
    view->setDragEnabled(true);
    view->setAcceptDrops(true);
    view->setDropIndicatorShown(true);
    view->setDragDropMode(QAbstractItemView::DragDrop);
    view->setDefaultDropAction(Qt::MoveAction);  // Default to move (like Finder)

    // Minimal styling - let theme handle colors, just add selection behavior
    view->setStyleSheet(
        "QListView { "
        "  border: none; "
        "  border-right: 1px solid palette(mid); "
        "}"
        "QListView::item { "
        "  padding: 2px 4px; "
        "  border-radius: 4px; "
        "  margin: 1px 2px; "
        "}"
        "QListView::item:selected { "
        "  background-color: palette(highlight); "
        "  color: palette(highlighted-text); "
        "}"
        "QListView::item:selected:!active { "
        "  background-color: #505050; "
        "  color: palette(highlighted-text); "
        "}"
        "QScrollBar:vertical { "
        "  background-color: transparent; width: 12px; border-radius: 6px; "
        "}"
        "QScrollBar::handle:vertical { "
        "  background-color: palette(mid); border-radius: 6px; min-height: 20px; margin: 2px; "
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar:horizontal { "
        "  background-color: transparent; height: 12px; border-radius: 6px; "
        "}"
        "QScrollBar::handle:horizontal { "
        "  background-color: palette(mid); border-radius: 6px; min-width: 20px; margin: 2px; "
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
    );

    // Context menu
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QWidget::customContextMenuRequested, this,
            [this, view, model](const QPoint &pos){
        // Set focus to this column
        view->setFocus();

        QModelIndex idx = view->indexAt(pos);
        if (idx.isValid()) {
            // Finder-like behavior: right-clicking an unselected item switches selection to it.
            if (QItemSelectionModel *sel = view->selectionModel();
                sel && !sel->isSelected(idx)) {
                sel->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                view->setCurrentIndex(idx);
            }

            // Collect all selected URLs from this column
            QList<QUrl> selectedUrls;
            QItemSelectionModel *sel = view->selectionModel();
            if (sel) {
                const auto indexes = sel->selectedIndexes();
                for (const QModelIndex &i : indexes) {
                    // Only count column 0 to avoid duplicates (model has Name, Size, Type, Date columns)
                    if (i.column() != 0) continue;
                    QString path = model->filePath(i);
                    if (!path.isEmpty()) {
                        selectedUrls.append(QUrl::fromLocalFile(path));
                    }
                }
            }
            // If nothing selected, use the clicked item
            if (selectedUrls.isEmpty()) {
                selectedUrls.append(QUrl::fromLocalFile(model->filePath(idx)));
            }
            emit contextMenuRequested(selectedUrls, view->mapToGlobal(pos));
        } else {
            // Empty space context menu - pass this column's folder URL
            QUrl folderUrl = QUrl::fromLocalFile(model->rootPath());
            emit emptySpaceContextMenuRequested(folderUrl, view->mapToGlobal(pos));
        }
    });

    // ---- Open helper (used by double-click and Right/Enter) ----
    auto openIndex = [this, model, view](const QModelIndex &idx){
        if (!idx.isValid()) return;
        const QString p = model->filePath(idx);
        if (p.isEmpty()) return;

        QFileInfo fi(p);
        if (fi.isDir()) {
            tryNavigateToPath(view, p, true);
        } else {
            pruneColumnsAfter(view);
            FileOpsService::openUrl(QUrl::fromLocalFile(p), this);
        }
    };

    // Selection changes for preview pane only
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this, model](const QModelIndex &current, const QModelIndex &) {
        if (current.isValid()) {
            emit selectionChanged(QUrl::fromLocalFile(model->filePath(current)));
        }
    });

    // Single-click: set focus to this column, and navigate into folders
    connect(view, &QListView::clicked, this, [this, model, view](const QModelIndex &idx) {
        // Always set focus to clicked column
        view->setFocus();

        if (!idx.isValid()) return;

        if (QItemSelectionModel *sel = view->selectionModel(); sel && sel->isSelected(idx)) {
            const bool sameTarget = (m_renameClickView == view && m_renameClickIndex == idx);
            const qint64 elapsedMs = m_renameClickTimer.isValid() ? m_renameClickTimer.elapsed() : -1;
            // Finder-style slow second click on selected item enters rename mode.
            if (sameTarget && elapsedMs >= 500 && elapsedMs <= 2000) {
                view->setCurrentIndex(idx);
                renameSelected();
                m_renameClickTimer.invalidate();
                m_renameClickIndex = QPersistentModelIndex();
                return;
            }
            m_renameClickView = view;
            m_renameClickIndex = idx;
            m_renameClickTimer.restart();
        }

        const QString path = model->filePath(idx);
        if (path.isEmpty()) return;

        QFileInfo fi(path);
        if (fi.isDir()) {
            tryNavigateToPath(view, path, false);
        }
    });

    // Right-click should also set focus before showing context menu
    connect(view, &QWidget::customContextMenuRequested, this, [view](const QPoint &) {
        view->setFocus();
    }, Qt::DirectConnection);  // Direct so focus is set before menu

    // Double-click: open files with default app
    connect(view, &QListView::doubleClicked, this, openIndex);

    // Keyboard handling for open/back/quicklook
    view->installEventFilter(this);

    layout->addWidget(view);
    columns.push_back(view);
    view->setFocus(Qt::OtherFocusReason);

    // Select first item after model is loaded AND sorted
    QPointer<QListView> vptr(view);
    QPointer<QFileSystemModel> mptr(model);

    // Track if we've already selected (to avoid re-selecting on every layoutChanged)
    auto selected = std::make_shared<bool>(false);

    // When directory loads, trigger sort
    connect(model, &QFileSystemModel::directoryLoaded, this,
        [this, mptr, rootPath](const QString &loaded){
            if (!mptr) return;
            if (loaded == rootPath) {
                mptr->sort(m_sortColumn, m_sortOrder);
            }
        }
    );

    // When layout changes (after sort), select first item
    connect(model, &QAbstractItemModel::layoutChanged, this,
        [vptr, mptr, selected](){
            if (!vptr || !mptr || *selected) return;
            QModelIndex r = vptr->rootIndex();
            int rows = mptr->rowCount(r);
            if (rows > 0) {
                QModelIndex first = mptr->index(0, 0, r);
                vptr->setCurrentIndex(first);
                vptr->scrollTo(first, QAbstractItemView::PositionAtTop);
                *selected = true;
            }
        }
    );
}

bool MillerView::eventFilter(QObject *obj, QEvent *event) {
    auto *view = qobject_cast<QListView*>(obj);
    if (!view) return QWidget::eventFilter(obj, event);
    if (event->type() != QEvent::KeyPress) return QWidget::eventFilter(obj, event);

    auto *ke = static_cast<QKeyEvent*>(event);
    auto *model = qobject_cast<QFileSystemModel*>(view->model());
    if (!model) return QWidget::eventFilter(obj, event);

    if (ke->key() == Qt::Key_Space) {
        QModelIndex idx = view->currentIndex();
        if (idx.isValid()) {
            const QString p = model->filePath(idx);
            if (!p.isEmpty()) emit quickLookRequested(p); // Pane toggles it
        }
        return true;
    }

    if (ke->key() == Qt::Key_Right) {
        // Right arrow: only enter directories, do nothing on files (Finder behavior)
        QModelIndex idx = view->currentIndex();
        if (!idx.isValid()) return true;
        const QString p = model->filePath(idx);
        if (p.isEmpty()) return true;

        QFileInfo fi(p);
        if (fi.isDir()) {
            tryNavigateToPath(view, p, true);
        }
        // On files: do nothing (classic Finder behavior)
        return true;
    }

    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
        if (ke->modifiers().testFlag(Qt::ControlModifier)) {
            // Ctrl+Enter: open/enter selection.
            QModelIndex idx = view->currentIndex();
            if (!idx.isValid()) return true;
            const QString p = model->filePath(idx);
            if (p.isEmpty()) return true;

            pruneColumnsAfter(view);

            QFileInfo fi(p);
            if (fi.isDir()) {
                addColumn(QUrl::fromLocalFile(p));
            } else {
                FileOpsService::openUrl(QUrl::fromLocalFile(p), this);
            }
            return true;
        }

        // Return: rename selected item (Finder behavior).
        renameSelected();
        return true;
    }

    if (ke->key() == Qt::Key_F2) {
        renameSelected();
        return true;
    }

    if (ke->key() == Qt::Key_Left) {
        if (columns.size() > 1) {
            QListView *last = columns.takeLast();
            layout->removeWidget(last);
            last->deleteLater();
            QListView *prev = columns.back();
            prev->setFocus(Qt::OtherFocusReason);
            // ensure something is highlighted
            if (!prev->currentIndex().isValid()) {
                QModelIndex r = prev->rootIndex();
                auto *m = qobject_cast<QFileSystemModel*>(prev->model());
                if (m && m->rowCount(r) > 0)
                    prev->setCurrentIndex(m->index(0, 0, r));
            }
            // Emit URL of current (previous) column
            if (auto *m = qobject_cast<QFileSystemModel*>(prev->model())) {
                emit navigatedTo(QUrl::fromLocalFile(m->rootPath()));
            }
        }
        return true;
    }

    // Type-to-select: handle printable characters (Finder behavior)
    QString text = ke->text();
    if (!text.isEmpty() && text.at(0).isPrint() && !ke->modifiers().testFlag(Qt::ControlModifier)) {
        // Reset search string if more than 1 second since last keystroke
        if (!m_searchTimer.isValid() || m_searchTimer.elapsed() > 1000) {
            m_searchString.clear();
        }
        m_searchTimer.restart();
        m_searchString += text;
        typeToSelect(view, m_searchString);
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

void MillerView::beginInlineRename(QListView *view, const QModelIndex &idx) {
    if (!view || !idx.isValid()) return;
    view->setCurrentIndex(idx);
    view->setEditTriggers(QAbstractItemView::AllEditTriggers);
    view->edit(idx);

    QAbstractItemDelegate *delegate = view->itemDelegate();
    if (delegate) {
        disconnect(delegate, &QAbstractItemDelegate::closeEditor, nullptr, nullptr);
        connect(delegate, &QAbstractItemDelegate::closeEditor, this, [view]() {
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        }, Qt::QueuedConnection);
    }
}

void MillerView::renameSelected() {
    if (columns.isEmpty()) return;

    QListView *focusedView = nullptr;
    for (QListView *view : columns) {
        if (view->hasFocus()) {
            focusedView = view;
            break;
        }
    }
    if (!focusedView) {
        focusedView = columns.last();
    }

    const QModelIndex current = focusedView->currentIndex();
    if (!current.isValid()) return;
    emit renameRequested();
}

void MillerView::typeToSelect(QListView *view, const QString &text) {
    auto *model = qobject_cast<QFileSystemModel*>(view->model());
    if (!model) return;

    QModelIndex root = view->rootIndex();
    int rowCount = model->rowCount(root);

    // Find first item that starts with the search string (case-insensitive)
    for (int i = 0; i < rowCount; ++i) {
        QModelIndex idx = model->index(i, 0, root);
        QString fileName = model->fileName(idx);
        if (fileName.startsWith(text, Qt::CaseInsensitive)) {
            view->setCurrentIndex(idx);
            view->scrollTo(idx);
            return;
        }
    }

    // If no prefix match, try contains match
    for (int i = 0; i < rowCount; ++i) {
        QModelIndex idx = model->index(i, 0, root);
        QString fileName = model->fileName(idx);
        if (fileName.contains(text, Qt::CaseInsensitive)) {
            view->setCurrentIndex(idx);
            view->scrollTo(idx);
            return;
        }
    }
}

void MillerView::focusLastColumn() {
    if (columns.isEmpty()) return;

    QListView *last = columns.last();
    if (!last) return;

    last->setFocus(Qt::OtherFocusReason);

    auto *model = qobject_cast<QFileSystemModel*>(last->model());
    if (!model) return;

    QModelIndex current = last->currentIndex();
    if (!current.isValid()) {
        QModelIndex root = last->rootIndex();
        if (model->rowCount(root) > 0) {
            current = model->index(0, 0, root);
            last->setCurrentIndex(current);
            last->scrollTo(current, QAbstractItemView::PositionAtTop);
        }
    }

    if (current.isValid()) {
        if (QItemSelectionModel *sel = last->selectionModel()) {
            sel->select(current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
    }
}

QList<QUrl> MillerView::getSelectedUrls() const {
    QList<QUrl> urls;
    if (columns.isEmpty()) return urls;

    // Find the focused column (not necessarily the last one)
    QListView *focusedView = nullptr;
    for (QListView *view : columns) {
        if (view->hasFocus()) {
            focusedView = view;
            break;
        }
    }
    // Fallback to last column if none has focus
    if (!focusedView) {
        focusedView = columns.last();
    }

    auto *model = qobject_cast<QFileSystemModel*>(focusedView->model());
    if (!model) return urls;

    QItemSelectionModel *sel = focusedView->selectionModel();
    if (!sel) return urls;

    const auto indexes = sel->selectedIndexes();
    for (const QModelIndex &idx : indexes) {
        if (idx.column() != 0) continue;  // Only count column 0
        QString path = model->filePath(idx);
        if (!path.isEmpty()) {
            urls.append(QUrl::fromLocalFile(path));
        }
    }

    // If nothing selected, try current index
    if (urls.isEmpty()) {
        QModelIndex current = focusedView->currentIndex();
        if (current.isValid()) {
            QString path = model->filePath(current);
            if (!path.isEmpty()) {
                urls.append(QUrl::fromLocalFile(path));
            }
        }
    }

    return urls;
}

void MillerView::setShowHiddenFiles(bool show) {
    m_showHiddenFiles = show;
    
    // Update all existing columns
    for (QListView *view : columns) {
        if (auto *model = qobject_cast<QFileSystemModel*>(view->model())) {
            QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
            if (show) {
                filters |= QDir::Hidden;
            }
            model->setFilter(filters);
        }
    }
}

void MillerView::setShowThumbnails(bool show) {
    m_showThumbnails = show;

    for (QListView *view : columns) {
        if (view && view->viewport()) {
            view->viewport()->update();
        }
    }
}

void MillerView::setShowFileExtensions(bool show) {
    m_showFileExtensions = show;

    for (QListView *view : columns) {
        if (view && view->viewport()) {
            view->viewport()->update();
        }
    }
}

void MillerView::setColumnWidth(int width) {
    if (width < 150) width = 150;
    if (width > 400) width = 400;

    m_columnWidth = width;

    for (QListView *view : columns) {
        if (!view) continue;
        view->setMinimumWidth(width);
        view->setMaximumWidth(width);
        view->updateGeometry();
    }
}

void MillerView::setFollowSymlinks(bool follow) {
    m_followSymlinks = follow;

    for (QListView *view : columns) {
        if (auto *model = qobject_cast<QFileSystemModel *>(view->model())) {
            model->setResolveSymlinks(follow);
        }
    }
}

void MillerView::setSort(int column, Qt::SortOrder order) {
    m_sortColumn = column;
    m_sortOrder = order;

    for (QListView *view : columns) {
        if (auto *model = qobject_cast<QFileSystemModel*>(view->model())) {
            model->sort(m_sortColumn, m_sortOrder);
        }
    }
}

void MillerView::generateThumbnail(const QUrl &url) const {
    if (!m_showThumbnails || !url.isLocalFile()) return;

    const QString path = url.toLocalFile();
    const QFileInfo fi(path);
    if (!fi.exists() || fi.isDir()) return;

    if (m_thumbs && m_thumbs->has(url)) return;

    const QString thumbPath = millerThumbnailPath(url);
    const QFileInfo thumbInfo(thumbPath);

    if (thumbInfo.exists() && thumbInfo.lastModified() >= fi.lastModified()) {
        const QPixmap thumb(thumbPath);
        if (!thumb.isNull() && m_thumbs) {
            m_thumbs->put(url, thumb);
            return;
        }
    }

    QPixmap thumbnail;
    if (isMillerImageFile(path)) {
        const QPixmap original(path);
        if (!original.isNull()) {
            thumbnail = original.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }

    if (!thumbnail.isNull()) {
        thumbnail.save(thumbPath, "PNG");
        if (m_thumbs) {
            m_thumbs->put(url, thumbnail);
        }
    }
}

QIcon MillerView::getIconForFile(const QUrl &url) const {
    if (!m_showThumbnails || !url.isLocalFile()) {
        return QIcon();
    }

    if (m_thumbs && m_thumbs->has(url)) {
        return QIcon(m_thumbs->get(url));
    }

    const QString thumbPath = millerThumbnailPath(url);
    if (QFileInfo(thumbPath).exists()) {
        const QPixmap thumb(thumbPath);
        if (!thumb.isNull()) {
            if (m_thumbs) {
                m_thumbs->put(url, thumb);
            }
            return QIcon(thumb);
        }
    }

    const QString path = url.toLocalFile();
    if (supportsMillerThumbnailPreview(path)) {
        generateThumbnail(url);
        if (m_thumbs && m_thumbs->has(url)) {
            return QIcon(m_thumbs->get(url));
        }
    }

    QFileIconProvider provider;
    return provider.icon(QFileInfo(path));
}

bool MillerView::isDirectorySymlinkPath(const QString &path) const {
    const QFileInfo fi(path);
    return fi.isSymLink() && fi.isDir();
}

bool MillerView::tryNavigateToPath(QListView *view, const QString &path, bool showBlockedMessage) {
    if (path.isEmpty()) {
        return false;
    }

    if (!m_followSymlinks && isDirectorySymlinkPath(path)) {
        if (showBlockedMessage) {
            const QFileInfo fi(path);
            const QString displayName = fi.fileName().isEmpty() ? path : fi.fileName();
            QMessageBox::information(
                this,
                tr("Symlink Navigation Disabled"),
                tr("'%1' is a symbolic link to a directory.\n\nEnable 'Follow symbolic links' in Preferences to navigate into it.")
                    .arg(displayName));
        }
        return false;
    }

    pruneColumnsAfter(view);
    addColumn(QUrl::fromLocalFile(path));
    return true;
}
