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
#include <QLabel>
#include <QStatusBar>
#include <QSettings>
#include <QCloseEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1500, 900);
    setWindowTitle(QString("KMiller %1").arg(KMILLER_VERSION_STR));
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
    
    // Update status bar when switching tabs
    connect(tabs, &QTabWidget::currentChanged, this, [this]{
        if (auto *p = currentPane()) {
            p->updateStatus(); // Trigger status update for current pane
        }
    });

    splitter->addWidget(placesView);
    splitter->addWidget(rightContainer);
    splitter->setStretchFactor(1, 1);

    tb = addToolBar("Main");
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tb->setIconSize(QSize(18,18));

    // Navigation buttons
    auto *actBack = tb->addAction(QIcon::fromTheme("go-previous"), "Back");
    connect(actBack, &QAction::triggered, this, [this]{ if (auto p=currentPane()) p->goBack(); });
    
    auto *actForward = tb->addAction(QIcon::fromTheme("go-next"), "Forward");
    connect(actForward, &QAction::triggered, this, [this]{ if (auto p=currentPane()) p->goForward(); });
    
    tb->addSeparator();

    actNewTab = tb->addAction(QIcon::fromTheme("tab-new"), "New Tab");
    connect(actNewTab, &QAction::triggered, this, &MainWindow::newTab);

    actCloseTab = tb->addAction(QIcon::fromTheme("tab-close"), "Close Tab");
    connect(actCloseTab, &QAction::triggered, this, &MainWindow::closeCurrentTab);

    tb->setVisible(false);

    connect(placesView, &KFilePlacesView::urlChanged, this, &MainWindow::placeActivated);

    buildMenus();
    
    // Setup status bar
    statusLabel = new QLabel("Ready");
    statusBar()->addWidget(statusLabel);
    
    addInitialTab(QUrl::fromLocalFile(QDir::homePath()));
    
    // Load saved settings
    loadSettings();
}

