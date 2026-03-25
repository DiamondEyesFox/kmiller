#pragma once

#include <QMainWindow>
#include <QListView>
#include <QSplitter>
#include <QFileSystemModel>
#include <QDialog>
#include <QLabel>
#include <QTextEdit>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onLeftSelectionChanged(const QModelIndex &index);
    void onMiddleSelectionChanged(const QModelIndex &index);
    void showQuickLook(const QString &filePath);

private:
    QSplitter *splitter;
    QListView *leftView;
    QListView *middleView;
    QListView *rightView;

    QFileSystemModel *modelLeft;
    QFileSystemModel *modelMiddle;
    QFileSystemModel *modelRight;

    QDialog *previewDialog;
    QLabel *imageLabel;
    QTextEdit *textViewer;
};
