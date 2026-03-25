#include "TabPage.h"
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeySequence>

TabPage::TabPage(const QString &startPath, QWidget *parent)
    : QWidget(parent)
{
    // Models
    fsModel = new QFileSystemModel(this);
    fsModel->setRootPath("/");
    fsModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files | QDir::Hidden);

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(fsModel);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setFilterRole(Qt::DisplayRole);

    // Toolbar
    toolbar = new QToolBar(this);
    actBack = toolbar->addAction("←");
    actForward = toolbar->addAction("→");
    actUp = toolbar->addAction("↑");

    pathEdit = new QLineEdit(this);
    pathEdit->setPlaceholderText("Path…");
    toolbar->addWidget(new QLabel(" Path: "));
    toolbar->addWidget(pathEdit);

    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(" View: "));
    viewModeBox = new QComboBox(this);
    viewModeBox->addItems({"Icons", "Details", "Compact", "Miller"});
    toolbar->addWidget(viewModeBox);

    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(" Search: "));
    searchEdit = new QLineEdit(this);
    toolbar->addWidget(searchEdit);

    // Views
    viewStack = new QStackedWidget(this);

    iconView = new QListView(this);
    iconView->setViewMode(QListView::IconMode);
    iconView->setResizeMode(QListView::Adjust);
    iconView->setIconSize(QSize(64,64));
    iconView->setModel(proxy);
    viewStack->addWidget(iconView);

    detailsView = new QTreeView(this);
    detailsView->setModel(proxy);
    detailsView->setRootIsDecorated(false);
    detailsView->setAlternatingRowColors(true);
    detailsView->setSortingEnabled(true);
    detailsView->header()->setStretchLastSection(true);
    viewStack->addWidget(detailsView);

    compactView = new QListView(this);
    compactView->setModel(proxy);
    compactView->setSpacing(2);
    compactView->setUniformItemSizes(true);
    viewStack->addWidget(compactView);

    millerView = new MillerView(this);
    viewStack->addWidget(millerView);

    // Quick Look
    ql = new QuickLookDialog(this);

    // Layout
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0,0,0,0);
    v->addWidget(toolbar);
    v->addWidget(viewStack);

    // Connections
    connect(actBack, &QAction::triggered, this, &TabPage::goBack);
    connect(actForward, &QAction::triggered, this, &TabPage::goForward);
    connect(actUp, &QAction::triggered, this, &TabPage::goUp);
    connect(pathEdit, &QLineEdit::returnPressed, this, &TabPage::onPathEdited);
    connect(viewModeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TabPage::onViewModeChanged);
    connect(searchEdit, &QLineEdit::textChanged, this, &TabPage::onSearchChanged);

    connect(iconView, &QListView::activated, this, &TabPage::onActivated);
    connect(detailsView, &QTreeView::activated, this, &TabPage::onActivated);
    connect(compactView, &QListView::activated, this, &TabPage::onActivated);

    navigateTo(startPath, false);
    history << startPath; historyIndex = 0;
    emit titlePathChanged(startPath);
}

void TabPage::setPath(const QString &path) {
    navigateTo(path, true);
}

void TabPage::navigateTo(const QString &path, bool recordHistory) {
    QFileInfo info(path);
    QString target = info.isDir() ? path : info.absolutePath();
    updateViewsRoot(target);
    pathEdit->setText(target);
    emit titlePathChanged(target);

    if (recordHistory) {
        if (historyIndex < history.size()-1) {
            history = history.mid(0, historyIndex+1);
        }
        history << target;
        historyIndex = history.size()-1;
    }
}

void TabPage::updateViewsRoot(const QString &path) {
    QModelIndex sourceIdx = fsModel->index(path);
    QModelIndex rootIdx = proxy->mapFromSource(sourceIdx);

    iconView->setRootIndex(rootIdx);
    detailsView->setRootIndex(rootIdx);
    compactView->setRootIndex(rootIdx);
    millerView->setRootPath(path);
}

void TabPage::goBack() {
    if (historyIndex > 0) {
        historyIndex--;
        navigateTo(history[historyIndex], false);
    }
}
void TabPage::goForward() {
    if (historyIndex+1 < history.size()) {
        historyIndex++;
        navigateTo(history[historyIndex], false);
    }
}
void TabPage::goUp() {
    QDir d(pathEdit->text());
    if (d.cdUp()) navigateTo(d.absolutePath(), true);
}

void TabPage::onPathEdited() {
    navigateTo(pathEdit->text(), true);
}

void TabPage::onViewModeChanged(int index) {
    viewStack->setCurrentIndex(index);
}

void TabPage::onSearchChanged(const QString &text) {
    proxy->setFilterWildcard(text);
}

void TabPage::onActivated(const QModelIndex &idx) {
    QModelIndex src = proxy->mapToSource(idx);
    QFileInfo info = fsModel->fileInfo(src);
    if (info.isDir()) {
        navigateTo(info.absoluteFilePath(), true);
    } else {
        // Launching files is out of scope for now.
    }
}

void TabPage::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        // Figure out which view has focus
        QString filePath;
        if (viewStack->currentIndex() == Miller) {
            filePath = millerView->currentFilePath();
        } else {
            QWidget *fw = focusWidget();
            QAbstractItemView *v = qobject_cast<QAbstractItemView*>(fw);
            if (!v) v = qobject_cast<QAbstractItemView*>(viewStack->currentWidget());
            if (v) {
                QModelIndex idx = v->currentIndex();
                if (idx.isValid()) {
                    QModelIndex src = proxy->mapToSource(idx);
                    QFileInfo info = fsModel->fileInfo(src);
                    if (info.isFile()) filePath = info.absoluteFilePath();
                }
            }
        }
        if (!filePath.isEmpty()) ql->showFile(filePath);
    } else if (event->key() == Qt::Key_Escape) {
        ql->hide();
    } else {
        QWidget::keyPressEvent(event);
    }
}
