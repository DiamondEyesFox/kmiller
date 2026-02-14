#include "QuickLookDialog.h"
#include "Pane.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QImageReader>
#include <QPixmap>
#include <QScrollArea>
#include <QTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QShortcut>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QScreen>
#include <QGuiApplication>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <poppler-qt6.h>

QuickLookDialog::QuickLookDialog(Pane *parentPane) : QDialog(parentPane), pane(parentPane) {
    // Frameless, dark, minimal - like macOS Quick Look
    // Use Tool flag so it floats above main window but doesn't steal focus
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_ShowWithoutActivating, true);  // Don't steal focus when shown
    setModal(false);
    resize(800, 600);

    // Dark background with minimal padding
    setStyleSheet(
        "QuickLookDialog { background-color: #1e1e1e; border: 1px solid #3a3a3a; border-radius: 8px; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);

    // Filename at top - subtle, small
    filenameLabel = new QLabel(this);
    filenameLabel->setAlignment(Qt::AlignCenter);
    filenameLabel->setStyleSheet(
        "QLabel { color: #cccccc; background-color: #2a2a2a; padding: 6px; font-size: 12px; "
        "border-top-left-radius: 8px; border-top-right-radius: 8px; }"
    );
    layout->addWidget(filenameLabel);

    // Content stack
    stack = new QStackedWidget(this);
    stack->setStyleSheet("QStackedWidget { background-color: #1e1e1e; }");
    layout->addWidget(stack, 1);

    // Image view - no scroll, just centered label
    auto *imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("QLabel { background-color: #1e1e1e; }");
    stack->addWidget(imageLabel);  // 0

    // PDF view
    auto *pdfLabel = new QLabel;
    pdfLabel->setAlignment(Qt::AlignCenter);
    pdfLabel->setStyleSheet("QLabel { background-color: #1e1e1e; }");
    auto *pdfScroll = new QScrollArea;
    pdfScroll->setWidget(pdfLabel);
    pdfScroll->setWidgetResizable(true);
    pdfScroll->setStyleSheet("QScrollArea { background-color: #1e1e1e; border: none; }");
    stack->addWidget(pdfScroll);  // 1

    // Text view
    auto *textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; color: #e0e0e0; border: none; "
        "font-family: monospace; font-size: 13px; padding: 8px; }"
    );
    stack->addWidget(textEdit);  // 2

    // Media view (audio/video)
    auto *mediaPage = new QWidget;
    auto *mediaLayout = new QVBoxLayout(mediaPage);
    mediaLayout->setContentsMargins(12, 12, 12, 12);
    mediaLayout->setSpacing(8);

    videoWidget = new QVideoWidget(mediaPage);
    videoWidget->setMinimumHeight(300);
    videoWidget->setStyleSheet("background-color: #111111; border: 1px solid #3a3a3a; border-radius: 6px;");
    mediaLayout->addWidget(videoWidget, 1);

    mediaInfoLabel = new QLabel(mediaPage);
    mediaInfoLabel->setAlignment(Qt::AlignCenter);
    mediaInfoLabel->setWordWrap(true);
    mediaInfoLabel->setStyleSheet("QLabel { color: #bdbdbd; }");
    mediaLayout->addWidget(mediaInfoLabel);

    auto *controlsLayout = new QHBoxLayout;
    mediaPlayPauseButton = new QPushButton("Pause", mediaPage);
    mediaPlayPauseButton->setMinimumWidth(72);
    controlsLayout->addWidget(mediaPlayPauseButton);

    mediaSeekSlider = new QSlider(Qt::Horizontal, mediaPage);
    mediaSeekSlider->setRange(0, 0);
    controlsLayout->addWidget(mediaSeekSlider, 1);
    mediaLayout->addLayout(controlsLayout);

    stack->addWidget(mediaPage);  // 3

    // Empty/unsupported
    auto *emptyLabel = new QLabel("No preview available");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("QLabel { color: #888888; background-color: #1e1e1e; }");
    unsupportedLabel = emptyLabel;
    stack->addWidget(emptyLabel);  // 4

    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(0.8f);
    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setVideoOutput(videoWidget);

    connect(mediaPlayPauseButton, &QPushButton::clicked, this, &QuickLookDialog::toggleMediaPlayback);
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        mediaPlayPauseButton->setText(state == QMediaPlayer::PlayingState ? "Pause" : "Play");
    });
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!mediaSeekSlider->isSliderDown()) {
            mediaSeekSlider->setValue(static_cast<int>(position));
        }
    });
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        mediaSeekSlider->setRange(0, static_cast<int>(duration));
    });
    connect(mediaSeekSlider, &QSlider::sliderMoved, this, [this](int position) {
        mediaPlayer->setPosition(position);
    });
    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &errorText) {
        if (!errorText.isEmpty()) {
            showUnsupported(currentFilePath, errorText);
        } else {
            showUnsupported(currentFilePath, "Media playback error");
        }
    });

    // Keyboard shortcuts - Space/Esc to close, Up/Down to navigate
    escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &QDialog::close);

    spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(spaceShortcut, &QShortcut::activated, this, &QDialog::close);

    upShortcut = new QShortcut(QKeySequence(Qt::Key_Up), this);
    connect(upShortcut, &QShortcut::activated, this, &QuickLookDialog::navigatePrevious);

    downShortcut = new QShortcut(QKeySequence(Qt::Key_Down), this);
    connect(downShortcut, &QShortcut::activated, this, &QuickLookDialog::navigateNext);

    connect(this, &QDialog::finished, this, [this](int) {
        stopMedia();
    });
}

