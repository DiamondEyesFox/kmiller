#include "MainWindow.h"
#include "ArchiveService.h"
#include "FileChooserPortal.h"
#include "FileOpsService.h"
#include "OpenWithService.h"
#include "Pane.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QGuiApplication>
#include <QUrl>

#include <KZip>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/MkdirJob>
#include <KJob>

static bool waitForJob(KJob *job, const QString &label) {
    if (!job) {
        qCritical() << "QA failed to create job:" << label;
        return false;
    }

    QEventLoop loop;
    QObject::connect(job, &KJob::result, &loop, &QEventLoop::quit);
    loop.exec();

    if (job->error()) {
        qCritical() << "QA job failed:" << label << job->errorString();
        return false;
    }
    return true;
}

static bool waitForArchive(ArchiveService *service, const QString &label) {
    if (!service) {
        qCritical() << "QA failed to create archive service:" << label;
        return false;
    }

    bool success = false;
    QString errorMessage;
    QEventLoop loop;
    QObject::connect(service, &ArchiveService::finished, &loop, [&](bool ok, const QString &error) {
        success = ok;
        errorMessage = error;
        loop.quit();
    });
    loop.exec();

    if (!success) {
        qCritical() << "QA archive job failed:" << label << errorMessage;
        return false;
    }
    return true;
}

static bool runQaFixture(const QString &fixturePath) {
    QDir fixture(fixturePath);
    if (!fixture.exists()) {
        qCritical() << "QA fixture does not exist:" << fixturePath;
        return false;
    }

    const QString alpha = fixture.filePath("alpha.txt");
    const QString duplicate = fixture.filePath("alpha-duplicate.txt");
    const QString copied = fixture.filePath("created-folder/alpha.txt");
    const QString moved = fixture.filePath("subdir/alpha-duplicate.txt");
    const QString beta = fixture.filePath("subdir/beta.txt");
    const QString renamed = fixture.filePath("subdir/beta-renamed.txt");
    const QString createdDir = fixture.filePath("created-folder");

    if (!QFileInfo::exists(alpha)) {
        qCritical() << "QA fixture missing alpha.txt";
        return false;
    }

    if (!waitForJob(FileOpsService::mkdir(QUrl::fromLocalFile(createdDir), nullptr), "mkdir")) {
        return false;
    }
    if (!QFileInfo(createdDir).isDir()) {
        qCritical() << "QA mkdir did not create expected folder";
        return false;
    }

    if (!waitForJob(FileOpsService::copy({QUrl::fromLocalFile(alpha)}, QUrl::fromLocalFile(createdDir), nullptr), "copy")) {
        return false;
    }
    if (!QFileInfo::exists(copied)) {
        qCritical() << "QA copy did not create expected file";
        return false;
    }

    if (!waitForJob(FileOpsService::copyAs(QUrl::fromLocalFile(alpha), QUrl::fromLocalFile(duplicate), nullptr), "copyAs duplicate")) {
        return false;
    }
    if (!QFileInfo::exists(duplicate)) {
        qCritical() << "QA duplicate did not create expected file";
        return false;
    }

    if (!waitForJob(FileOpsService::move({QUrl::fromLocalFile(duplicate)}, QUrl::fromLocalFile(fixture.filePath("subdir")), nullptr), "move")) {
        return false;
    }
    if (QFileInfo::exists(duplicate) || !QFileInfo::exists(moved)) {
        qCritical() << "QA move left unexpected file state";
        return false;
    }

    if (!waitForJob(FileOpsService::rename(QUrl::fromLocalFile(beta), QUrl::fromLocalFile(renamed), nullptr), "rename")) {
        return false;
    }
    if (QFileInfo::exists(beta) || !QFileInfo::exists(renamed)) {
        qCritical() << "QA rename left unexpected file state";
        return false;
    }

    if (!waitForJob(FileOpsService::del({QUrl::fromLocalFile(moved)}, nullptr), "delete moved duplicate")) {
        return false;
    }
    if (QFileInfo::exists(moved)) {
        qCritical() << "QA delete left unexpected file state";
        return false;
    }

    qInfo() << "QA fixture operations passed";
    return true;
}

