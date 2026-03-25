#include "MillerView.h"
#include <QVBoxLayout>
#include <KDirLister>

MillerView::MillerView(QWidget *parent) : QWidget(parent) {
    splitter = new QSplitter(this);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(splitter);
}

void MillerView::setRootUrl(const QUrl &url) {
    for (auto *v : columns) v->deleteLater();
    for (auto *m : models) m->deleteLater();
    columns.clear(); models.clear();
    addColumn(url);
}

void MillerView::addColumn(const QUrl &url) {
    auto *model = new KDirModel(this);
    model->dirLister()->openUrl(url, KDirLister::OpenUrlFlags(KDirLister::Reload));

    auto *view = new QListView;
    view->setModel(model);

    int colIndex = columns.size();
    QObject::connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
                     this, [this, colIndex, model](const QModelIndex &idx){
        KFileItem item = model->itemForIndex(idx);
        if (item.isDir()) {
            removeColumnsAfter(colIndex);
            addColumn(item.url());
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
