#include "ArchiveService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>

#include <K7Zip>
#include <KArchive>
#include <KArchiveEntry>
#include <KArchiveDirectory>
#include <KTar>
#include <KZip>

#include <memory>
#include <algorithm>

namespace {

struct ArchiveJobResult {
    bool success = false;
    QString errorMessage;
};

struct ArchiveConflict {
    QString relativePath;
    QString absolutePath;
};

enum class ArchiveFormat {
    Unknown,
    Zip,
    SevenZip,
    Tar,
    TarGz,
    TarBz2,
    TarXz,
    TarZstd,
    TarLzip,
    Rar,
};

ArchiveFormat archiveFormatForPath(const QString &path) {
    const QString lowerPath = path.toLower();

    if (lowerPath.endsWith(".tar.gz") || lowerPath.endsWith(".tgz")) {
        return ArchiveFormat::TarGz;
    }
    if (lowerPath.endsWith(".tar.bz2") || lowerPath.endsWith(".tbz2") || lowerPath.endsWith(".tbz")) {
        return ArchiveFormat::TarBz2;
    }
    if (lowerPath.endsWith(".tar.xz") || lowerPath.endsWith(".txz")) {
        return ArchiveFormat::TarXz;
    }
    if (lowerPath.endsWith(".tar.zst") || lowerPath.endsWith(".tzst")) {
        return ArchiveFormat::TarZstd;
    }
    if (lowerPath.endsWith(".tar.lz") || lowerPath.endsWith(".tlz")) {
        return ArchiveFormat::TarLzip;
    }
    if (lowerPath.endsWith(".7z")) {
        return ArchiveFormat::SevenZip;
    }
    if (lowerPath.endsWith(".zip")) {
        return ArchiveFormat::Zip;
    }
    if (lowerPath.endsWith(".tar")) {
        return ArchiveFormat::Tar;
    }
    if (lowerPath.endsWith(".rar")) {
        return ArchiveFormat::Rar;
    }

    return ArchiveFormat::Unknown;
}

bool hasRarFallback() {
    return !QStandardPaths::findExecutable(QStringLiteral("7z")).isEmpty()
        || !QStandardPaths::findExecutable(QStringLiteral("unrar")).isEmpty();
}

bool pathExistsOrIsSymlink(const QString &path) {
    const QFileInfo info(path);
    return info.exists() || info.isSymLink();
}

QString formatOpenError(const QString &archivePath, const QString &errorString) {
    const QString message = errorString.trimmed();
    if (!message.isEmpty()) {
        return message;
    }

    return QStringLiteral("Failed to open archive \"%1\".").arg(QFileInfo(archivePath).fileName());
}

std::unique_ptr<KArchive> createArchiveHandler(const QString &archivePath, ArchiveFormat format) {
    switch (format) {
    case ArchiveFormat::Zip:
        return std::make_unique<KZip>(archivePath);
    case ArchiveFormat::SevenZip:
        return std::make_unique<K7Zip>(archivePath);
    case ArchiveFormat::Tar:
        return std::make_unique<KTar>(archivePath);
    case ArchiveFormat::TarGz:
        return std::make_unique<KTar>(archivePath, QStringLiteral("application/gzip"));
    case ArchiveFormat::TarBz2:
        return std::make_unique<KTar>(archivePath, QStringLiteral("application/x-bzip"));
    case ArchiveFormat::TarXz:
        return std::make_unique<KTar>(archivePath, QStringLiteral("application/x-xz"));
    case ArchiveFormat::TarZstd:
        return std::make_unique<KTar>(archivePath, QStringLiteral("application/zstd"));
    case ArchiveFormat::TarLzip:
        return std::make_unique<KTar>(archivePath, QStringLiteral("application/x-lzip"));
    case ArchiveFormat::Unknown:
    case ArchiveFormat::Rar:
        return nullptr;
    }

    return nullptr;
}

bool openArchiveForReading(const QString &archivePath,
                           std::unique_ptr<KArchive> *archiveOut,
                           const KArchiveDirectory **rootDirectoryOut,
                           QString *errorMessage) {
    if (!archiveOut || !rootDirectoryOut) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Internal archive error.");
        }
        return false;
    }

    const ArchiveFormat format = archiveFormatForPath(archivePath);
    if (format == ArchiveFormat::Unknown) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("That archive format is not supported yet.");
        }
        return false;
    }
    if (format == ArchiveFormat::Rar) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Conflict preview is not available for RAR archives.");
        }
        return false;
    }

    std::unique_ptr<KArchive> archive = createArchiveHandler(archivePath, format);
    if (!archive) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not initialize the selected archive format.");
        }
        return false;
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = formatOpenError(archivePath, archive->errorString());
        }
        return false;
    }

    const KArchiveDirectory *rootDirectory = archive->directory();
    if (!rootDirectory) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The archive did not expose a readable directory tree.");
        }
        archive->close();
        return false;
    }

    *archiveOut = std::move(archive);
    *rootDirectoryOut = rootDirectory;
    return true;
}

