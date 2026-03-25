#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    splitter = new QSplitter(this);

    // Models
    modelLeft = new QFileSystemModel(this);
    modelMiddle = new QFileSystemModel(this);
    modelRight = new QFileSystemModel(this);

    modelLeft->setRootPath(QDir::rootPath());
    modelMiddle->setRootPath(QDir::rootPath());
    modelRight->setRootPath(QDir::rootPath());

    // Views
    leftView = new QListView;
    middleView = new QListView;
    rightView = new QListView;

    leftView->setModel(modelLeft);
    middleView->setModel(modelMiddle);
    rightView->setModel(modelRight);

    leftView->setRootIndex(modelLeft->index(QDir::homePath()));
    middleView->setRootIndex(modelMiddle->index(QDir::homePath()));
    rightView->setRootIndex(modelRight->index(QDir::homePath()));

    splitter->addWidget(leftView);
    splitter->addWidget(middleView);
    splitter->addWidget(rightView);

    setCentralWidget(splitter);
    resize(1200, 600);

    // Connections
    connect(leftView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onLeftSelectionChanged);

    connect(middleView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onMiddleSelectionChanged);
}

void MainWindow::onLeftSelectionChanged(const QModelIndex &index)
{
    if (modelLeft->isDir(index)) {
        middleView->setRootIndex(modelMiddle->setRootPath(modelLeft->filePath(index)));
    }
}

void MainWindow::onMiddleSelectionChanged(const QModelIndex &index)
{
    if (modelMiddle->isDir(index)) {
        rightView->setRootIndex(modelRight->setRootPath(modelMiddle->filePath(index)));
    }
}
