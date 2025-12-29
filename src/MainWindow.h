#pragma once
#include <QMainWindow>
class QAction;
class QMenu;
class QStatusBar;
class QLabel;
class QSlider;
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
    
    // Public getters for current state (for SettingsDialog)
    bool isToolbarVisible() const { return actShowToolbar->isChecked(); }
    bool isPreviewPaneVisible() const { return actPreviewPane->isChecked(); }
    bool areHiddenFilesVisible() const { return actShowHidden->isChecked(); }
    int getCurrentTheme() const { return currentTheme; }
    int getCurrentViewMode() const;

public slots:
    void updateStatusBar(int totalFiles, int selectedFiles, qint64 selectedSize = 0);

protected:
    void closeEvent(QCloseEvent *event) override;

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
    void showAbout();
    void goToFolder();
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
    QSlider *globalZoomSlider;
    
    // Current theme tracking
    int currentTheme = 0;

    void addInitialTab(const QUrl &url);
    Pane* currentPane() const;
    void buildMenus();
    void loadSettings();
    void saveSettings();
    void applyTheme(int theme);
};
