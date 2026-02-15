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
#include <QToolButton>
#include <QSlider>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QShortcut>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QSizePolicy>
#include <QRegularExpression>
#include <QDate>
#include <QDateTime>
#include <QIcon>
#include <QPainter>
#include <QStyle>
#include <QScreen>
#include <QGuiApplication>
#include <QMediaPlayer>
#include <QMediaMetaData>
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

    mediaContentStack = new QStackedWidget(mediaPage);
    mediaContentStack->setStyleSheet(
        "QStackedWidget { background-color: #111111; border: 1px solid #3a3a3a; border-radius: 8px; }"
    );
    mediaLayout->addWidget(mediaContentStack, 1);

    videoWidget = new QVideoWidget(mediaContentStack);
    videoWidget->setMinimumHeight(300);
    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoWidget->setStyleSheet("background-color: #111111; border-radius: 7px;");
    mediaContentStack->addWidget(videoWidget);

    audioPanel = new QWidget(mediaContentStack);
    audioPanel->setObjectName("audioPanel");
    audioPanel->setStyleSheet(
        "#audioPanel { background-color: #171a1f; border-radius: 7px; }"
    );
    auto *audioPanelLayout = new QHBoxLayout(audioPanel);
    audioPanelLayout->setContentsMargins(28, 24, 28, 24);
    audioPanelLayout->setSpacing(22);
    audioPanelLayout->setAlignment(Qt::AlignCenter);

    audioArtworkLabel = new QLabel(audioPanel);
    audioArtworkLabel->setFixedSize(250, 250);
    audioArtworkLabel->setAlignment(Qt::AlignCenter);
    audioArtworkLabel->setStyleSheet(
        "QLabel { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2a2f38, stop:1 #1a1d23); "
        "border: 1px solid #3a3f49; border-radius: 12px; }"
    );
    audioPanelLayout->addWidget(audioArtworkLabel, 0, Qt::AlignVCenter);

    auto *audioMetaCard = new QWidget(audioPanel);
    audioMetaCard->setMinimumWidth(320);
    audioMetaCard->setMaximumWidth(620);
    audioMetaCard->setStyleSheet(
        "QWidget { background-color: #202327; border: 1px solid #363b43; border-radius: 10px; }"
    );
    auto *audioMetaLayout = new QVBoxLayout(audioMetaCard);
    audioMetaLayout->setContentsMargins(20, 18, 20, 18);
    audioMetaLayout->setSpacing(8);
    audioMetaLayout->setAlignment(Qt::AlignVCenter);

    audioTitleLabel = new QLabel(audioMetaCard);
    audioTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    audioTitleLabel->setWordWrap(true);
    audioTitleLabel->setStyleSheet("QLabel { color: #f2f2f2; font-size: 24px; font-weight: 700; }");
    audioMetaLayout->addWidget(audioTitleLabel);

    audioArtistLabel = new QLabel(audioMetaCard);
    audioArtistLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    audioArtistLabel->setWordWrap(true);
    audioArtistLabel->setStyleSheet("QLabel { color: #a8beff; font-size: 17px; font-weight: 600; }");
    audioMetaLayout->addWidget(audioArtistLabel);

    audioAlbumLabel = new QLabel(audioMetaCard);
    audioAlbumLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    audioAlbumLabel->setWordWrap(true);
    audioAlbumLabel->setStyleSheet("QLabel { color: #c3c7cf; font-size: 14px; }");
    audioMetaLayout->addWidget(audioAlbumLabel);

    audioYearLabel = new QLabel(audioMetaCard);
    audioYearLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    audioYearLabel->setStyleSheet(
        "QLabel { color: #d7dbe2; font-size: 12px; background-color: #2d333d; border: 1px solid #47505f; "
        "border-radius: 10px; padding: 2px 10px; max-width: 120px; }"
    );
    audioMetaLayout->addWidget(audioYearLabel);
    audioMetaLayout->addStretch(1);

    audioPanelLayout->addWidget(audioMetaCard, 1, Qt::AlignVCenter);
    mediaContentStack->addWidget(audioPanel);

    mediaInfoLabel = new QLabel(mediaPage);
    mediaInfoLabel->setAlignment(Qt::AlignCenter);
    mediaInfoLabel->setWordWrap(true);
    mediaInfoLabel->setStyleSheet("QLabel { color: #aeb4bf; font-size: 12px; }");
    mediaLayout->addWidget(mediaInfoLabel);

    auto *controlsShell = new QWidget(mediaPage);
    controlsShell->setObjectName("mediaControlsShell");
    controlsShell->setStyleSheet(
        "#mediaControlsShell { background-color: rgba(34, 37, 43, 220); border: 1px solid #4a515e; border-radius: 13px; }"
        "QToolButton { color: #e7ebf2; background: transparent; border: none; border-radius: 8px; padding: 4px; }"
        "QToolButton:hover { background-color: rgba(255,255,255,20); }"
        "QToolButton:pressed { background-color: rgba(255,255,255,32); }"
        "QSlider::groove:horizontal { border: none; height: 4px; background: #586173; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #b7c2d8; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #f3f5fa; border: 1px solid #8590a6; width: 12px; margin: -5px 0; border-radius: 6px; }"
    );
    auto *controlsLayout = new QHBoxLayout(controlsShell);
    controlsLayout->setContentsMargins(12, 8, 12, 8);
    controlsLayout->setSpacing(6);

    mediaRewindButton = new QToolButton(controlsShell);
    mediaRewindButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    mediaRewindButton->setToolTip("Back 10 seconds");
    controlsLayout->addWidget(mediaRewindButton);

    mediaPlayPauseButton = new QToolButton(controlsShell);
    mediaPlayPauseButton->setIconSize(QSize(20, 20));
    mediaPlayPauseButton->setToolTip("Play/Pause");
    controlsLayout->addWidget(mediaPlayPauseButton);

    mediaForwardButton = new QToolButton(controlsShell);
    mediaForwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    mediaForwardButton->setToolTip("Forward 10 seconds");
    controlsLayout->addWidget(mediaForwardButton);

    mediaCurrentTimeLabel = new QLabel("0:00", controlsShell);
    mediaCurrentTimeLabel->setMinimumWidth(44);
    mediaCurrentTimeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mediaCurrentTimeLabel->setStyleSheet("QLabel { color: #d3d9e6; font-size: 11px; }");
    controlsLayout->addWidget(mediaCurrentTimeLabel);

    mediaSeekSlider = new QSlider(Qt::Horizontal, controlsShell);
    mediaSeekSlider->setRange(0, 0);
    controlsLayout->addWidget(mediaSeekSlider, 1);

    mediaDurationLabel = new QLabel("0:00", controlsShell);
    mediaDurationLabel->setMinimumWidth(44);
    mediaDurationLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mediaDurationLabel->setStyleSheet("QLabel { color: #d3d9e6; font-size: 11px; }");
    controlsLayout->addWidget(mediaDurationLabel);

    mediaMuteButton = new QToolButton(controlsShell);
    mediaMuteButton->setToolTip("Mute");
    controlsLayout->addWidget(mediaMuteButton);

    mediaVolumeSlider = new QSlider(Qt::Horizontal, controlsShell);
    mediaVolumeSlider->setRange(0, 100);
    mediaVolumeSlider->setFixedWidth(90);
    controlsLayout->addWidget(mediaVolumeSlider);

    mediaLayout->addWidget(controlsShell);
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
    if (mediaVolumeSlider) {
        mediaVolumeSlider->setValue(static_cast<int>(audioOutput->volume() * 100.0));
    }
    connect(audioOutput, &QAudioOutput::mutedChanged, this, [this](bool) {
        updateMuteButtonIcon();
    });
    connect(audioOutput, &QAudioOutput::volumeChanged, this, [this](float volume) {
        if (!mediaVolumeSlider) return;
        const int sliderValue = static_cast<int>(volume * 100.0);
        if (mediaVolumeSlider->value() != sliderValue) {
            mediaVolumeSlider->setValue(sliderValue);
        }
        updateMuteButtonIcon();
    });
    updatePlaybackButtonIcon();
    updateMuteButtonIcon();

    connect(mediaPlayPauseButton, &QToolButton::clicked, this, &QuickLookDialog::toggleMediaPlayback);
    connect(mediaRewindButton, &QToolButton::clicked, this, [this]() { seekRelative(-10000); });
    connect(mediaForwardButton, &QToolButton::clicked, this, [this]() { seekRelative(10000); });
    connect(mediaMuteButton, &QToolButton::clicked, this, [this]() {
        if (!audioOutput) return;
        audioOutput->setMuted(!audioOutput->isMuted());
        updateMuteButtonIcon();
    });
    connect(mediaVolumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (!audioOutput) return;
        audioOutput->setVolume(static_cast<qreal>(value) / 100.0);
        if (value > 0 && audioOutput->isMuted()) {
            audioOutput->setMuted(false);
        }
        updateMuteButtonIcon();
    });
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        Q_UNUSED(state);
        updatePlaybackButtonIcon();
    });
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!mediaSeekSlider->isSliderDown()) {
            mediaSeekSlider->setValue(static_cast<int>(position));
        }
        if (mediaCurrentTimeLabel) {
            mediaCurrentTimeLabel->setText(formatMediaTime(position));
        }
    });
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        mediaSeekSlider->setRange(0, static_cast<int>(duration));
        if (mediaDurationLabel) {
            mediaDurationLabel->setText(formatMediaTime(duration));
        }
    });
    connect(mediaPlayer, &QMediaPlayer::metaDataChanged, this, &QuickLookDialog::updateAudioMetadata);
    connect(mediaSeekSlider, &QSlider::sliderMoved, this, [this](int position) {
        mediaPlayer->setPosition(position);
    });
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

