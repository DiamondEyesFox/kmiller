#include "MillerView.h"
#include <memory>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPointer>
#include <QTimer>
#include <QListView>
#include <KIO/OpenUrlJob>

MillerView::MillerView(QWidget *parent) : QWidget(parent) {
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
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

    auto *model = new QFileSystemModel(this);
    const QString rootPath = url.toLocalFile();
    model->setRootPath(rootPath);
    
    // Set hidden file filter based on current setting
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (m_showHiddenFiles) {
        filters |= QDir::Hidden;
    }
    model->setFilter(filters);

    auto *view = new QListView(this);
    view->setModel(model);
    view->setRootIndex(model->index(rootPath));
    view->setSelectionMode(QAbstractItemView::ExtendedSelection); // allow multi
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);     // no rename on dblclick


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
    );

    // Context menu
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QWidget::customContextMenuRequested, this,
            [this, view, model](const QPoint &pos){
        QModelIndex idx = view->indexAt(pos);
        if (idx.isValid()) {
            // Collect all selected URLs from this column
            QList<QUrl> selectedUrls;
            QItemSelectionModel *sel = view->selectionModel();
            if (sel) {
                const auto indexes = sel->selectedIndexes();
                for (const QModelIndex &i : indexes) {
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
            // Empty space context menu
            emit emptySpaceContextMenuRequested(view->mapToGlobal(pos));
        }
    });

    // ---- Open helper (used by double-click and Right/Enter) ----
    auto openIndex = [this, model, view](const QModelIndex &idx){
        if (!idx.isValid()) return;
        const QString p = model->filePath(idx);
        if (p.isEmpty()) return;

        pruneColumnsAfter(view);

        QFileInfo fi(p);
        if (fi.isDir()) {
            addColumn(QUrl::fromLocalFile(p));
        } else {
            auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(p));
            job->start();
        }
    };

    // Selection changes for preview pane only
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this, model](const QModelIndex &current, const QModelIndex &) {
        if (current.isValid()) {
            emit selectionChanged(QUrl::fromLocalFile(model->filePath(current)));
        }
    });

    // Single-click on folder: navigate into it (Finder behavior)
    connect(view, &QListView::clicked, this, [this, model, view](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        const QString path = model->filePath(idx);
        if (path.isEmpty()) return;

        QFileInfo fi(path);
        if (fi.isDir()) {
            pruneColumnsAfter(view);
            addColumn(QUrl::fromLocalFile(path));
        }
    });

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
        [mptr, rootPath](const QString &loaded){
            if (!mptr) return;
            if (loaded == rootPath) {
                mptr->sort(0, Qt::AscendingOrder);
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
            pruneColumnsAfter(view);
            addColumn(QUrl::fromLocalFile(p));
        }
        // On files: do nothing (classic Finder behavior)
        return true;
    }

    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
        // Enter/Return: enter directories OR open files
        QModelIndex idx = view->currentIndex();
        if (!idx.isValid()) return true;
        const QString p = model->filePath(idx);
        if (p.isEmpty()) return true;

        pruneColumnsAfter(view);

        QFileInfo fi(p);
        if (fi.isDir()) {
            addColumn(QUrl::fromLocalFile(p));
        } else {
            auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(p));
            job->start();
        }
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
    if (!columns.isEmpty()) {
        columns.last()->setFocus();
    }
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
