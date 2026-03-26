#pragma once
#include <QDialog>
#include <QString>
#include <QUrl>
class QStackedWidget;
class QLabel;
class QShortcut;
class QMediaPlayer;
class QAudioOutput;
class QImage;
class QWidget;
class QVideoWidget;
class QPushButton;
class QSlider;
class Pane;

class QuickLookDialog : public QDialog {
    Q_OBJECT
public:
    explicit QuickLookDialog(Pane *parentPane = nullptr);
    void showFile(const QString &path);

private slots:
    void navigateNext();
    void navigatePrevious();
    void toggleMediaPlayback();
    void updateAudioMetadata();

private:
    void showImage(const QString &path);
    void showPdf(const QString &path);
    bool showText(const QString &path);
    void showDirectory(const QString &path);
    void showMedia(const QString &path, bool isVideo);
    void stopMedia();
    void showUnsupported(const QString &path, const QString &mime);
    void resetAudioMetadata(const QString &path);
    void setAudioArtwork(const QImage &image);

    Pane *pane = nullptr;
    QString currentFilePath;
    QStackedWidget *stack = nullptr;
    QLabel *filenameLabel = nullptr;
    QLabel *unsupportedLabel = nullptr;
    QShortcut *escShortcut = nullptr;
    QShortcut *spaceShortcut = nullptr;
    QShortcut *upShortcut = nullptr;
    QShortcut *downShortcut = nullptr;
    QShortcut *leftShortcut = nullptr;
    QShortcut *rightShortcut = nullptr;
    QShortcut *jShortcut = nullptr;
    QShortcut *kShortcut = nullptr;
    QShortcut *lShortcut = nullptr;

    QMediaPlayer *mediaPlayer = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QStackedWidget *mediaContentStack = nullptr;
    QVideoWidget *videoWidget = nullptr;
    QWidget *audioPanel = nullptr;
    QLabel *audioArtworkLabel = nullptr;
    QLabel *audioTitleLabel = nullptr;
    QLabel *audioArtistLabel = nullptr;
    QLabel *audioAlbumLabel = nullptr;
    QLabel *audioYearLabel = nullptr;
    QWidget *folderPage = nullptr;
    QLabel *folderIconLabel = nullptr;
    QLabel *folderNameLabel = nullptr;
    QLabel *folderInfoLabel = nullptr;
    QLabel *folderPathLabel = nullptr;
    QLabel *mediaInfoLabel = nullptr;
    QLabel *mediaCurrentTimeLabel = nullptr;
    QLabel *mediaTotalTimeLabel = nullptr;
    QPushButton *mediaPlayPauseButton = nullptr;
    QPushButton *mediaMuteButton = nullptr;
    QSlider *mediaSeekSlider = nullptr;
    QSlider *mediaVolumeSlider = nullptr;
    QUrl activeMediaSource;
    bool activeMediaIsVideo = false;
    bool retriedVideoWithoutAudio = false;
    quint64 m_directorySizeRequestId = 0;
};
