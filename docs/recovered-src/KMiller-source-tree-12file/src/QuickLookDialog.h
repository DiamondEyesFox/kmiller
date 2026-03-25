#pragma once
#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>

class QuickLookDialog : public QDialog {
    Q_OBJECT
public:
    explicit QuickLookDialog(QWidget *parent=nullptr);
    void showFile(const QString &filePath);

private:
    QLabel *imageLabel;
    QTextEdit *textViewer;
    QVideoWidget *videoWidget;
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
};
