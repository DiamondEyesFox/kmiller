#include "QuickLookDialog.h"
#include "Pane.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QImage>
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
#include <QDirIterator>
#include <QUrl>
#include <QSizePolicy>
#include <QRegularExpression>
#include <QDate>
#include <QDateTime>
#include <QFutureWatcher>
#include <QIcon>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QStyle>
#include <QtConcurrent/QtConcurrentRun>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QAudioOutput>
#include <QVideoWidget>
#include <poppler-qt6.h>
#include <algorithm>

static QString formatBytes(qint64 size) {
    const QStringList units = {"bytes", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(qMax<qint64>(0, size));
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < units.size() - 1) {
        value /= 1024.0;
        ++unitIndex;
    }

    if (unitIndex == 0) {
        return QString("%1 %2").arg(static_cast<qint64>(value)).arg(units[unitIndex]);
    }
    return QString("%1 %2").arg(value, 0, 'f', 1).arg(units[unitIndex]);
}

static qint64 computeDirectorySizeBytes(const QString &dirPath) {
    qint64 totalSize = 0;
    QDirIterator it(
        dirPath,
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden | QDir::System,
        QDirIterator::Subdirectories
    );

    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        if (fi.isFile()) {
            totalSize += fi.size();
        }
    }
    return totalSize;
}

static QString formatDurationMs(qint64 ms) {
    if (ms <= 0) return "0:00";
    const qint64 totalSeconds = ms / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
    return QString("%1:%2")
        .arg(minutes)
        .arg(seconds, 2, 10, QChar('0'));
}