static bool runQaUiLogic(const QString &fixturePath) {
    QDir fixture(fixturePath);
    if (!fixture.exists()) {
        qCritical() << "QA UI fixture does not exist:" << fixturePath;
        return false;
    }

    const QUrl fixtureUrl = QUrl::fromLocalFile(fixture.absolutePath());
    const QUrl linkUrl = QUrl::fromLocalFile(fixture.filePath("link-to-subdir"));

    Pane pane(fixtureUrl);
    if (pane.currentUrl() != fixtureUrl) {
        qCritical() << "QA pane did not initialize at fixture path";
        return false;
    }

    pane.setFollowSymlinks(false);
    if (pane.canNavigateIntoDirectoryForTesting(linkUrl)) {
        qCritical() << "QA follow-symlinks=false still allowed symlink navigation";
        return false;
    }

    pane.setFollowSymlinks(true);
    if (!pane.canNavigateIntoDirectoryForTesting(linkUrl)) {
        qCritical() << "QA follow-symlinks=true blocked symlink navigation";
        return false;
    }

    MainWindow window(fixtureUrl);
    window.show();
    QCoreApplication::processEvents();

    qInfo() << "QA UI logic checks passed";
    return true;
}

static bool runQaOpenWith(const QString &filePath) {
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        qCritical() << "QA Open With file does not exist:" << filePath;
        return false;
    }

    const QList<OpenWithApp> apps = OpenWithService::applicationsForFile(fileInfo.absoluteFilePath());
    if (apps.isEmpty()) {
        qCritical() << "QA Open With found no applications for:" << filePath;
        return false;
    }

    qInfo() << "QA Open With found" << apps.size() << "application(s) for" << fileInfo.fileName();
    return true;
}

