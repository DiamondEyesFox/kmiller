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

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent=nullptr);

private slots:
    void newTab();
    void closeCurrentTab();
    void toggleSplit();
    void placeActivated(const QUrl &url);

private:
    QSplitter *splitter;
    KFilePlacesModel *placesModel;
    KFilePlacesView *placesView;
    QWidget *rightContainer;
    QTabWidget *tabs;

    // Split support
    bool splitOn = false;
    Pane *leftPane = nullptr;
    Pane *rightPane = nullptr;

    // Toolbar
    QToolBar *tb;
    QAction *actNewTab;
    QAction *actCloseTab;
    QAction *actSplit;

    void addInitialTab(const QUrl &url);
    void rebuildRightArea();
};
