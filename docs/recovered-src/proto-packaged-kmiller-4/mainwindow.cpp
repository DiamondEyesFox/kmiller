#include "mainwindow.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QPixmap>
#include <QFile>
#include <QTextStream>
#include <poppler-qt6.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    splitter = new QSplitter(this);
    setCentralWidget(splitter);
    resize(1400, 800);

    // Start with the home directory column
    addColumn(QDir::homePath());

    // Quick Look dialog setup
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

void MainWindow::addColumn(const QString &path)
{
    QFileSystemModel *model = new QFileSystemModel(this);
    model->setRootPath(path);

    QListView *view = new QListView;
    view->setModel(model);
    view->setRootIndex(model->index(path));

    int colIndex = columnViews.size();
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this, colIndex](const QModelIndex &idx) {
                onItemSelected(idx, colIndex);
            });

    columnViews.append(view);
    columnModels.append(model);
    splitter->addWidget(view);
}

void MainWindow::removeColumnsAfter(int columnIndex)
{
    while (columnViews.size() > columnIndex + 1) {
        QListView *view = columnViews.takeLast();
        QFileSystemModel *model = columnModels.takeLast();
        splitter->widget(splitter->indexOf(view))->deleteLater();
        model->deleteLater();
    }
}

void MainWindow::onItemSelected(const QModelIndex &index, int columnIndex)
{
    QFileSystemModel *model = columnModels[columnIndex];
    if (model->isDir(index)) {
        removeColumnsAfter(columnIndex);
        addColumn(model->filePath(index));
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        for (auto *view : columnViews) {
            if (view->hasFocus()) {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(view->model());
                QModelIndex idx = view->currentIndex();
                if (!idx.isValid() || model->isDir(idx)) return;
                showQuickLook(model->filePath(idx));
                return;
            }
        }
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
