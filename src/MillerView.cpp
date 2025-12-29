#include "MillerView.h"
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
    if (!columns.isEmpty()) {
        if (auto *prev = columns.back()) {
            if (prev->selectionModel()) prev->selectionModel()->clearSelection();
        }
    }

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

    // Finder-like visual styling for selection and focus
    view->setStyleSheet(
        "QListView { "
        "  border: none; "
        "  border-right: 1px solid #d0d0d0; "
        "  background-color: #ffffff; "
        "}"
        "QListView::item { "
        "  padding: 2px 4px; "
        "  border-radius: 4px; "
        "  margin: 1px 2px; "
        "}"
        "QListView::item:selected { "
        "  background-color: #007aff; "
        "  color: #ffffff; "
        "}"
        "QListView::item:selected:!active { "
        "  background-color: #c0c0c0; "  // Grayed when unfocused
        "  color: #000000; "
        "}"
        "QListView::item:hover:!selected { "
        "  background-color: #e8e8e8; "
        "}"
    );

    // Context menu on both view and viewport  
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Connect to view's context menu signal
    connect(view, &QWidget::customContextMenuRequested, this,
            [this, view, model](const QPoint &pos){
        QModelIndex idx = view->indexAt(pos);
        if (idx.isValid()) {
            QUrl u = QUrl::fromLocalFile(model->filePath(idx));
            emit contextMenuRequested(u, view->mapToGlobal(pos));
        } else {
            // Empty space context menu
            emit emptySpaceContextMenuRequested(view->mapToGlobal(pos));
        }
    });
    
    // Also connect to viewport (backup)
    connect(view->viewport(), &QWidget::customContextMenuRequested, this,
            [this, view, model](const QPoint &vpPos){
        QModelIndex idx = view->indexAt(vpPos);
        if (idx.isValid()) {
            QUrl u = QUrl::fromLocalFile(model->filePath(idx));
            emit contextMenuRequested(u, view->viewport()->mapToGlobal(vpPos));
        } else {
            // Empty space context menu
            emit emptySpaceContextMenuRequested(view->viewport()->mapToGlobal(vpPos));
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

    // Selection changes for preview pane
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, 
            [this, model](const QModelIndex &current, const QModelIndex &) {
        if (current.isValid()) {
            QUrl url = QUrl::fromLocalFile(model->filePath(current));
            emit selectionChanged(url);
        }
    });

    // Single-click: select only (Finder column behavior)
    // Double-click: open
    connect(view, &QListView::doubleClicked, this, openIndex);

    // Keyboard handling for open/back/quicklook
    view->installEventFilter(this);

    layout->addWidget(view);
    columns.push_back(view);
    view->setFocus(Qt::OtherFocusReason);

    // Select first item once content is ready
    QPointer<QListView> vptr(view);
    QPointer<QFileSystemModel> mptr(model);
    auto safeSelectFirst = [vptr, mptr]() {
        if (!vptr || !mptr) return;
        QModelIndex r = vptr->rootIndex();
        int rows = mptr->rowCount(r);
        if (rows > 0 && !vptr->currentIndex().isValid()) {
            vptr->setCurrentIndex(mptr->index(0, 0, r));
        }
        if (vptr) vptr->setFocus(Qt::OtherFocusReason);
    };
    QTimer::singleShot(0, vptr, safeSelectFirst);
    connect(model, &QFileSystemModel::directoryLoaded, this,
        [rootPath, safeSelectFirst](const QString &loaded){
            if (loaded == rootPath) safeSelectFirst();
        }
    );
    connect(model, &QFileSystemModel::rowsInserted, this,
        [vptr, mptr, safeSelectFirst](const QModelIndex &parent, int, int){
            if (!vptr || !mptr) return;
            if (parent == vptr->rootIndex()) safeSelectFirst();
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