void appendConflictIfNeeded(const KArchiveEntry *entry,
                            const QString &relativePath,
                            const QString &destinationPath,
                            QList<ArchiveConflict> *conflicts,
                            QSet<QString> *seenPaths) {
    if (!entry || !conflicts || !seenPaths) {
        return;
    }

    const QString absolutePath = QDir(destinationPath).filePath(relativePath);
    const QFileInfo targetInfo(absolutePath);
    const bool targetExists = targetInfo.exists() || targetInfo.isSymLink();
    const bool targetIsDirectory = targetInfo.isDir() && !targetInfo.isSymLink();
    const bool targetIsSymlink = targetInfo.isSymLink();

    bool conflict = false;
    if (entry->isDirectory()) {
        conflict = targetExists && (!targetIsDirectory || targetIsSymlink);
    } else {
        conflict = targetExists;
    }

    if (!conflict || seenPaths->contains(absolutePath)) {
        return;
    }

    conflicts->append({relativePath, absolutePath});
    seenPaths->insert(absolutePath);
}

void collectArchiveConflicts(const KArchiveDirectory *directory,
                             const QString &relativePrefix,
                             const QString &destinationPath,
                             QList<ArchiveConflict> *conflicts,
                             QSet<QString> *seenPaths) {
    if (!directory || !conflicts || !seenPaths) {
        return;
    }

    QStringList entryNames = directory->entries();
    std::sort(entryNames.begin(), entryNames.end());

    for (const QString &entryName : entryNames) {
        const KArchiveEntry *entry = directory->entry(entryName);
        if (!entry) {
            continue;
        }

        const QString relativePath = relativePrefix.isEmpty()
            ? entryName
            : QStringLiteral("%1/%2").arg(relativePrefix, entryName);

        appendConflictIfNeeded(entry, relativePath, destinationPath, conflicts, seenPaths);
        if (entry->isDirectory()) {
            collectArchiveConflicts(
                static_cast<const KArchiveDirectory *>(entry),
                relativePath,
                destinationPath,
                conflicts,
                seenPaths);
        }
    }
}

QList<ArchiveConflict> listExtractionConflictsImpl(const QString &archivePath,
                                                   const QString &destinationPath,
                                                   QString *errorMessage) {
    QList<ArchiveConflict> conflicts;

    std::unique_ptr<KArchive> archive;
    const KArchiveDirectory *rootDirectory = nullptr;
    if (!openArchiveForReading(archivePath, &archive, &rootDirectory, errorMessage)) {
        return conflicts;
    }

    QSet<QString> seenPaths;
    collectArchiveConflicts(rootDirectory, QString(), destinationPath, &conflicts, &seenPaths);

    archive->close();
    return conflicts;
}

bool removeExistingPath(const QString &path, QString *errorMessage) {
    const QFileInfo info(path);
    if (!info.exists() && !info.isSymLink()) {
        return true;
    }

    bool removed = false;
    if (info.isSymLink() || !info.isDir()) {
        removed = QFile::remove(path);
    } else {
        QDir dir(path);
        removed = dir.removeRecursively();
    }

    if (!removed && errorMessage) {
        *errorMessage = QStringLiteral("Could not replace existing item \"%1\".").arg(path);
    }
    return removed;
}