void QuickLookDialog::showImage(const QString &path) {
    auto *label = qobject_cast<QLabel*>(stack->widget(0));
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage img = reader.read();
    if (img.isNull()) { showUnsupported(path, "Unreadable image"); return; }

    // Scale to fit dialog while preserving aspect ratio
    int availableWidth = width() - 2;
    int availableHeight = height() - filenameLabel->height() - 2;

    QPixmap pixmap = QPixmap::fromImage(img);
    if (pixmap.width() > availableWidth || pixmap.height() > availableHeight) {
        pixmap = pixmap.scaled(availableWidth, availableHeight,
                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    label->setPixmap(pixmap);
    stack->setCurrentIndex(0);
}

void QuickLookDialog::showPdf(const QString &path) {
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
    if (!doc) { showUnsupported(path, "Unreadable PDF"); return; }
    doc->setRenderHint(Poppler::Document::Antialiasing);
    doc->setRenderHint(Poppler::Document::TextAntialiasing);
    std::unique_ptr<Poppler::Page> page(doc->page(0));
    if (!page) { showUnsupported(path, "Unreadable PDF page"); return; }

    const QImage img = page->renderToImage(150.0, 150.0);
    if (img.isNull()) { showUnsupported(path, "Failed to render PDF"); return; }
    auto *scroll = qobject_cast<QScrollArea*>(stack->widget(1));
    auto *label = qobject_cast<QLabel*>(scroll->widget());
    label->setPixmap(QPixmap::fromImage(img));
    label->adjustSize();
    stack->setCurrentIndex(1);
}

bool QuickLookDialog::showText(const QString &path) {
    auto *te = qobject_cast<QTextEdit*>(stack->widget(2));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    // Limit text preview to first 100KB
    QByteArray rawData = f.read(100 * 1024);
    if (rawData.contains('\0')) {
        return false;
    }
    QString text = QString::fromUtf8(rawData);
    if (!text.isEmpty()) {
        const int replacementCount = text.count(QChar::ReplacementCharacter);
        if (replacementCount > (text.size() / 20)) {
            return false;
        }
    }
    te->setPlainText(text);
    stack->setCurrentIndex(2);
    return true;
}

void QuickLookDialog::showMedia(const QString &path, bool isVideo) {
    QFileInfo fi(path);
    videoWidget->setVisible(isVideo);
    mediaInfoLabel->setText(
        isVideo
            ? QString("Video preview: %1").arg(fi.fileName())
            : QString("Audio preview: %1").arg(fi.fileName())
    );
    mediaSeekSlider->setRange(0, 0);
    mediaSeekSlider->setValue(0);
    mediaPlayer->setSource(QUrl::fromLocalFile(path));
    mediaPlayer->play();
    stack->setCurrentIndex(3);
}

void QuickLookDialog::stopMedia() {
    if (!mediaPlayer) return;
    mediaPlayer->stop();
    mediaPlayer->setSource(QUrl());
    if (mediaSeekSlider) {
        mediaSeekSlider->setRange(0, 0);
        mediaSeekSlider->setValue(0);
    }
}

void QuickLookDialog::showUnsupported(const QString &path, const QString &mime) {
    QFileInfo fi(path);
    QString name = fi.fileName().isEmpty() ? path : fi.fileName();
    QString typeText = mime.isEmpty() ? "unknown type" : mime;
    unsupportedLabel->setText(
        QString("No preview available for:\n%1\n\nType: %2").arg(name, typeText)
    );
    stack->setCurrentWidget(unsupportedLabel);
}

void QuickLookDialog::toggleMediaPlayback() {
    if (!mediaPlayer) return;
    if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        mediaPlayer->pause();
    } else {
        mediaPlayer->play();
    }
}

void QuickLookDialog::showFile(const QString &path) {
    currentFilePath = path;
    QFileInfo fi(path);
    filenameLabel->setText(fi.fileName());
    stopMedia();

    QMimeDatabase db;
    const auto mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    const QString mime = mt.name();
    const QString suffix = fi.suffix().toLower();

    const QStringList audioSuffixes = {"mp3", "m4a", "aac", "flac", "wav", "ogg", "opus", "oga", "wma"};
    const QStringList videoSuffixes = {"mp4", "m4v", "mkv", "webm", "mov", "avi", "wmv", "flv", "mpeg", "mpg"};
    const bool isAudio = mime.startsWith("audio/") || mime == "application/ogg" || audioSuffixes.contains(suffix);
    const bool isVideo = mime.startsWith("video/") || videoSuffixes.contains(suffix);

    if (mime.startsWith("image/")) {
        showImage(path);
    } else if (mime == "application/pdf") {
        showPdf(path);
    } else if (isVideo) {
        showMedia(path, true);
    } else if (isAudio) {
        showMedia(path, false);
    } else if (mime.startsWith("text/") || mime.contains("json") || mime.contains("xml") ||
               mime.contains("javascript") || mime.contains("x-shellscript")) {
        if (!showText(path)) {
            showUnsupported(path, mime);
        }
    } else {
        // Try as image, fallback to text
        QImageReader test(path);
        if (test.canRead()) {
            showImage(path);
        } else {
            if (!showText(path)) {
                showUnsupported(path, mime);
            }
        }
    }

    show();
    raise();
    // Don't call activateWindow() - let main window keep focus so arrow keys work
}

void QuickLookDialog::navigateNext() {
    if (currentFilePath.isEmpty() || !pane) return;

    QFileInfo fi(currentFilePath);
    QDir dir(fi.absolutePath());
    QStringList files = dir.entryList(QDir::Files, QDir::Name);
    int idx = files.indexOf(fi.fileName());

    if (idx >= 0 && idx < files.count() - 1) {
        showFile(dir.absoluteFilePath(files[idx + 1]));
    }
}

void QuickLookDialog::navigatePrevious() {
    if (currentFilePath.isEmpty() || !pane) return;

    QFileInfo fi(currentFilePath);
    QDir dir(fi.absolutePath());
    QStringList files = dir.entryList(QDir::Files, QDir::Name);
    int idx = files.indexOf(fi.fileName());

    if (idx > 0) {
        showFile(dir.absoluteFilePath(files[idx - 1]));
    }
}
