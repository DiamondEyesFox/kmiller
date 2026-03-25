#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QFileSystemModel>
#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onItemSelected(const QModelIndex &index, int columnIndex);
    void showQuickLook(const QString &filePath);

private:
    QSplitter *splitter;
    QList<QListView*> columnViews;
    QList<QFileSystemModel*> columnModels;

    QDialog *previewDialog;
    QLabel *imageLabel;
    QTextEdit *textViewer;
    QMediaPlayer *mediaPlayer;
    QVideoWidget *videoWidget;
    QAudioOutput *audioOutput;

    void addColumn(const QString &path);
    void removeColumnsAfter(int columnIndex);
};