QString QuickLookDialog::formatMediaTime(qint64 ms) const {
    if (ms < 0) ms = 0;
    const qint64 totalSeconds = ms / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QString("%1:%2")
        .arg(minutes)
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

void QuickLookDialog::seekRelative(qint64 deltaMs) {
    if (!mediaPlayer) return;
    qint64 target = mediaPlayer->position() + deltaMs;
    if (target < 0) target = 0;
    const qint64 duration = mediaPlayer->duration();
    if (duration > 0 && target > duration) target = duration;
    mediaPlayer->setPosition(target);
}

void QuickLookDialog::updatePlaybackButtonIcon() {
    if (!mediaPlayPauseButton || !mediaPlayer) return;
    const bool playing = mediaPlayer->playbackState() == QMediaPlayer::PlayingState;
    mediaPlayPauseButton->setIcon(style()->standardIcon(
        playing ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay
    ));
}

void QuickLookDialog::updateMuteButtonIcon() {
    if (!mediaMuteButton || !audioOutput) return;
    const bool muted = audioOutput->isMuted() || audioOutput->volume() <= 0.0001;
    mediaMuteButton->setIcon(style()->standardIcon(
        muted ? QStyle::SP_MediaVolumeMuted : QStyle::SP_MediaVolume
    ));
}

void QuickLookDialog::setAudioArtwork(const QImage &image) {
    if (!audioArtworkLabel) return;

    QPixmap artwork;
    if (!image.isNull()) {
        artwork = QPixmap::fromImage(image);
    } else {
        QIcon icon = QIcon::fromTheme("audio-x-generic");
        artwork = icon.pixmap(180, 180);
        if (artwork.isNull()) {
            artwork = QPixmap(180, 180);
            artwork.fill(Qt::transparent);
            QPainter p(&artwork);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#d6dde8"));
            p.drawEllipse(artwork.rect().adjusted(10, 10, -10, -10));
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
    if (mediaDurationLabel) {
        mediaDurationLabel->setText("0:00");
    }
    updatePlaybackButtonIcon();
    updateMuteButtonIcon();
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