QuickLookDialog::QuickLookDialog(Pane *parentPane) : QDialog(parentPane), pane(parentPane) {
    // Frameless, dark, minimal - like macOS Quick Look
    // Use a regular top-level window so stacking/focus is reliable across launch contexts.
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setModal(false);
    resize(800, 600);

    // Finder-inspired quick-look chrome.
    setStyleSheet(
        "QuickLookDialog { background-color: #20242b; border: 1px solid #434956; border-radius: 12px; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);

    // Filename at top - subtle, small
    filenameLabel = new QLabel(this);
    filenameLabel->setAlignment(Qt::AlignCenter);
    filenameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    filenameLabel->setStyleSheet(
        "QLabel { color: #eef1f6; "
        "background-color: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #303543, stop:1 #282d39); "
        "padding: 7px 12px; font-size: 12px; font-weight: 600; "
        "border-top-left-radius: 12px; border-top-right-radius: 12px; border-bottom: 1px solid #3f4655; }"
    );
    layout->addWidget(filenameLabel);

    // Content stack
    stack = new QStackedWidget(this);
    stack->setStyleSheet("QStackedWidget { background-color: #1f2127; }");
    layout->addWidget(stack, 1);

    // Image view - no scroll, just centered label
    auto *imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("QLabel { background-color: #1f2127; }");
    stack->addWidget(imageLabel);  // 0

    // PDF view
    auto *pdfLabel = new QLabel;
    pdfLabel->setAlignment(Qt::AlignCenter);
    pdfLabel->setStyleSheet("QLabel { background-color: #1f2127; }");
    auto *pdfScroll = new QScrollArea;
    pdfScroll->setWidget(pdfLabel);
    pdfScroll->setWidgetResizable(true);
    pdfScroll->setStyleSheet("QScrollArea { background-color: #1f2127; border: none; }");
    stack->addWidget(pdfScroll);  // 1

    // Text view
    auto *textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setStyleSheet(
        "QTextEdit { background-color: #1f2127; color: #e5e8ed; border: none; "
        "font-family: monospace; font-size: 13px; padding: 10px; }"
    );
    stack->addWidget(textEdit);  // 2

    // Media view (audio/video)
    auto *mediaPage = new QWidget;
    auto *mediaLayout = new QVBoxLayout(mediaPage);
    mediaLayout->setContentsMargins(12, 12, 12, 12);
    mediaLayout->setSpacing(10);

    mediaContentStack = new QStackedWidget(mediaPage);
    mediaContentStack->setStyleSheet(
        "QStackedWidget { background-color: #14161b; border: 1px solid #383c45; border-radius: 12px; }"
    );
    mediaLayout->addWidget(mediaContentStack, 1);

    videoWidget = new QVideoWidget(mediaContentStack);
    videoWidget->setMinimumHeight(300);
    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoWidget->setStyleSheet("background-color: #14161b; border-radius: 9px;");
    mediaContentStack->addWidget(videoWidget);

    audioPanel = new QWidget(mediaContentStack);
    auto *audioPanelLayout = new QHBoxLayout(audioPanel);
    audioPanelLayout->setContentsMargins(24, 24, 24, 24);
    audioPanelLayout->setSpacing(18);
    audioPanelLayout->setAlignment(Qt::AlignCenter);

    audioArtworkLabel = new QLabel(audioPanel);
    audioArtworkLabel->setFixedSize(240, 240);
    audioArtworkLabel->setAlignment(Qt::AlignCenter);
    audioArtworkLabel->setStyleSheet(
        "QLabel { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2a2f38, stop:1 #1a1d23); "
        "border: 1px solid #3a3f49; border-radius: 14px; }"
    );
    audioPanelLayout->addWidget(audioArtworkLabel, 0, Qt::AlignVCenter);

    auto *audioMetaCard = new QWidget(audioPanel);
    audioMetaCard->setObjectName("audioMetaCard");
    audioMetaCard->setMinimumWidth(0);
    audioMetaCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    audioMetaCard->setStyleSheet(
        "#audioMetaCard { background-color: #1f2229; border: 1px solid #393d47; border-radius: 12px; }"
    );
    auto *audioMetaLayout = new QVBoxLayout(audioMetaCard);
    audioMetaLayout->setContentsMargins(20, 16, 20, 16);
    audioMetaLayout->setSpacing(8);
    audioMetaLayout->setAlignment(Qt::AlignVCenter);

    audioTitleLabel = new QLabel(audioMetaCard);
    audioTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    audioTitleLabel->setWordWrap(true);
    audioTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    audioTitleLabel->setStyleSheet("QLabel { color: #f3f5f9; font-size: 22px; font-weight: 700; }");
    audioMetaLayout->addWidget(audioTitleLabel);

    audioArtistLabel = new QLabel(audioMetaCard);
    audioArtistLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    audioArtistLabel->setWordWrap(true);
    audioArtistLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    audioArtistLabel->setStyleSheet("QLabel { color: #b6c7ff; font-size: 16px; font-weight: 600; }");
    audioMetaLayout->addWidget(audioArtistLabel);

    audioAlbumLabel = new QLabel(audioMetaCard);
    audioAlbumLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    audioAlbumLabel->setWordWrap(true);
    audioAlbumLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    audioAlbumLabel->setStyleSheet("QLabel { color: #cfd4dc; font-size: 14px; }");
    audioMetaLayout->addWidget(audioAlbumLabel);

    audioYearLabel = new QLabel(audioMetaCard);
    audioYearLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    audioYearLabel->setStyleSheet(
        "QLabel { color: #d7dbe2; font-size: 12px; background-color: #2c313a; border: 1px solid #47505f; "
        "border-radius: 10px; padding: 2px 10px; }"
    );
    audioMetaLayout->addWidget(audioYearLabel, 0, Qt::AlignLeft);
    audioMetaLayout->addStretch(1);

    audioPanelLayout->addWidget(audioMetaCard, 1, Qt::AlignVCenter);
    mediaContentStack->addWidget(audioPanel);

    mediaInfoLabel = new QLabel(mediaPage);
    mediaInfoLabel->setAlignment(Qt::AlignCenter);
    mediaInfoLabel->setWordWrap(true);
    mediaInfoLabel->setStyleSheet("QLabel { color: #bac0cb; font-size: 11px; }");
    mediaLayout->addWidget(mediaInfoLabel);

    auto *controlsLayout = new QHBoxLayout;
    controlsLayout->setContentsMargins(4, 2, 4, 0);
    controlsLayout->setSpacing(10);
    mediaPlayPauseButton = new QPushButton(mediaPage);
    mediaPlayPauseButton->setMinimumSize(36, 36);
    mediaPlayPauseButton->setMaximumSize(36, 36);
    mediaPlayPauseButton->setIconSize(QSize(18, 18));
    mediaPlayPauseButton->setToolTip("Pause");
    mediaPlayPauseButton->setStyleSheet(
        "QPushButton { background-color: #303642; color: #e7eaf0; border: 1px solid #505a6d; border-radius: 18px; padding: 0px; }"
        "QPushButton:hover { background-color: #3a4250; }"
        "QPushButton:pressed { background-color: #444d5d; }"
    );
    mediaPlayPauseButton->setIcon(QIcon::fromTheme("media-playback-pause", style()->standardIcon(QStyle::SP_MediaPause)));
    controlsLayout->addWidget(mediaPlayPauseButton);

    mediaCurrentTimeLabel = new QLabel("0:00", mediaPage);
    mediaCurrentTimeLabel->setMinimumWidth(44);
    mediaCurrentTimeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mediaCurrentTimeLabel->setStyleSheet("QLabel { color: #c9ced7; font-size: 11px; }");
    controlsLayout->addWidget(mediaCurrentTimeLabel);

    mediaSeekSlider = new QSlider(Qt::Horizontal, mediaPage);
    mediaSeekSlider->setRange(0, 0);
    mediaSeekSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; background: #414754; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #b8c7e6; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 12px; margin: -4px 0; border-radius: 6px; background: #eceff5; border: 1px solid #4d5566; }"
        "QSlider::handle:horizontal:hover { background: #ffffff; }"
    );
    controlsLayout->addWidget(mediaSeekSlider, 1);

    mediaTotalTimeLabel = new QLabel("0:00", mediaPage);
    mediaTotalTimeLabel->setMinimumWidth(44);
    mediaTotalTimeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mediaTotalTimeLabel->setStyleSheet("QLabel { color: #c9ced7; font-size: 11px; }");
    controlsLayout->addWidget(mediaTotalTimeLabel);

    mediaMuteButton = new QPushButton(mediaPage);
    mediaMuteButton->setMinimumSize(28, 28);
    mediaMuteButton->setMaximumSize(28, 28);
    mediaMuteButton->setIconSize(QSize(15, 15));
    mediaMuteButton->setToolTip("Mute");
    mediaMuteButton->setStyleSheet(
        "QPushButton { background-color: #2a2f39; color: #dfe4ed; border: 1px solid #4b5567; border-radius: 14px; padding: 0px; }"
        "QPushButton:hover { background-color: #353d4a; }"
        "QPushButton:pressed { background-color: #3f4858; }"
    );
    controlsLayout->addWidget(mediaMuteButton);

    mediaVolumeSlider = new QSlider(Qt::Horizontal, mediaPage);
    mediaVolumeSlider->setRange(0, 100);
    mediaVolumeSlider->setFixedWidth(92);
    mediaVolumeSlider->setValue(80);
    mediaVolumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 3px; background: #3f4654; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #b4c3e4; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 10px; margin: -4px 0; border-radius: 5px; background: #eff2f8; border: 1px solid #586173; }"
        "QSlider::handle:horizontal:hover { background: #ffffff; }"
    );
    controlsLayout->addWidget(mediaVolumeSlider);

    mediaLayout->addLayout(controlsLayout);

    stack->addWidget(mediaPage);  // 3

    // Empty/unsupported
    auto *emptyLabel = new QLabel("No preview available");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);
    emptyLabel->setMinimumWidth(420);
    emptyLabel->setStyleSheet(
        "QLabel { color: #d2d7df; background-color: #242831; border: 1px solid #3b404a; "
        "border-radius: 12px; padding: 22px; margin: 26px; font-size: 14px; }"
    );
    unsupportedLabel = emptyLabel;
    stack->addWidget(emptyLabel);  // 4

    // Folder preview page (Finder-like quick summary card)
    folderPage = new QWidget;
    auto *folderLayout = new QHBoxLayout(folderPage);
    folderLayout->setContentsMargins(24, 24, 24, 24);
    folderLayout->setSpacing(20);
    folderLayout->setAlignment(Qt::AlignCenter);

    folderIconLabel = new QLabel(folderPage);
    folderIconLabel->setFixedSize(240, 240);
    folderIconLabel->setAlignment(Qt::AlignCenter);
    folderIconLabel->setStyleSheet(
        "QLabel { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #2a3038, stop:1 #20242b); "
        "border: 1px solid #3a424e; border-radius: 16px; }"
    );
    folderLayout->addWidget(folderIconLabel, 0, Qt::AlignVCenter);

    auto *folderMetaCard = new QWidget(folderPage);
    folderMetaCard->setObjectName("folderMetaCard");
    folderMetaCard->setStyleSheet(
        "#folderMetaCard { background-color: #1f2229; border: 1px solid #393d47; border-radius: 12px; }"
    );
    folderMetaCard->setMinimumWidth(420);
    folderMetaCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *folderMetaLayout = new QVBoxLayout(folderMetaCard);
    folderMetaLayout->setContentsMargins(20, 16, 20, 16);
    folderMetaLayout->setSpacing(10);
    folderMetaLayout->setAlignment(Qt::AlignVCenter);

    folderNameLabel = new QLabel(folderMetaCard);
    folderNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    folderNameLabel->setWordWrap(true);
    folderNameLabel->setStyleSheet("QLabel { color: #f3f5f9; font-size: 24px; font-weight: 700; border: none; background: transparent; }");
    folderMetaLayout->addWidget(folderNameLabel);

    folderInfoLabel = new QLabel(folderMetaCard);
    folderInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    folderInfoLabel->setWordWrap(true);
    folderInfoLabel->setStyleSheet("QLabel { color: #cfd4dc; font-size: 14px; border: none; background: transparent; }");
    folderMetaLayout->addWidget(folderInfoLabel);

    folderPathLabel = new QLabel(folderMetaCard);
    folderPathLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    folderPathLabel->setWordWrap(true);
    folderPathLabel->setStyleSheet("QLabel { color: #a8b0bd; font-size: 12px; border: none; background: transparent; }");
    folderMetaLayout->addWidget(folderPathLabel);

    folderMetaLayout->addStretch(1);
    folderLayout->addWidget(folderMetaCard, 1, Qt::AlignVCenter);
    stack->addWidget(folderPage);  // 5

    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(0.8f);
    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setVideoOutput(videoWidget);

    connect(mediaPlayPauseButton, &QPushButton::clicked, this, &QuickLookDialog::toggleMediaPlayback);
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        const bool isPlaying = (state == QMediaPlayer::PlayingState);
        const QIcon icon = isPlaying
            ? QIcon::fromTheme("media-playback-pause", style()->standardIcon(QStyle::SP_MediaPause))
            : QIcon::fromTheme("media-playback-start", style()->standardIcon(QStyle::SP_MediaPlay));
        mediaPlayPauseButton->setIcon(icon);
        mediaPlayPauseButton->setToolTip(isPlaying ? "Pause" : "Play");
    });
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!mediaSeekSlider->isSliderDown()) {
            mediaSeekSlider->setValue(static_cast<int>(position));
        }
        if (mediaCurrentTimeLabel) {
            mediaCurrentTimeLabel->setText(formatDurationMs(position));
        }
    });
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        mediaSeekSlider->setRange(0, static_cast<int>(duration));
        if (mediaTotalTimeLabel) {
            mediaTotalTimeLabel->setText(formatDurationMs(duration));
        }
    });
    connect(mediaPlayer, &QMediaPlayer::metaDataChanged, this, &QuickLookDialog::updateAudioMetadata);
    connect(mediaSeekSlider, &QSlider::sliderMoved, this, [this](int position) {
        mediaPlayer->setPosition(position);
    });

    auto updateVolumeUi = [this]() {
        if (!audioOutput || !mediaMuteButton || !mediaVolumeSlider) return;

        const qreal volume = std::clamp(static_cast<double>(audioOutput->volume()), 0.0, 1.0);
        const bool muted = audioOutput->isMuted() || volume <= 0.001;
        if (!mediaVolumeSlider->isSliderDown()) {
            mediaVolumeSlider->setValue(static_cast<int>(volume * 100.0 + 0.5));
        }

        QIcon icon = QIcon::fromTheme(muted ? "audio-volume-muted"
                                    : (volume < 0.5 ? "audio-volume-low" : "audio-volume-high"));
        if (!icon.isNull()) {
            mediaMuteButton->setIcon(icon);
            mediaMuteButton->setText(QString());
        } else {
            mediaMuteButton->setIcon(QIcon());
            mediaMuteButton->setText(muted ? "M" : "V");
        }
        mediaMuteButton->setToolTip(muted ? "Unmute" : "Mute");
    };

    connect(mediaMuteButton, &QPushButton::clicked, this, [this, updateVolumeUi]() {
        if (!audioOutput) return;
        const bool newMuted = !audioOutput->isMuted();
        audioOutput->setMuted(newMuted);
        if (!newMuted && audioOutput->volume() <= 0.001) {
            audioOutput->setVolume(0.8f);
        }
        updateVolumeUi();
    });
    connect(mediaVolumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (!audioOutput) return;
        const qreal volume = std::clamp(static_cast<double>(value) / 100.0, 0.0, 1.0);
        audioOutput->setVolume(volume);
        audioOutput->setMuted(value == 0);
    });
    connect(audioOutput, &QAudioOutput::volumeChanged, this, [updateVolumeUi](qreal) { updateVolumeUi(); });
    connect(audioOutput, &QAudioOutput::mutedChanged, this, [updateVolumeUi](bool) { updateVolumeUi(); });
    updateVolumeUi();

    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &errorText) {
        const QString activeRawPath = activeMediaSource.toLocalFile();
        const QString sourceRawPath = mediaPlayer->source().toLocalFile();
        if (activeRawPath.isEmpty() || sourceRawPath.isEmpty()) {
            return;
        }

        const QString activePath = QDir::cleanPath(activeRawPath);
        const QString sourcePath = QDir::cleanPath(sourceRawPath);
        if (sourcePath != activePath) {
            // Ignore stale errors emitted after source switches.
            return;
        }

        if (activeMediaIsVideo && !retriedVideoWithoutAudio) {
            // Some desktops/backends fail video startup when audio sink init fails.
            retriedVideoWithoutAudio = true;
            mediaPlayer->setAudioOutput(nullptr);
            mediaPlayer->setPosition(0);
            mediaPlayer->play();
            return;
        }

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

    // Left/Right behavior depends on view mode:
    // - Miller: forward to Miller for column navigation (left=back, right=drill in).
    //   Quick Look auto-updates via Miller's selectionChanged signal.
    // - Icon/Details/Compact: prev/next file in model order.
    // Left/Right navigate prev/next in classic views.
    // In Miller, left/right are column navigation — not supported from Quick Look
    // (close Quick Look first, navigate columns, then reopen).
    leftShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(leftShortcut, &QShortcut::activated, this, [this]{
        if (pane && !pane->isMillerViewActive()) navigatePrevious();
    });

    rightShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(rightShortcut, &QShortcut::activated, this, [this]{
        if (pane && !pane->isMillerViewActive()) navigateNext();
    });

    // YouTube-style transport keys.
    jShortcut = new QShortcut(QKeySequence(Qt::Key_J), this);
    connect(jShortcut, &QShortcut::activated, this, [this]() {
        if (!isVisible() || !mediaPlayer || mediaPlayer->duration() <= 0) return;
        mediaPlayer->setPosition(std::max<qint64>(0, mediaPlayer->position() - 10000));
    });

    kShortcut = new QShortcut(QKeySequence(Qt::Key_K), this);
    connect(kShortcut, &QShortcut::activated, this, [this]() {
        if (!isVisible() || !mediaPlayer || mediaPlayer->duration() <= 0) return;
        toggleMediaPlayback();
    });

    lShortcut = new QShortcut(QKeySequence(Qt::Key_L), this);
    connect(lShortcut, &QShortcut::activated, this, [this]() {
        if (!isVisible() || !mediaPlayer || mediaPlayer->duration() <= 0) return;
        const qint64 duration = mediaPlayer->duration();
        mediaPlayer->setPosition(std::min<qint64>(duration, mediaPlayer->position() + 10000));
    });

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

void QuickLookDialog::setAudioArtwork(const QImage &image) {
    if (!audioArtworkLabel) return;

    QPixmap artwork;
    if (!image.isNull()) {
        artwork = QPixmap::fromImage(image);
    } else {
        QIcon icon = QIcon::fromTheme("audio-x-generic");
        artwork = icon.pixmap(132, 132);
        if (artwork.isNull()) {
            artwork = QPixmap(132, 132);
            artwork.fill(Qt::transparent);
            QPainter p(&artwork);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#d6dde8"));
            p.drawEllipse(artwork.rect().adjusted(8, 8, -8, -8));
        }
    }

    if (!artwork.isNull()) {
        audioArtworkLabel->setPixmap(artwork.scaled(
            audioArtworkLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    } else {
        audioArtworkLabel->setPixmap(QPixmap());
    }
}

void QuickLookDialog::resetAudioMetadata(const QString &path) {
    QFileInfo fi(path);
    const QString fallbackTitle = !fi.completeBaseName().isEmpty() ? fi.completeBaseName() : fi.fileName();
    if (audioTitleLabel) {
        audioTitleLabel->setText(fallbackTitle.isEmpty() ? "Unknown Track" : fallbackTitle);
    }
    if (audioArtistLabel) {
        audioArtistLabel->setText("Unknown Artist");
    }
    if (audioAlbumLabel) {
        audioAlbumLabel->setText("Unknown Album");
    }
    if (audioYearLabel) {
        audioYearLabel->setText("Year Unknown");
    }
    setAudioArtwork(QImage());

    if (mediaInfoLabel) {
        mediaInfoLabel->setText(QString("Audio preview: %1").arg(fi.fileName()));
    }
}

void QuickLookDialog::updateAudioMetadata() {
    if (activeMediaIsVideo || currentFilePath.isEmpty() || !mediaPlayer) {
        return;
    }

    const QMediaMetaData md = mediaPlayer->metaData();
    QFileInfo fi(currentFilePath);

    QString title = md.stringValue(QMediaMetaData::Title).trimmed();
    if (title.isEmpty()) {
        title = fi.completeBaseName();
    }
    if (title.isEmpty()) {
        title = fi.fileName();
    }
    if (title.isEmpty()) {
        title = "Unknown Track";
    }

    QString artist;
    const QStringList contributingArtists = md.value(QMediaMetaData::ContributingArtist).toStringList();
    for (const QString &value : contributingArtists) {
        if (!value.trimmed().isEmpty()) {
            artist = value.trimmed();
            break;
        }
    }
    if (artist.isEmpty()) artist = md.stringValue(QMediaMetaData::AlbumArtist).trimmed();
    if (artist.isEmpty()) artist = md.stringValue(QMediaMetaData::Author).trimmed();
    if (artist.isEmpty()) artist = md.stringValue(QMediaMetaData::LeadPerformer).trimmed();
    if (artist.isEmpty()) artist = "Unknown Artist";

    QString album = md.stringValue(QMediaMetaData::AlbumTitle).trimmed();
    if (album.isEmpty()) {
        album = "Unknown Album";
    }

    QString year;
    const QVariant dateValue = md.value(QMediaMetaData::Date);
    if (dateValue.canConvert<QDateTime>()) {
        const QDate d = dateValue.toDateTime().date();
        if (d.isValid()) year = d.toString("yyyy");
    }
    if (year.isEmpty() && dateValue.canConvert<QDate>()) {
        const QDate d = dateValue.toDate();
        if (d.isValid()) year = d.toString("yyyy");
    }
    if (year.isEmpty()) {
        const QString dateText = dateValue.toString();
        const QRegularExpressionMatch m = QRegularExpression("(\\d{4})").match(dateText);
        if (m.hasMatch()) {
            year = m.captured(1);
        }
    }
    if (year.isEmpty()) {
        year = "Year Unknown";
    }

    if (audioTitleLabel) audioTitleLabel->setText(title);
    if (audioArtistLabel) audioArtistLabel->setText(artist);
    if (audioAlbumLabel) audioAlbumLabel->setText(album);
    if (audioYearLabel) audioYearLabel->setText(year);

    QImage artwork = md.value(QMediaMetaData::CoverArtImage).value<QImage>();
    if (artwork.isNull()) {
        artwork = md.value(QMediaMetaData::ThumbnailImage).value<QImage>();
    }
    if (!artwork.isNull()) {
        setAudioArtwork(artwork);
    }
}

void QuickLookDialog::showMedia(const QString &path, bool isVideo) {
    QFileInfo fi(path);
    activeMediaSource = QUrl::fromLocalFile(path);
    activeMediaIsVideo = isVideo;
    retriedVideoWithoutAudio = false;

    if (isVideo) {
        if (mediaContentStack && videoWidget) {
            mediaContentStack->setCurrentWidget(videoWidget);
        }
        if (mediaInfoLabel) {
            mediaInfoLabel->setText(QString("Video preview: %1").arg(fi.fileName()));
        }
    } else {
        if (mediaContentStack && audioPanel) {
            mediaContentStack->setCurrentWidget(audioPanel);
        }
        resetAudioMetadata(path);
    }

    mediaSeekSlider->setRange(0, 0);
    mediaSeekSlider->setValue(0);
    if (mediaCurrentTimeLabel) {
        mediaCurrentTimeLabel->setText("0:00");
    }
    if (mediaTotalTimeLabel) {
        mediaTotalTimeLabel->setText("0:00");
    }
    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setSource(activeMediaSource);
    mediaPlayer->play();
    if (!isVideo) {
        updateAudioMetadata();
    }
    stack->setCurrentIndex(3);
}

void QuickLookDialog::stopMedia() {
    if (!mediaPlayer) return;
    activeMediaSource = QUrl();
    activeMediaIsVideo = false;
    retriedVideoWithoutAudio = false;
    mediaPlayer->stop();
    if (mediaSeekSlider) {
        mediaSeekSlider->setRange(0, 0);
        mediaSeekSlider->setValue(0);
    }
    if (mediaCurrentTimeLabel) {
        mediaCurrentTimeLabel->setText("0:00");
    }
    if (mediaTotalTimeLabel) {
        mediaTotalTimeLabel->setText("0:00");
    }
}

void QuickLookDialog::showUnsupported(const QString &path, const QString &mime) {
    QFileInfo fi(path);
    QString name = fi.fileName().isEmpty() ? path : fi.fileName();
    QString typeText = mime.isEmpty() ? "unknown type" : mime;
    unsupportedLabel->setText(
        QString("<b>No Preview Available</b><br/><br/>%1<br/><span style='color:#a8b0bd;'>Type: %2</span>")
            .arg(name, typeText)
    );
    stack->setCurrentWidget(unsupportedLabel);
}

void QuickLookDialog::showDirectory(const QString &path) {
    QFileInfo fi(path);
    const QString displayName = fi.fileName().isEmpty() ? path : fi.fileName();
    const int itemCount = QDir(path).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).size();
    const quint64 requestId = m_directorySizeRequestId;
    const QString modified = fi.lastModified().isValid()
        ? fi.lastModified().toString("yyyy-MM-dd hh:mm")
        : QString("Unknown");

    if (folderNameLabel) {
        folderNameLabel->setText(displayName);
    }
    if (folderInfoLabel) {
        folderInfoLabel->setText(
            QString("%1 items\nCalculating size…\nModified %2")
                .arg(itemCount)
                .arg(modified)
        );
    }
    if (folderPathLabel) {
        folderPathLabel->setText(path);
    }

    if (folderIconLabel) {
        QIcon folderIcon = QIcon::fromTheme("folder");
        QPixmap pm = folderIcon.pixmap(150, 150);
        if (pm.isNull()) {
            pm = QIcon::fromTheme("inode-directory").pixmap(150, 150);
        }
        folderIconLabel->setPixmap(pm);
    }

    if (folderPage) {
        stack->setCurrentWidget(folderPage);
    } else {
        stack->setCurrentWidget(unsupportedLabel);
    }

    auto *watcher = new QFutureWatcher<qint64>(this);
    connect(watcher, &QFutureWatcher<qint64>::finished, this, [this, watcher, requestId, path, itemCount, modified]() {
        const qint64 totalSize = watcher->result();
        watcher->deleteLater();

        if (requestId != m_directorySizeRequestId || currentFilePath != path) {
            return;
        }

        if (folderInfoLabel) {
            folderInfoLabel->setText(
                QString("%1 items\n%2 total\nModified %3")
                    .arg(itemCount)
                    .arg(formatBytes(totalSize))
                    .arg(modified)
            );
        }
    });
    watcher->setFuture(QtConcurrent::run([path]() {
        return computeDirectorySizeBytes(path);
    }));
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
    filenameLabel->setText(fi.fileName().isEmpty() ? path : fi.fileName());
    stopMedia();
    ++m_directorySizeRequestId;

    if (fi.isDir()) {
        showDirectory(path);
        show();
        raise();
        activateWindow();
        return;
    }

    QMimeDatabase db;
    const auto mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    const QString mime = mt.name();
    const QString suffix = fi.suffix().toLower();

    const QStringList audioSuffixes = {"mp3", "m4a", "aac", "flac", "wav", "ogg", "opus", "oga", "wma", "aif", "aiff", "ac3", "mka"};
    const QStringList videoSuffixes = {"mp4", "m4v", "mkv", "webm", "mov", "avi", "wmv", "flv", "mpeg", "mpg", "3gp", "3g2", "ogv", "asf", "mts", "m2ts", "ts", "mxf", "vob"};
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
    activateWindow();
}

void QuickLookDialog::navigateNext() {
    if (currentFilePath.isEmpty() || !pane) return;
    QString next = pane->adjacentFilePath(currentFilePath, +1);
    if (!next.isEmpty()) {
        showFile(next);
        pane->selectFileInView(next);
    }
}

void QuickLookDialog::navigatePrevious() {
    if (currentFilePath.isEmpty() || !pane) return;
    QString prev = pane->adjacentFilePath(currentFilePath, -1);
    if (!prev.isEmpty()) {
        showFile(prev);
        pane->selectFileInView(prev);
    }
}
