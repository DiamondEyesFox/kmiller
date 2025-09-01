#pragma once
#include <QMainWindow>
class QAction;
class QMenu;
class QStatusBar;
class QLabel;
#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QAction>
#include <KFilePlacesView>
#include <KFilePlacesModel>

class Pane;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent=nullptr);
public slots:
    void updateStatusBar(int totalFiles, int selectedFiles);

private slots:
    void newTab();
    void closeCurrentTab();
    void placeActivated(const QUrl &url);
    void toggleToolbar(bool on);
    void openPreferences();
    void setViewIcons();
    void setViewDetails();
    void setViewCompact();
    void setViewMiller();
    void quickLook();
private:
    QAction *actPreviewPane = nullptr;

    QSplitter *splitter;
    KFilePlacesModel *placesModel;
    KFilePlacesView *placesView;
    QWidget *rightContainer;
    QTabWidget *tabs;
    QToolBar *tb;
    QAction *actNewTab;
    QAction *actCloseTab;
    QAction *actShowToolbar;
    QAction *actShowHidden;
    QAction *actPrefs;
    QAction *actQuickLook;
    
    // Status bar
    QLabel *statusLabel;

    void addInitialTab(const QUrl &url);
    Pane* currentPane() const;
    void buildMenus();
    void loadSettings();
};