ArchiveJobResult createArchiveImpl(const QStringList &sourcePaths, const QString &archivePath) {
    if (sourcePaths.isEmpty()) {
        return {false, QStringLiteral("There is nothing to compress.")};
    }

    const ArchiveFormat format = archiveFormatForPath(archivePath);
    if (format == ArchiveFormat::Unknown || format == ArchiveFormat::Rar) {
        return {
            false,
            QStringLiteral("Unsupported archive format. Use .zip, .7z, .tar, .tar.gz, .tar.bz2, or .tar.xz.")
        };
    }

    const QFileInfo archiveInfo(archivePath);
    QDir outputDir(archiveInfo.absolutePath());
    if (!outputDir.exists()) {
        return {false, QStringLiteral("The archive destination folder does not exist.")};
    }

    std::unique_ptr<KArchive> archive = createArchiveHandler(archivePath, format);
    if (!archive) {
        return {false, QStringLiteral("Could not initialize the selected archive format.")};
    }

    if (!archive->open(QIODevice::WriteOnly)) {
        return {false, formatOpenError(archivePath, archive->errorString())};
    }

    for (const QString &sourcePath : sourcePaths) {
        const QFileInfo sourceInfo(sourcePath);
        if (!sourceInfo.exists()) {
            archive->close();
            return {false, QStringLiteral("\"%1\" no longer exists.").arg(sourcePath)};
        }

        QString destName = sourceInfo.fileName();
        if (destName.isEmpty()) {
            destName = QDir(sourcePath).dirName();
        }

        const bool added = sourceInfo.isDir()
            ? archive->addLocalDirectory(sourcePath, destName)
            : archive->addLocalFile(sourcePath, destName);
        if (!added) {
            const QString error = archive->errorString().trimmed();
            archive->close();
            return {
                false,
                error.isEmpty()
                    ? QStringLiteral("Failed to add \"%1\" to the archive.").arg(destName)
                    : error
            };
        }
    }

    if (!archive->close()) {
        return {false, formatOpenError(archivePath, archive->errorString())};
    }

    return {true, QString()};
}

ArchiveJobResult extractRarFallback(const QString &archivePath, const QString &destinationPath) {
    QString program = QStandardPaths::findExecutable(QStringLiteral("7z"));
    QStringList arguments;

    if (!program.isEmpty()) {
        arguments << QStringLiteral("x")
                  << QStringLiteral("-aos")
                  << archivePath
                  << (QStringLiteral("-o") + destinationPath);
    } else {
        program = QStandardPaths::findExecutable(QStringLiteral("unrar"));
        if (program.isEmpty()) {
            return {false, QStringLiteral("RAR extraction requires either 7z or unrar to be installed.")};
        }
        arguments << QStringLiteral("x")
                  << QStringLiteral("-o-")
                  << archivePath
                  << destinationPath;
    }

    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted()) {
        return {false, QStringLiteral("Failed to launch \"%1\" for archive extraction.").arg(QFileInfo(program).fileName())};
    }

    process.closeWriteChannel();
    if (!process.waitForFinished(-1)) {
        process.kill();
        process.waitForFinished();
        return {false, QStringLiteral("Archive extraction did not finish cleanly.")};
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString error = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        if (error.isEmpty()) {
            error = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        }
        if (error.isEmpty()) {
            error = QStringLiteral("The extractor returned exit code %1.").arg(process.exitCode());
        }
        return {false, error};
    }

    return {true, QString()};
}

ArchiveJobResult extractArchiveImpl(const QString &archivePath,
                                    const QString &destinationPath,
                                    ArchiveService::ExtractConflictPolicy conflictPolicy) {
    const ArchiveFormat format = archiveFormatForPath(archivePath);
    if (format == ArchiveFormat::Unknown) {
        return {false, QStringLiteral("That archive format is not supported yet.")};
    }

    QDir destinationDir(destinationPath);
    if (!destinationDir.exists() && !destinationDir.mkpath(QStringLiteral("."))) {
        return {false, QStringLiteral("Could not create the extraction folder.")};
    }

    if (format == ArchiveFormat::Rar) {
        return extractRarFallback(archivePath, destinationPath);
    }

    QString openError;
    std::unique_ptr<KArchive> archive;
    const KArchiveDirectory *rootDirectory = nullptr;
    if (!openArchiveForReading(archivePath, &archive, &rootDirectory, &openError)) {
        return {false, openError};
    }

    if (conflictPolicy == ArchiveService::ExtractConflictPolicy::ReplaceExisting) {
        const QList<ArchiveConflict> conflicts = listExtractionConflictsImpl(archivePath, destinationPath, nullptr);
        for (const ArchiveConflict &conflict : conflicts) {
            QString removalError;
            if (!removeExistingPath(conflict.absolutePath, &removalError)) {
                archive->close();
                return {false, removalError};
            }
        }
    }

    if (!rootDirectory->copyTo(destinationPath, true)) {
        const QString error = archive->errorString().trimmed();
        archive->close();
        return {
            false,
            error.isEmpty()
                ? QStringLiteral("Failed to extract the archive contents.")
                : error
        };
    }

    if (!archive->close()) {
        return {false, formatOpenError(archivePath, archive->errorString())};
    }

    return {true, QString()};
}

} // namespace

