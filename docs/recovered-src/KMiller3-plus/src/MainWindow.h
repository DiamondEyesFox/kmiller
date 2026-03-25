#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QAction>
#include <QComboBox>
#include <QSlider>
#include <KFilePlacesView>
#include <KFilePlacesModel>

class Pane;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent=nullptr);

private slots:
    void newTab();
    void closeCurrentTab();
    void placeActivated(const QUrl &url);
    void toggleToolbar(bool on);
    void openPreferences();

    // View mode slots
    void setViewIcons();
    void setViewDetails();
    void setViewCompact();
    void setViewMiller();

private:
    QSplitter *splitter;
    KFilePlacesModel *placesModel;
    KFilePlacesView *placesView;
    QWidget *rightContainer;
    QTabWidget *tabs;

    // Toolbar
    QToolBar *tb;
    QAction *actNewTab;
    QAction *actCloseTab;

    // Menus
    QAction *actShowToolbar;
    QAction *actPrefs;

    // Helpers
    void addInitialTab(const QUrl &url);
    Pane* currentPane() const;
    void buildMenus();
};
