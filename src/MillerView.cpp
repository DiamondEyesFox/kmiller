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

    auto *view = new QListView(this);
    view->setModel(model);
    view->setRootIndex(model->index(rootPath));
    view->setSelectionMode(QAbstractItemView::ExtendedSelection); // allow multi
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);     // no rename on dblclick

    // Context menu on viewport
    view->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view->viewport(), &QWidget::customContextMenuRequested, this,
            [this, view, model](const QPoint &vpPos){
        QModelIndex idx = view->indexAt(vpPos);
        QUrl u;
        if (idx.isValid()) u = QUrl::fromLocalFile(model->filePath(idx));
        emit contextMenuRequested(u, view->viewport()->mapToGlobal(vpPos));
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

    if (ke->key() == Qt::Key_Right || ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
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
    } else if (ke->key() == Qt::Key_Left) {
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
    return QWidget::eventFilter(obj, event);
}
