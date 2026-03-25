#include "MillerView.h"
#include <QDir>

MillerView::MillerView(QWidget *parent) : QWidget(parent) {
    splitter = new QSplitter(this);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(splitter);
}

void MillerView::setRootPath(const QString &path) {
    // Clear existing
    for (auto *v : columns) v->deleteLater();
    for (auto *m : models) m->deleteLater();
    columns.clear(); models.clear();
    addColumn(path);
}

void MillerView::addColumn(const QString &path) {
    auto *model = new QFileSystemModel(this);
    model->setRootPath(path);
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files | QDir::Hidden);

    auto *view = new QListView;
    view->setModel(model);
    view->setRootIndex(model->index(path));

    int colIndex = columns.size();
    QObject::connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
                     this, [this, colIndex, model](const QModelIndex &idx){
        if (model->isDir(idx)) {
            removeColumnsAfter(colIndex);
            addColumn(model->filePath(idx));
        }
    });

    columns.append(view);
    models.append(model);
    splitter->addWidget(view);
}

void MillerView::removeColumnsAfter(int col) {
    while (columns.size() > col + 1) {
        auto *v = columns.takeLast();
        auto *m = models.takeLast();
        v->deleteLater();
        m->deleteLater();
    }
}

QString MillerView::currentFilePath() const {
    for (auto *v : columns) {
        if (v->hasFocus()) {
            auto *m = qobject_cast<QFileSystemModel*>(v->model());
            auto idx = v->currentIndex();
            if (idx.isValid() && m && !m->isDir(idx)) {
                return m->filePath(idx);
            }
        }
    }
    return QString();
}