QString ArchiveService::defaultArchiveExtension() {
    return QStringLiteral(".zip");
}

QString ArchiveService::supportedCreateFormatsHint() {
    return QStringLiteral(".zip, .7z, .tar, .tar.gz, .tar.bz2, or .tar.xz");
}

bool ArchiveService::canCreateArchiveAtPath(const QString &archivePath) {
    const ArchiveFormat format = archiveFormatForPath(archivePath);
    return format != ArchiveFormat::Unknown && format != ArchiveFormat::Rar;
}

bool ArchiveService::canExtractArchive(const QString &archivePath) {
    const ArchiveFormat format = archiveFormatForPath(archivePath);
    if (format == ArchiveFormat::Rar) {
        return hasRarFallback();
    }
    return format != ArchiveFormat::Unknown;
}

bool ArchiveService::canPreviewExtractionConflicts(const QString &archivePath) {
    const ArchiveFormat format = archiveFormatForPath(archivePath);
    return format != ArchiveFormat::Unknown && format != ArchiveFormat::Rar;
}

QStringList ArchiveService::listExtractionConflicts(const QUrl &archiveUrl,
                                                    const QString &destinationPath,
                                                    QString *errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }

    if (!archiveUrl.isValid() || !archiveUrl.isLocalFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Only local archives are supported.");
        }
        return {};
    }

    const QList<ArchiveConflict> conflicts = listExtractionConflictsImpl(archiveUrl.toLocalFile(), destinationPath, errorMessage);
    QStringList relativePaths;
    relativePaths.reserve(conflicts.size());
    for (const ArchiveConflict &conflict : conflicts) {
        relativePaths.append(conflict.relativePath);
    }
    return relativePaths;
}

ArchiveService *ArchiveService::createArchive(const QList<QUrl> &urls, const QString &archivePath, QObject *parent) {
    auto *service = new ArchiveService(parent);

    QStringList sourcePaths;
    sourcePaths.reserve(urls.size());
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            sourcePaths.append(url.toLocalFile());
        }
    }

    service->startCreate(sourcePaths, archivePath);
    return service;
}

ArchiveService *ArchiveService::extractArchive(const QUrl &archiveUrl,
                                              const QString &destinationPath,
                                              ExtractConflictPolicy conflictPolicy,
                                              QObject *parent) {
    auto *service = new ArchiveService(parent);
    service->startExtract(archiveUrl.toLocalFile(), destinationPath, conflictPolicy);
    return service;
}

ArchiveService::ArchiveService(QObject *parent)
    : QObject(parent) {
}

void ArchiveService::startCreate(const QStringList &sourcePaths, const QString &archivePath) {
    auto *watcher = new QFutureWatcher<ArchiveJobResult>(this);
    connect(watcher, &QFutureWatcher<ArchiveJobResult>::finished, this, [this, watcher]() {
        const ArchiveJobResult result = watcher->result();
        watcher->deleteLater();
        emit finished(result.success, result.errorMessage);
        deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([sourcePaths, archivePath]() {
        return createArchiveImpl(sourcePaths, archivePath);
    }));
}

void ArchiveService::startExtract(const QString &archivePath,
                                  const QString &destinationPath,
                                  ExtractConflictPolicy conflictPolicy) {
    auto *watcher = new QFutureWatcher<ArchiveJobResult>(this);
    connect(watcher, &QFutureWatcher<ArchiveJobResult>::finished, this, [this, watcher]() {
        const ArchiveJobResult result = watcher->result();
        watcher->deleteLater();
        emit finished(result.success, result.errorMessage);
        deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([archivePath, destinationPath, conflictPolicy]() {
        return extractArchiveImpl(archivePath, destinationPath, conflictPolicy);
    }));
}
