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
#include <QSplitter>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QFileInfo>
#include <QImageReader>

#include <KFileItem>
#include <KDirModel>
#include <KDirLister>
#include <KDirSortFilterProxyModel>
#include <KIO/OpenUrlJob>
#include <KIO/CopyJob>
#include <QClipboard>
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>

#include <poppler-qt6.h>

static bool isImageFile(const QString &path) {
    const QByteArray fmt = QImageReader::imageFormat(path);
    return !fmt.isEmpty();
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

    root->addWidget(tb);

    // Splitter: top = stacked views, bottom = preview
    vsplit = new QSplitter(Qt::Vertical, this);
    vsplit->setChildrenCollapsible(false);
    root->addWidget(vsplit);

    stack = new QStackedWidget(this);
    vsplit->addWidget(stack);

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
    vsplit->addWidget(preview);
    preview->setVisible(false); // hidden by default

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
    iconView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stack->addWidget(iconView);

    detailsView = new QTreeView(this);
    detailsView->setModel(proxy);
    detailsView->setRootIsDecorated(false);
    detailsView->setAlternatingRowColors(true);
    detailsView->setSortingEnabled(true);
    detailsView->header()->setStretchLastSection(true);
    detailsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stack->addWidget(detailsView);

    compactView = new QListView(this);
    compactView->setModel(proxy);
    compactView->setUniformItemSizes(true);
    compactView->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
    connect(miller, &MillerView::contextMenuRequested, this, [this](const QUrl &url, const QPoint &globalPos){ showContextMenu(globalPos, {url}); });

    ql = new QuickLookDialog(this);
    thumbs = new ThumbCache(this);

    connect(viewBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &Pane::onViewModeChanged);
    connect(zoom, &QSlider::valueChanged, this, &Pane::onZoomChanged);

    // Selection -> preview (Icons/Details/Compact)
    auto hookSel = [this](QAbstractItemView *v){
        if (!v) return;
        connect(v->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &Pane::onCurrentChanged);
        connect(v, &QAbstractItemView::activated, this, &Pane::onActivated);
        connect(v, &QAbstractItemView::clicked,   this, &Pane::onActivated); // open on click for list views if desired
        
        // Context menu setup
        v->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(v, &QAbstractItemView::customContextMenuRequested, this, [this, v](const QPoint &pos){
            QModelIndex index = v->indexAt(pos);
            QList<QUrl> urls;
            if (index.isValid()) {
                urls << urlForIndex(index);
            }
            showContextMenu(v->mapToGlobal(pos), urls);
        });
        
        // Add Enter key handling for all views
        auto *enterShortcut = new QShortcut(Qt::Key_Return, v);
        connect(enterShortcut, &QShortcut::activated, this, [this, v](){
            QModelIndex current = v->currentIndex();
            if (current.isValid()) {
                onActivated(current);
            }
        });
        
        auto *enterShortcut2 = new QShortcut(Qt::Key_Enter, v);
        connect(enterShortcut2, &QShortcut::activated, this, [this, v](){
            QModelIndex current = v->currentIndex();
            if (current.isValid()) {
                onActivated(current);
            }
        });
        
        // Add spacebar Quick Look for all views
        auto *spaceShortcut = new QShortcut(Qt::Key_Space, v);
        connect(spaceShortcut, &QShortcut::activated, this, [this, v](){
            QModelIndex current = v->currentIndex();
            if (current.isValid()) {
                QUrl url = urlForIndex(current);
                if (url.isValid()) {
                    if (ql && ql->isVisible()) { 
                        ql->close(); 
                    } else { 
                        if (!ql) ql = new QuickLookDialog(this); 
                        ql->showFile(url.toLocalFile()); 
                    }
                }
            }
        });
    };
    hookSel(iconView);
    hookSel(detailsView);
    hookSel(compactView);

    applyIconSize(64);
    setRoot(startUrl);
    // Sync UI and set default to Miller view
    viewBox->setCurrentIndex(3);
    setViewMode(3);
}

void Pane::setRoot(const QUrl &url) {
    currentRoot = url;
    if (auto *l = dirModel->dirLister()) {
        l->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));
    }
    if (miller) miller->setRootUrl(url);
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
    if (!ql) ql = new QuickLookDialog(this);
    
    // Get currently selected item(s)
    auto *view = qobject_cast<QAbstractItemView*>(stack->currentWidget());
    if (!view) return;
    
    auto *selModel = view->selectionModel();
    if (!selModel) return;
    
    auto indices = selModel->selectedIndexes();
    if (indices.isEmpty()) {
        // Try current index if nothing selected
        QModelIndex current = view->currentIndex();
        if (current.isValid()) {
            QUrl url = urlForIndex(current);
            if (url.isLocalFile()) {
                ql->showFile(url.toLocalFile());
            }
        }
        return;
    }
    
    // Show first selected file
    for (const auto &idx : indices) {
        QUrl url = urlForIndex(idx);
        if (url.isLocalFile()) {
            ql->showFile(url.toLocalFile());
            break; // Only show first one
        }
    }
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

