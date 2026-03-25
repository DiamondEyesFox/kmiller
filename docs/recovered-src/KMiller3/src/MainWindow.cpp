#include "MainWindow.h"
#include "Pane.h"
#include <QFileInfo>
#include <QDir>
#include <QVBoxLayout>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1500, 900);
    splitter = new QSplitter(this);
    setCentralWidget(splitter);

    // Left: Places
    placesModel = new KFilePlacesModel(this);
    placesView = new KFilePlacesView(splitter);
    placesView->setModel(placesModel);

    // Right container
    rightContainer = new QWidget(splitter);
    auto *v = new QVBoxLayout(rightContainer);
    v->setContentsMargins(0,0,0,0);
    tabs = new QTabWidget(rightContainer);
    tabs->setTabsClosable(true);
    tabs->setDocumentMode(true);
    v->addWidget(tabs);

    connect(tabs, &QTabWidget::tabCloseRequested, this, [this](int idx){
        QWidget *w = tabs->widget(idx);
        tabs->removeTab(idx);
        delete w;
        if (tabs->count()==0) newTab();
    });

    splitter->addWidget(placesView);
    splitter->addWidget(rightContainer);
    splitter->setStretchFactor(1, 1);

    // Toolbar
    tb = addToolBar("Main");
    actNewTab = tb->addAction("New Tab");
    connect(actNewTab, &QAction::triggered, this, &MainWindow::newTab);
    actCloseTab = tb->addAction("Close Tab");
    connect(actCloseTab, &QAction::triggered, this, &MainWindow::closeCurrentTab);
    actSplit = tb->addAction("Split");
    connect(actSplit, &QAction::triggered, this, &MainWindow::toggleSplit);

    connect(placesView, &KFilePlacesView::placeActivated, this, [this](const QModelIndex &index){
        QUrl url = placesModel->url(index);
        placeActivated(url);
    });

    addInitialTab(QUrl::fromLocalFile(QDir::homePath()));
}

void MainWindow::addInitialTab(const QUrl &url) {
    Pane *p = new Pane(url, this);
    int idx = tabs->addTab(p, url.isLocalFile()? QFileInfo(url.toLocalFile()).fileName() : url.toString());
    tabs->setCurrentIndex(idx);
}

void MainWindow::newTab() {
    addInitialTab(QUrl::fromLocalFile(QDir::homePath()));
}

void MainWindow::closeCurrentTab() {
    int idx = tabs->currentIndex();
    if (idx>=0) {
        QWidget *w = tabs->widget(idx);
        tabs->removeTab(idx);
        delete w;
        if (tabs->count()==0) newTab();
    }
}

void MainWindow::toggleSplit() {
    splitOn = !splitOn;
    rebuildRightArea();
}

void MainWindow::rebuildRightArea() {
    delete rightContainer->layout();
    auto *v = new QVBoxLayout(rightContainer);
    v->setContentsMargins(0,0,0,0);

    if (!splitOn) {
        // tabs only
        v->addWidget(tabs);
        leftPane = nullptr;
        rightPane = nullptr;
        return;
    }

    // split: two panes side by side using a splitter
    auto *hsplit = new QSplitter(Qt::Horizontal, rightContainer);
    leftPane = new Pane(QUrl::fromLocalFile(QDir::homePath()), rightContainer);
    rightPane = new Pane(QUrl::fromLocalFile(QDir::homePath()), rightContainer);
    hsplit->addWidget(leftPane);
    hsplit->addWidget(rightPane);
    hsplit->setStretchFactor(0,1);
    hsplit->setStretchFactor(1,1);
    v->addWidget(hsplit);
}

void MainWindow::placeActivated(const QUrl &url) {
    if (splitOn && leftPane) {
        leftPane->setUrl(url);
        return;
    }
    auto *p = qobject_cast<Pane*>(tabs->currentWidget());
    if (!p) return;
    p->setUrl(url);
}
