#pragma once

#include <QMainWindow>
#include <QListView>
#include <QSplitter>
#include <QFileSystemModel>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onLeftSelectionChanged(const QModelIndex &index);
    void onMiddleSelectionChanged(const QModelIndex &index);

private:
    QSplitter *splitter;
    QListView *leftView;
    QListView *middleView;
    QListView *rightView;

    QFileSystemModel *modelLeft;
    QFileSystemModel *modelMiddle;
    QFileSystemModel *modelRight;
};
