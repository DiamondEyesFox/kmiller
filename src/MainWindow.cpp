#include <QAction>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include "MainWindow.h"
#include "Pane.h"
#include "SettingsDialog.h"
#include <QFileInfo>
#include <QDir>
#include <QVBoxLayout>
#include <QUrl>
#include <QMenuBar>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1500, 900);
    splitter = new QSplitter(this);
    setCentralWidget(splitter);

    placesModel = new KFilePlacesModel(this);
    placesView = new KFilePlacesView(splitter);
    placesView->setModel(placesModel);

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

    tb = addToolBar("Main");
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tb->setIconSize(QSize(18,18));

    actNewTab = tb->addAction(QIcon::fromTheme("tab-new"), "New Tab");
    connect(actNewTab, &QAction::triggered, this, &MainWindow::newTab);

    actCloseTab = tb->addAction(QIcon::fromTheme("tab-close"), "Close Tab");
    connect(actCloseTab, &QAction::triggered, this, &MainWindow::closeCurrentTab);

    tb->setVisible(false);

    connect(placesView, &KFilePlacesView::urlChanged, this, &MainWindow::placeActivated);

    buildMenus();
    // ---- View menu with Preview Pane ----
    {
        QMenu *viewMenu = menuBar()->addMenu("View");
        actPreviewPane = viewMenu->addAction("Preview Pane");
        actPreviewPane->setCheckable(true);
        actPreviewPane->setChecked(false);
        connect(actPreviewPane, &QAction::toggled, this, [this](bool on){
            if (auto *p = currentPane()) p->setPreviewVisible(on);
        });
    }
    addInitialTab(QUrl::fromLocalFile(QDir::homePath()));
}

void MainWindow::buildMenus() {
    auto file = menuBar()->addMenu("&File");
    auto *helpMenu = menuBar()->addMenu("&Help");
    QAction *actNotes = helpMenu->addAction("Patch Notes…");
    connect(actNotes, &QAction::triggered, this, []{
        QUrl u = QUrl::fromLocalFile("/opt/kmiller/PATCHNOTES-latest.md");
        QDesktopServices::openUrl(u);
    });
    file->addAction(QIcon::fromTheme("tab-new"), "New Tab\tCtrl+T", this, &MainWindow::newTab);
    file->addAction(QIcon::fromTheme("tab-close"), "Close Tab\tCtrl+W", this, &MainWindow::closeCurrentTab);
    file->addSeparator();
    actPrefs = file->addAction(QIcon::fromTheme("configure"), "Preferences…");
    connect(actPrefs, &QAction::triggered, this, &MainWindow::openPreferences);

    auto edit = menuBar()->addMenu("&Edit");
    edit->addAction("Cut");
    edit->addAction("Copy");
    edit->addAction("Paste");

    auto view = menuBar()->addMenu("&View");
    view->addAction("Icons", this, &MainWindow::setViewIcons);
    view->addAction("Details", this, &MainWindow::setViewDetails);
    view->addAction("Compact", this, &MainWindow::setViewCompact);
    view->addAction("Miller Columns", this, &MainWindow::setViewMiller);
    view->addSeparator();

    actShowToolbar = view->addAction("Show Toolbar");
    actShowToolbar->setCheckable(true);
    actShowToolbar->setChecked(false);
    connect(actShowToolbar, &QAction::toggled, this, &MainWindow::toggleToolbar);

    view->addSeparator();
    actQuickLook = view->addAction("Quick Look\tSpace", this, &MainWindow::quickLook);

    auto go = menuBar()->addMenu("&Go");
    go->addAction("Up", [this]{ if (auto p=currentPane()) p->goUp(); });
    go->addAction("Home", [this]{ if (auto p=currentPane()) p->goHome(); });

    auto tools = menuBar()->addMenu("&Tools");
    tools->addAction("Preferences…", this, &MainWindow::openPreferences);

    auto help = menuBar()->addMenu("&Help");
    help->addAction("About", []{});
}}
void MainWindow::addInitialTab(const QUrl &url) {
    Pane *p = new Pane(url, this);
    int idx = tabs->addTab(p, url.isLocalFile() ? QFileInfo(url.toLocalFile()).fileName() : url.toString());
    tabs->setCurrentIndex(idx);
}

void MainWindow::newTab() { addInitialTab(QUrl::fromLocalFile(QDir::homePath())); }

void MainWindow::closeCurrentTab() {
    int idx = tabs->currentIndex();
    if (idx>=0) {
        QWidget *w = tabs->widget(idx);
        tabs->removeTab(idx);
        delete w;
        if (tabs->count()==0) newTab();
    }
}

void MainWindow::placeActivated(const QUrl &url) { if (auto *p = currentPane()) p->setUrl(url); }
void MainWindow::toggleToolbar(bool on) { tb->setVisible(on); }
void MainWindow::openPreferences() { SettingsDialog dlg(this); dlg.exec(); }
void MainWindow::setViewIcons(){ if (auto p=currentPane()) p->setViewMode(0); }
void MainWindow::setViewDetails(){ if (auto p=currentPane()) p->setViewMode(1); }
void MainWindow::setViewCompact(){ if (auto p=currentPane()) p->setViewMode(2); }
void MainWindow::setViewMiller(){ if (auto p=currentPane()) p->setViewMode(3); }
void MainWindow::quickLook(){ if (auto p=currentPane()) p->quickLookSelected(); 
}

Pane* MainWindow::currentPane() const {
    return qobject_cast<Pane*>(tabs->currentWidget());
}
