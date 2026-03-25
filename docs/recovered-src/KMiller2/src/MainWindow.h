#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <KFilePlacesView>
#include <KFilePlacesModel>

class TabPage;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent=nullptr);

private slots:
    void newTab();
    void closeCurrentTab();
    void placeActivated(const QUrl &url);
    void updateWindowTitle(const QString &path);

private:
    QSplitter *splitter;
    KFilePlacesModel *placesModel;
    KFilePlacesView *placesView;
    QTabWidget *tabs;

    void addInitialTab(const QString &path);
};
