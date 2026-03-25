#include "MillerView.h"
#include <QFileSystemModel>
#include <QDir>
#include <QDebug>
#include <QContextMenuEvent>
#include <KIO/OpenUrlJob>
#include <KIO/Global>

MillerView::MillerView(QWidget *parent) : QWidget(parent) {
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
}

void MillerView::setRootUrl(const QUrl &url) {
    root = url;
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    columns.clear();
    addColumn(url);
}

void MillerView::addColumn(const QUrl &url) {
    QFileSystemModel *model = new QFileSystemModel(this);
    model->setRootPath(url.toLocalFile());

    QListView *view = new QListView(this);
    view->setModel(model);
    view->setRootIndex(model->index(url.toLocalFile()));
    view->setSelectionMode(QAbstractItemView::SingleSelection);

    layout->addWidget(view);
    columns.push_back(view);

    connect(view, &QListView::doubleClicked, this, [this, model, view](const QModelIndex &idx){
        QUrl url = QUrl::fromLocalFile(model->filePath(idx));
        QFileInfo fi(model->filePath(idx));
        if (fi.isDir()) {
            addColumn(url);
        } else {
            auto *job = new KIO::OpenUrlJob(url);
            job->start();
        }
    });

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QWidget::customContextMenuRequested, this, [this, view](const QPoint &pos){
        emit contextMenuRequested(view->viewport()->mapToGlobal(pos));
    });
}

bool MillerView::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent*>(event);
        QListView *view = qobject_cast<QListView*>(obj);
        if (!view) return false;

        if (keyEvent->key() == Qt::Key_Right) {
            QModelIndex idx = view->currentIndex();
            if (idx.isValid()) {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(view->model());
                if (model && model->isDir(idx)) {
                    QUrl url = QUrl::fromLocalFile(model->filePath(idx));
                    addColumn(url);
                    return true;
                }
            }
        } else if (keyEvent->key() == Qt::Key_Left) {
            if (columns.size() > 1) {
                QListView *last = columns.takeLast();
                layout->removeWidget(last);
                delete last;
                return true;
            }
        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QModelIndex idx = view->currentIndex();
            if (idx.isValid()) {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(view->model());
                if (model) {
                    QUrl url = QUrl::fromLocalFile(model->filePath(idx));
                    QFileInfo fi(model->filePath(idx));
                    if (fi.isDir()) {
                        addColumn(url);
                    } else {
                        auto *job = new KIO::OpenUrlJob(url);
                        job->start();
                    }
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
