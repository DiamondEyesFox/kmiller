#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QUrl>

class ArchiveService : public QObject {
    Q_OBJECT

public:
    enum class ExtractConflictPolicy {
        KeepExisting,
        ReplaceExisting,
    };

    static QString defaultArchiveExtension();
    static QString supportedCreateFormatsHint();
    static bool canCreateArchiveAtPath(const QString &archivePath);
    static bool canExtractArchive(const QString &archivePath);
    static bool canPreviewExtractionConflicts(const QString &archivePath);
    static QStringList listExtractionConflicts(const QUrl &archiveUrl, const QString &destinationPath, QString *errorMessage = nullptr);

    static ArchiveService *createArchive(const QList<QUrl> &urls, const QString &archivePath, QObject *parent = nullptr);
    static ArchiveService *extractArchive(const QUrl &archiveUrl,
                                          const QString &destinationPath,
                                          ExtractConflictPolicy conflictPolicy = ExtractConflictPolicy::KeepExisting,
                                          QObject *parent = nullptr);

signals:
    void finished(bool success, const QString &errorMessage);

private:
    explicit ArchiveService(QObject *parent = nullptr);

    void startCreate(const QStringList &sourcePaths, const QString &archivePath);
    void startExtract(const QString &archivePath, const QString &destinationPath, ExtractConflictPolicy conflictPolicy);
};
