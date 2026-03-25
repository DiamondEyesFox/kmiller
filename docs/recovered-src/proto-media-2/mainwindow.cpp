#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPixmap>
#include <QFile>
#include <QTextStream>
#include <poppler-qt6.h>

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

    // Preview dialog setup
    previewDialog = new QDialog(this);
    previewDialog->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    previewDialog->setModal(false);
    previewDialog->resize(800, 600);

    QVBoxLayout *layout = new QVBoxLayout(previewDialog);
    imageLabel = new QLabel;
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

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        QListView *focusedView = qobject_cast<QListView*>(focusWidget());
        if (!focusedView) return;

        QFileSystemModel *model = qobject_cast<QFileSystemModel*>(focusedView->model());
        QModelIndex idx = focusedView->currentIndex();
        if (!idx.isValid() || model->isDir(idx)) return;

        QString path = model->filePath(idx);
        showQuickLook(path);
    }
    else if (event->key() == Qt::Key_Escape) {
        if (previewDialog->isVisible()) {
            mediaPlayer->stop();
            previewDialog->hide();
        }
    }
}

void MainWindow::showQuickLook(const QString &filePath)
{
    imageLabel->hide();
    textViewer->hide();
    videoWidget->hide();
    mediaPlayer->stop();

    if (filePath.endsWith(".png", Qt::CaseInsensitive) ||
        filePath.endsWith(".jpg", Qt::CaseInsensitive) ||
        filePath.endsWith(".jpeg", Qt::CaseInsensitive) ||
        filePath.endsWith(".bmp", Qt::CaseInsensitive)) {
        
        QPixmap pix(filePath);
        imageLabel->setPixmap(pix.scaled(previewDialog->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        imageLabel->show();
    }
    else if (filePath.endsWith(".pdf", Qt::CaseInsensitive)) {
        std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(filePath));
        if (doc && doc->numPages() > 0) {
            Poppler::Page *page = doc->page(0);
            QImage img = page->renderToImage(150, 150);
            delete page;
            imageLabel->setPixmap(QPixmap::fromImage(img).scaled(previewDialog->size(), Qt::KeepAspectRatio));
            imageLabel->show();
        }
    }
    else if (filePath.endsWith(".txt", Qt::CaseInsensitive) ||
             filePath.endsWith(".cpp", Qt::CaseInsensitive) ||
             filePath.endsWith(".h", Qt::CaseInsensitive) ||
             filePath.endsWith(".md", Qt::CaseInsensitive)) {

        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            textViewer->setPlainText(in.readAll());
            textViewer->show();
        }
    }
    else if (filePath.endsWith(".mp4", Qt::CaseInsensitive) ||
             filePath.endsWith(".mkv", Qt::CaseInsensitive) ||
             filePath.endsWith(".avi", Qt::CaseInsensitive) ||
             filePath.endsWith(".mp3", Qt::CaseInsensitive) ||
             filePath.endsWith(".wav", Qt::CaseInsensitive) ||
             filePath.endsWith(".flac", Qt::CaseInsensitive)) {

        mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        mediaPlayer->play();

        // Only show video widget for video files
        if (filePath.endsWith(".mp4", Qt::CaseInsensitive) ||
            filePath.endsWith(".mkv", Qt::CaseInsensitive) ||
            filePath.endsWith(".avi", Qt::CaseInsensitive)) {
            videoWidget->show();
        }
    }

    previewDialog->show();
    previewDialog->raise();
    previewDialog->activateWindow();
}
