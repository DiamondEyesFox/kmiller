#include "MainWindow.h"
#include "TabPage.h"
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QVBoxLayout>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1400, 900);
    splitter = new QSplitter(this);

    // Left: Places
    placesModel = new KFilePlacesModel(this);
    placesView = new KFilePlacesView(splitter);
    placesView->setModel(placesModel);

    // Right: Tabs
    tabs = new QTabWidget(splitter);
    tabs->setTabsClosable(true);
    tabs->setDocumentMode(true);

    connect(tabs, &QTabWidget::tabCloseRequested, this, [this](int idx){
        QWidget *w = tabs->widget(idx);
        tabs->removeTab(idx);
        delete w;
        if (tabs->count()==0) newTab();
    });

    setCentralWidget(splitter);
    splitter->addWidget(placesView);
    splitter->addWidget(tabs);
    splitter->setStretchFactor(1, 1);

    // Toolbar actions
    QToolBar *tb = addToolBar("Main");
    QAction *actNewTab = tb->addAction("New Tab");
    actNewTab->setShortcut(QKeySequence::AddTab);
    connect(actNewTab, &QAction::triggered, this, &MainWindow::newTab);

    QAction *actCloseTab = tb->addAction("Close Tab");
    actCloseTab->setShortcut(QKeySequence::Close);
    connect(actCloseTab, &QAction::triggered, this, &MainWindow::closeCurrentTab);

    // Places activation -> change path in active tab
    connect(placesView, &KFilePlacesView::urlChanged, this, &MainWindow::placeActivated);
    connect(placesView, &KFilePlacesView::placeActivated, this, [this](const QModelIndex &index){
        QUrl url = placesModel->url(index);
        placeActivated(url);
    });

    addInitialTab(QDir::homePath());
}

void MainWindow::addInitialTab(const QString &path) {
    TabPage *page = new TabPage(path, this);
    int idx = tabs->addTab(page, QFileInfo(path).fileName().isEmpty() ? "/" : QFileInfo(path).fileName());
    tabs->setCurrentIndex(idx);
    connect(page, &TabPage::titlePathChanged, this, &MainWindow::updateWindowTitle);
}

void MainWindow::newTab() {
    addInitialTab(QDir::homePath());
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

void MainWindow::placeActivated(const QUrl &url) {
    auto *page = qobject_cast<TabPage*>(tabs->currentWidget());
    if (!page) return;
    if (url.isLocalFile()) {
        page->setPath(url.toLocalFile());
    } else {
        // Non-local URLs would require KIO-slave-backed model;
        // ignored in this simplified QFileSystemModel build.
    }
}

void MainWindow::updateWindowTitle(const QString &path) {
    setWindowTitle(QString("KMiller2 — %1").arg(path));
    int idx = tabs->currentIndex();
    if (idx>=0) tabs->setTabText(idx, QFileInfo(path).fileName().isEmpty()? "/" : QFileInfo(path).fileName());
}