static bool runQaArchive(const QString &fixturePath) {
    QDir fixture(fixturePath);
    if (!fixture.exists()) {
        qCritical() << "QA archive fixture does not exist:" << fixturePath;
        return false;
    }

    const QString alpha = fixture.filePath("alpha.txt");
    const QString archivePath = fixture.filePath("archive-qa.zip");
    const QString extractDir = fixture.filePath("archive-extract");
    const QString extractedAlpha = QDir(extractDir).filePath("alpha.txt");
    const QString traversalArchivePath = fixture.filePath("archive-traversal.zip");
    const QString traversalExtractDir = fixture.filePath("archive-traversal-extract");
    const QString escapedPath = fixture.filePath("evil.txt");
    const QString escapedPath2 = fixture.filePath("evil2.txt");
    const QString absoluteEscapePath = QStringLiteral("/tmp/kmiller-archive-absolute-escape.txt");

    if (!QFileInfo::exists(alpha)) {
        qCritical() << "QA archive fixture missing alpha.txt";
        return false;
    }
    if (!ArchiveService::canCreateArchiveAtPath(archivePath) || !ArchiveService::canExtractArchive(archivePath)) {
        qCritical() << "QA archive format unexpectedly unsupported:" << archivePath;
        return false;
    }

    if (!waitForArchive(
            ArchiveService::createArchive({QUrl::fromLocalFile(alpha)}, archivePath, nullptr),
            "create zip")) {
        return false;
    }
    if (!QFileInfo::exists(archivePath)) {
        qCritical() << "QA archive creation did not create expected file";
        return false;
    }

    if (!waitForArchive(
            ArchiveService::extractArchive(
                QUrl::fromLocalFile(archivePath),
                extractDir,
                ArchiveService::ExtractConflictPolicy::KeepExisting,
                nullptr),
            "extract zip")) {
        return false;
    }
    if (!QFileInfo::exists(extractedAlpha)) {
        qCritical() << "QA archive extraction did not create expected file";
        return false;
    }

    QString conflictError;
    const QStringList conflicts = ArchiveService::listExtractionConflicts(
        QUrl::fromLocalFile(archivePath),
        extractDir,
        &conflictError);
    if (!conflictError.isEmpty() || !conflicts.contains(QStringLiteral("alpha.txt"))) {
        qCritical() << "QA archive conflict detection failed:" << conflictError << conflicts;
        return false;
    }

    {
        QFile::remove(absoluteEscapePath);
        KZip maliciousZip(traversalArchivePath);
        if (!maliciousZip.open(QIODevice::WriteOnly)) {
            qCritical() << "QA failed to create traversal archive";
            return false;
        }
        if (!maliciousZip.writeFile(QStringLiteral("../evil.txt"), QByteArray("escape\n"))) {
            qCritical() << "QA failed to write traversal archive entry";
            maliciousZip.close();
            return false;
        }
        if (!maliciousZip.writeFile(QStringLiteral("nested/../../evil2.txt"), QByteArray("escape2\n"))) {
            qCritical() << "QA failed to write nested traversal archive entry";
            maliciousZip.close();
            return false;
        }
        if (!maliciousZip.writeFile(absoluteEscapePath, QByteArray("absolute\n"))) {
            qCritical() << "QA failed to write absolute traversal archive entry";
            maliciousZip.close();
            return false;
        }
        if (!maliciousZip.close()) {
            qCritical() << "QA failed to close traversal archive";
            return false;
        }
    }

    if (!waitForArchive(
            ArchiveService::extractArchive(
                QUrl::fromLocalFile(traversalArchivePath),
                traversalExtractDir,
                ArchiveService::ExtractConflictPolicy::KeepExisting,
                nullptr),
            "extract traversal zip")) {
        return false;
    }
    if (QFileInfo::exists(escapedPath) || QFileInfo::exists(escapedPath2) || QFileInfo::exists(absoluteEscapePath)) {
        qCritical() << "QA archive traversal escaped extraction target";
        QFile::remove(absoluteEscapePath);
        return false;
    }
    QFile::remove(absoluteEscapePath);

    qInfo() << "QA archive checks passed";
    return true;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("KMiller");
    app.setOrganizationName("DiamondEyesFox");
    app.setApplicationVersion(KMILLER_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Finder-style file manager with Miller Columns and Quick Look");
    parser.addVersionOption();
    parser.addHelpOption();

    // Portal mode option
    QCommandLineOption portalOption(
        QStringList() << "portal",
        "Run as xdg-desktop-portal FileChooser backend"
    );
    parser.addOption(portalOption);

    QCommandLineOption appIdOption(
        QStringList() << "app-id",
        "Override the Wayland app id / desktop file name for this instance.",
        "app-id"
    );
    parser.addOption(appIdOption);

    QCommandLineOption pathOption(
        QStringList() << "path",
        "Open KMiller at the given local folder path.",
        "path"
    );
    parser.addOption(pathOption);

    QCommandLineOption qaFixtureOption(
        QStringList() << "qa-fixture",
        "Run noninteractive QA operations against the given disposable fixture path and exit.",
        "path"
    );
    parser.addOption(qaFixtureOption);

    QCommandLineOption qaUiLogicOption(
        QStringList() << "qa-ui-logic",
        "Run noninteractive UI logic checks against the given disposable fixture path and exit.",
        "path"
    );
    parser.addOption(qaUiLogicOption);

    QCommandLineOption qaOpenWithOption(
        QStringList() << "qa-open-with",
        "Run noninteractive Open With app-discovery QA against the given local file and exit.",
        "path"
    );
    parser.addOption(qaOpenWithOption);

    QCommandLineOption qaArchiveOption(
        QStringList() << "qa-archive",
        "Run noninteractive archive create/extract/conflict QA against the given disposable fixture path and exit.",
        "path"
    );
    parser.addOption(qaArchiveOption);

    parser.process(app);

    if (parser.isSet(appIdOption)) {
        const QString appId = parser.value(appIdOption).trimmed();
        if (!appId.isEmpty()) {
            QGuiApplication::setDesktopFileName(appId);
        }
    }

    if (parser.isSet(portalOption)) {
        // Portal mode: register D-Bus service and wait for requests
        qDebug() << "Starting KMiller in FileChooser portal mode...";

        FileChooserPortal portal;
        if (!portal.registerService()) {
            qCritical() << "Failed to register FileChooser portal service";
            return 1;
        }

        qDebug() << "FileChooser portal ready, waiting for requests...";
        return app.exec();
    }

    if (parser.isSet(qaFixtureOption)) {
        return runQaFixture(parser.value(qaFixtureOption)) ? 0 : 1;
    }

    if (parser.isSet(qaUiLogicOption)) {
        return runQaUiLogic(parser.value(qaUiLogicOption)) ? 0 : 1;
    }

    if (parser.isSet(qaOpenWithOption)) {
        return runQaOpenWith(parser.value(qaOpenWithOption)) ? 0 : 1;
    }

    if (parser.isSet(qaArchiveOption)) {
        return runQaArchive(parser.value(qaArchiveOption)) ? 0 : 1;
    }

    // Normal file manager mode
    QUrl initialUrl = QUrl::fromLocalFile("/");
    if (parser.isSet(pathOption)) {
        const QFileInfo fi(parser.value(pathOption));
        if (fi.exists()) {
            initialUrl = QUrl::fromLocalFile(fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath());
        }
    }

    MainWindow w(initialUrl);
    w.show();
    return app.exec();
}
