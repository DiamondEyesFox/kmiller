#include "QuickLookDialog.h"
#include <QVBoxLayout>
#include <QPixmap>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <poppler-qt6.h>

QuickLookDialog::QuickLookDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setModal(false);
    resize(900, 640);

    auto *layout = new QVBoxLayout(this);

    imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);

    textViewer = new QTextEdit;
    textViewer->setReadOnly(true);

    videoWidget = new QVideoWidget;
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setVideoOutput(videoWidget);

    layout->addWidget(imageLabel);
    layout->addWidget(textViewer);
    layout->addWidget(videoWidget);

    imageLabel->hide();
    textViewer->hide();
    videoWidget->hide();
}

void QuickLookDialog::showFile(const QString &filePath) {
    imageLabel->hide();
    textViewer->hide();
    videoWidget->hide();
    mediaPlayer->stop();

    const QString f = filePath.toLower();

    // Images
    if (f.endsWith(".png") || f.endsWith(".jpg") || f.endsWith(".jpeg")
        || f.endsWith(".bmp") || f.endsWith(".webp") || f.endsWith(".gif")) {
        QPixmap pix(filePath);
        imageLabel->setPixmap(pix.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        imageLabel->show();
    }
    // PDFs (first page)
    else if (f.endsWith(".pdf")) {
        std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(filePath));
        if (doc && doc->numPages() > 0) {
            auto page = doc->page(0);
            if (page) {
                QImage img = page->renderToImage(150, 150);
                imageLabel->setPixmap(QPixmap::fromImage(img).scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                imageLabel->show();
            }
        }
    }
    // Text / code
    else if (f.endsWith(".txt") || f.endsWith(".cpp") || f.endsWith(".h")
             || f.endsWith(".md") || f.endsWith(".json") || f.endsWith(".log")
             || f.endsWith(".ini") || f.endsWith(".conf") || f.endsWith(".yaml") || f.endsWith(".yml")) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            textViewer->setPlainText(in.readAll());
            textViewer->show();
        }
    }
    // Media (audio/video)
    else if (f.endsWith(".mp4") || f.endsWith(".mkv") || f.endsWith(".avi") || f.endsWith(".mov")
             || f.endsWith(".mp3") || f.endsWith(".wav") || f.endsWith(".flac") || f.endsWith(".ogg")) {
        mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        mediaPlayer->play();
        // Show video widget for video filetypes
        if (f.endsWith(".mp4") || f.endsWith(".mkv") || f.endsWith(".avi") || f.endsWith(".mov")) {
            videoWidget->show();
        }
    }

    show();
    raise();
    activateWindow();
}