void Pane::showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls)
{
    QMenu menu(this);
    
    // If no URLs provided, show folder context menu
    if (urls.isEmpty() || !urls.first().isValid()) {
        menu.addAction(QIcon::fromTheme("folder-new"), "New Folder", this, [this]() {
            bool ok;
            QString name = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "New Folder", &ok);
            if (ok && !name.isEmpty()) {
                QString newPath = currentRoot.toLocalFile() + "/" + name;
                if (QDir().mkpath(newPath)) {
                    // Refresh the view
                    if (auto *l = dirModel->dirLister()) {
                        l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
                    }
                } else {
                    QMessageBox::warning(this, "Error", "Failed to create folder");
                }
            }
        });
        menu.addSeparator();
        menu.addAction("Paste", this, &Pane::paste)->setEnabled(canPaste());
    } else {
        // File/folder context menu
        QUrl url = urls.first();
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        bool isDir = fileInfo.isDir();
        
        if (isDir) {
            menu.addAction(QIcon::fromTheme("document-open-folder"), "Open", this, [this, url]() {
                setRoot(url);  // Navigate internally instead of opening in Dolphin
            });
        } else {
            menu.addAction(QIcon::fromTheme("document-open"), "Open", this, [url]() {
                KIO::OpenUrlJob *job = new KIO::OpenUrlJob(url);
                job->start();
            });
        }
        
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("edit-copy"), "Copy", this, [this, urls]() { copy(urls); });
        menu.addAction(QIcon::fromTheme("edit-cut"), "Cut", this, [this, urls]() { cut(urls); });
        
        if (canPaste()) {
            menu.addAction(QIcon::fromTheme("edit-paste"), "Paste", this, &Pane::paste);
        }
        
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("edit-rename"), "Rename", this, [this, url, fileInfo]() {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, fileInfo.fileName(), &ok);
            if (ok && !newName.isEmpty() && newName != fileInfo.fileName()) {
                QString newPath = fileInfo.absolutePath() + "/" + newName;
                if (QFile::rename(url.toLocalFile(), newPath)) {
                    // Refresh the view
                    if (auto *l = dirModel->dirLister()) {
                        l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
                    }
                } else {
                    QMessageBox::warning(this, "Error", "Failed to rename file");
                }
            }
        });
        
        menu.addAction(QIcon::fromTheme("edit-delete"), "Delete", this, [this, url]() {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete", 
                QString("Are you sure you want to delete '%1'?").arg(QFileInfo(url.toLocalFile()).fileName()),
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                if (QFile::remove(url.toLocalFile()) || QDir().rmdir(url.toLocalFile())) {
                    // Refresh the view
                    if (auto *l = dirModel->dirLister()) {
                        l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
                    }
                } else {
                    QMessageBox::warning(this, "Error", "Failed to delete file");
                }
            }
        });
        
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("document-preview"), "Quick Look", this, [this, filePath]() {
            if (!ql) ql = new QuickLookDialog(this);
            ql->showFile(filePath);
        });
    }
    
    menu.exec(globalPos);
}

// Clipboard operations implementation
void Pane::copy(const QList<QUrl> &urls)
{
    if (urls.isEmpty()) return;
    
    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);
    mimeData->setData("application/x-kde-cutselection", "0"); // 0 = copy, 1 = cut
    QApplication::clipboard()->setMimeData(mimeData);
}

void Pane::cut(const QList<QUrl> &urls)
{
    if (urls.isEmpty()) return;
    
    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);
    mimeData->setData("application/x-kde-cutselection", "1"); // 0 = copy, 1 = cut
    QApplication::clipboard()->setMimeData(mimeData);
}

void Pane::paste()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData || !mimeData->hasUrls()) return;
    
    QList<QUrl> sourceUrls = mimeData->urls();
    if (sourceUrls.isEmpty()) return;
    
    // Check if this is a cut operation
    bool isCut = mimeData->data("application/x-kde-cutselection") == "1";
    
    // Use KIO to perform the copy/move operation
    KIO::CopyJob *job;
    if (isCut) {
        job = KIO::move(sourceUrls, currentRoot, KIO::DefaultFlags);
    } else {
        job = KIO::copy(sourceUrls, currentRoot, KIO::DefaultFlags);
    }
    
    // Refresh view when operation completes
    connect(job, &KIO::CopyJob::finished, this, [this]() {
        if (auto *l = dirModel->dirLister()) {
            l->openUrl(currentRoot, KDirLister::OpenUrlFlags(KDirLister::Reload));
        }
    });
}

bool Pane::canPaste()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    return mimeData && mimeData->hasUrls();
}