void MainWindow::buildMenus() {
    auto file = menuBar()->addMenu("&File");
    file->addAction(QIcon::fromTheme("tab-new"), "New Tab\tCtrl+T", this, &MainWindow::newTab);
    file->addAction(QIcon::fromTheme("tab-close"), "Close Tab\tCtrl+W", this, &MainWindow::closeCurrentTab);
    file->addSeparator();
    actPrefs = file->addAction(QIcon::fromTheme("configure"), "Preferences…");
    connect(actPrefs, &QAction::triggered, this, &MainWindow::openPreferences);

    auto edit = menuBar()->addMenu("&Edit");
    edit->addAction("Cut", [this]{ if (auto p=currentPane()) p->cutSelected(); }, QKeySequence::Cut);
    edit->addAction("Copy", [this]{ if (auto p=currentPane()) p->copySelected(); }, QKeySequence::Copy);
    edit->addAction("Paste", [this]{ if (auto p=currentPane()) p->pasteFiles(); }, QKeySequence::Paste);
    edit->addSeparator();
    edit->addAction("Delete", [this]{ if (auto p=currentPane()) p->deleteSelected(); }, QKeySequence::Delete);
    edit->addAction("Rename", [this]{ if (auto p=currentPane()) p->renameSelected(); }, QKeySequence("F2"));
    edit->addSeparator();
    edit->addAction("New Folder", [this]{ if (auto p=currentPane()) p->createNewFolder(); }, QKeySequence("Ctrl+Shift+N"));

    auto view = menuBar()->addMenu("&View");
    view->addAction("Icons", this, &MainWindow::setViewIcons);
    view->addAction("Details", this, &MainWindow::setViewDetails);
    view->addAction("Compact", this, &MainWindow::setViewCompact);
    view->addAction("Miller Columns", this, &MainWindow::setViewMiller);
    view->addSeparator();
    
    // Sort options
    auto *sortMenu = view->addMenu("Sort By");
    sortMenu->addAction("Name", [this]{ if (auto p=currentPane()) p->setSortCriteria(0); });
    sortMenu->addAction("Size", [this]{ if (auto p=currentPane()) p->setSortCriteria(1); });
    sortMenu->addAction("Type", [this]{ if (auto p=currentPane()) p->setSortCriteria(2); });
    sortMenu->addAction("Date Modified", [this]{ if (auto p=currentPane()) p->setSortCriteria(3); });
    sortMenu->addSeparator();
    sortMenu->addAction("Ascending", [this]{ if (auto p=currentPane()) p->setSortOrder(Qt::AscendingOrder); });
    sortMenu->addAction("Descending", [this]{ if (auto p=currentPane()) p->setSortOrder(Qt::DescendingOrder); });
    
    view->addSeparator();

    actShowToolbar = view->addAction("Show Toolbar");
    actShowToolbar->setCheckable(true);
    actShowToolbar->setChecked(false);
    connect(actShowToolbar, &QAction::toggled, this, &MainWindow::toggleToolbar);

    actShowHidden = view->addAction("Show Hidden Files");
    actShowHidden->setCheckable(true);
    actShowHidden->setChecked(false);
    connect(actShowHidden, &QAction::toggled, this, [this](bool on){
        if (auto *p = currentPane()) p->setShowHiddenFiles(on);
    });

    view->addSeparator();
    actPreviewPane = view->addAction("Preview Pane");
    actPreviewPane->setCheckable(true);
    actPreviewPane->setChecked(false);
    connect(actPreviewPane, &QAction::toggled, this, [this](bool on){
        if (auto *p = currentPane()) p->setPreviewVisible(on);
    });

    view->addSeparator();
    actQuickLook = view->addAction("Quick Look\tSpace", this, &MainWindow::quickLook);

    auto go = menuBar()->addMenu("&Go");
    go->addAction("Back", [this]{ if (auto p=currentPane()) p->goBack(); })->setShortcut(QKeySequence("Alt+Left"));
    go->addAction("Forward", [this]{ if (auto p=currentPane()) p->goForward(); })->setShortcut(QKeySequence("Alt+Right"));
    go->addSeparator();
    go->addAction("Up", [this]{ if (auto p=currentPane()) p->goUp(); });
    go->addAction("Home", [this]{ if (auto p=currentPane()) p->goHome(); });
    
    go->addSeparator();
    
    // Stock locations
    QString homeDir = QDir::homePath();
    go->addAction("Desktop", [this, homeDir]{ if (auto p=currentPane()) p->setUrl(QUrl::fromLocalFile(homeDir + "/Desktop")); });
    go->addAction("Documents", [this, homeDir]{ if (auto p=currentPane()) p->setUrl(QUrl::fromLocalFile(homeDir + "/Documents")); });
    go->addAction("Downloads", [this, homeDir]{ if (auto p=currentPane()) p->setUrl(QUrl::fromLocalFile(homeDir + "/Downloads")); });
    go->addAction("Music", [this, homeDir]{ if (auto p=currentPane()) p->setUrl(QUrl::fromLocalFile(homeDir + "/Music")); });
    go->addAction("Pictures", [this, homeDir]{ if (auto p=currentPane()) p->setUrl(QUrl::fromLocalFile(homeDir + "/Pictures")); });
    go->addAction("Videos", [this, homeDir]{ if (auto p=currentPane()) p->setUrl(QUrl::fromLocalFile(homeDir + "/Videos")); });
    
    go->addSeparator();
    go->addAction("Root (/)", [this]{ if (auto p=currentPane()) p->setUrl(QUrl::fromLocalFile("/")); });
    go->addAction("Applications (/usr/share/applications)", [this]{ if (auto p=currentPane()) p->setUrl(QUrl::fromLocalFile("/usr/share/applications")); });

    auto tools = menuBar()->addMenu("&Tools");
    tools->addAction("Preferences…", this, &MainWindow::openPreferences);

    auto help = menuBar()->addMenu("&Help");
    QAction *actNotes = help->addAction("Patch Notes…");
    connect(actNotes, &QAction::triggered, this, []{
        QUrl u = QUrl::fromLocalFile("/opt/kmiller/PATCHNOTES-latest.md");
        QDesktopServices::openUrl(u);
    });
    help->addAction("About", this, &MainWindow::showAbout);
}
void MainWindow::addInitialTab(const QUrl &url) {
    Pane *p = new Pane(url, this);
    int idx = tabs->addTab(p, url.isLocalFile() ? QFileInfo(url.toLocalFile()).fileName() : url.toString());
    tabs->setCurrentIndex(idx);
    
    // Connect status updates
    connect(p, &Pane::statusChanged, this, &MainWindow::updateStatusBar);
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
void MainWindow::openPreferences() { 
    SettingsDialog dlg(this); 
    if (dlg.exec() == QDialog::Accepted) {
        loadSettings(); // Reload settings to sync UI
    }
}
void MainWindow::setViewIcons(){ if (auto p=currentPane()) p->setViewMode(0); }
void MainWindow::setViewDetails(){ if (auto p=currentPane()) p->setViewMode(1); }
void MainWindow::setViewCompact(){ if (auto p=currentPane()) p->setViewMode(2); }
void MainWindow::setViewMiller(){ if (auto p=currentPane()) p->setViewMode(3); }
void MainWindow::quickLook(){ if (auto p=currentPane()) p->quickLookSelected(); 
}

Pane* MainWindow::currentPane() const {
    return qobject_cast<Pane*>(tabs->currentWidget());
}

void MainWindow::updateStatusBar(int totalFiles, int selectedFiles) {
    if (selectedFiles == 0) {
        statusLabel->setText(QString("%1 items").arg(totalFiles));
    } else if (selectedFiles == 1) {
        statusLabel->setText(QString("%1 of %2 items selected").arg(selectedFiles).arg(totalFiles));
    } else {
        statusLabel->setText(QString("%1 of %2 items selected").arg(selectedFiles).arg(totalFiles));
    }
}

void MainWindow::loadSettings() {
    QSettings settings;
    
    // Load toolbar visibility
    bool showToolbar = settings.value("general/showToolbar", false).toBool();
    actShowToolbar->setChecked(showToolbar);
    tb->setVisible(showToolbar);
    
    // Load hidden files setting
    bool showHidden = settings.value("general/showHiddenFiles", false).toBool();
    actShowHidden->setChecked(showHidden);
    if (auto *p = currentPane()) {
        p->setShowHiddenFiles(showHidden);
    }
    
    // Load preview pane setting
    bool showPreview = settings.value("general/showPreviewPane", false).toBool();
    actPreviewPane->setChecked(showPreview);
    if (auto *p = currentPane()) {
        p->setPreviewVisible(showPreview);
    }
    
    // Load default view mode
    int defaultView = settings.value("general/defaultView", 0).toInt();
    if (auto *p = currentPane()) {
        p->setViewMode(defaultView);
    }
}

void MainWindow::saveSettings() {
    QSettings settings;
    
    // Save toolbar visibility
    settings.setValue("general/showToolbar", actShowToolbar->isChecked());
    
    // Save hidden files setting
    settings.setValue("general/showHiddenFiles", actShowHidden->isChecked());
    
    // Save preview pane setting
    settings.setValue("general/showPreviewPane", actPreviewPane->isChecked());
    
    // Save current view mode from active pane
    if (auto *p = currentPane()) {
        settings.setValue("general/defaultView", p->currentViewMode());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::showAbout() {
    QDialog dialog(this);
    dialog.setWindowTitle("About KMiller");
    dialog.setModal(true);
    dialog.resize(500, 400);
    
    auto *layout = new QVBoxLayout(&dialog);
    
    // Logo/Icon section
    auto *iconLabel = new QLabel();
    QPixmap icon = QIcon::fromTheme("folder").pixmap(64, 64);
    if (icon.isNull()) {
        // Fallback text icon
        iconLabel->setText("📁");
        iconLabel->setStyleSheet("font-size: 48px;");
    } else {
        iconLabel->setPixmap(icon);
    }
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);
    
    // Title and version
    auto *titleLabel = new QLabel(QString("<h2>KMiller %1</h2>").arg(KMILLER_VERSION_STR));
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // Description
    auto *descLabel = new QLabel(
        "<p align='center'><b>Finder-style File Manager</b></p>"
        "<p align='center'>A modern file manager with Miller columns, QuickLook preview, "
        "and comprehensive file operations.</p>"
    );
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
    
    layout->addSpacing(20);
    
    // Features
    auto *featuresLabel = new QLabel(
        "<p><b>Key Features:</b></p>"
        "<ul>"
        "<li>Miller column navigation with preview pane</li>"
        "<li>Multiple view modes (Icons, Details, Compact, Miller)</li>"
        "<li>QuickLook preview with file navigation</li>"
        "<li>Comprehensive file operations (copy, cut, paste, duplicate)</li>"
        "<li>Archive support (compress & extract)</li>"
        "<li>Smart file type detection and icons</li>"
        "<li>Customizable settings and preferences</li>"
        "<li>KDE Frameworks integration</li>"
        "</ul>"
    );
    featuresLabel->setWordWrap(true);
    layout->addWidget(featuresLabel);
    
    layout->addSpacing(20);
    
    // Credits and info
    auto *infoLabel = new QLabel(
        "<p align='center'><small>"
        "Built with Qt6 and KDE Frameworks<br/>"
        "System: Linux • Architecture: x86_64<br/>"
        "Installation: /opt/kmiller/versions/" + QString(KMILLER_VERSION_STR) + "/"
        "</small></p>"
    );
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    // Copyright
    auto *copyrightLabel = new QLabel(
        "<p align='center'><small>"
        "🤖 Generated with <a href='https://claude.ai/code'>Claude Code</a><br/>"
        "© 2025 KMiller File Manager"
        "</small></p>"
    );
    copyrightLabel->setOpenExternalLinks(true);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(copyrightLabel);
    
    // Close button
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::accept);
    layout->addWidget(buttonBox);
    
    dialog.exec();
}
