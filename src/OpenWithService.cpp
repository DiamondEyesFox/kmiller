#include "OpenWithService.h"

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KService>

#include <QFileInfo>
#include <QDir>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>

namespace {

QStringList gioMimeDesktopIds(const QString &mimeTypeName) {
    QProcess gio;
    gio.start(QStringLiteral("gio"), {QStringLiteral("mime"), mimeTypeName});
    if (!gio.waitForFinished(1500) || gio.exitStatus() != QProcess::NormalExit || gio.exitCode() != 0) {
        return {};
    }

    QStringList desktopIds;
    const QString output = QString::fromUtf8(gio.readAllStandardOutput());
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.endsWith(QStringLiteral(".desktop"))) {
            continue;
        }

        const QStringList fields = trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        for (const QString &field : fields) {
            if (field.endsWith(QStringLiteral(".desktop")) && !desktopIds.contains(field)) {
                desktopIds << field;
            }
        }
    }

    return desktopIds;
}

KService::Ptr serviceForDesktopId(const QString &desktopId) {
    KService::Ptr service = KService::serviceByDesktopName(desktopId);
    if (!service && desktopId.endsWith(QStringLiteral(".desktop"))) {
        service = KService::serviceByDesktopName(desktopId.chopped(QStringLiteral(".desktop").size()));
    }
    return service;
}

bool appendService(QList<OpenWithApp> *apps, QSet<QString> *seenDesktopIds, const KService::Ptr &service) {
    if (!apps || !seenDesktopIds || !service || service->exec().isEmpty()) {
        return false;
    }

    const QString desktopId = service->desktopEntryName();
    if (desktopId.isEmpty() || seenDesktopIds->contains(desktopId)) {
        return false;
    }

    seenDesktopIds->insert(desktopId);
    apps->append({
        desktopId,
        service->name(),
        service->comment(),
        service->exec(),
        QIcon::fromTheme(service->icon())
    });
    return true;
}

QStringList applicationDesktopDirs() {
    QStringList dirs;
    const QString dataHome = qEnvironmentVariable(
        "XDG_DATA_HOME",
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    if (!dataHome.isEmpty()) {
        dirs << QDir(dataHome).filePath(QStringLiteral("applications"));
    }

    const QString dataDirsEnv = qEnvironmentVariable("XDG_DATA_DIRS", QStringLiteral("/usr/local/share:/usr/share"));
    const QStringList dataDirs = dataDirsEnv.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString &dataDir : dataDirs) {
        dirs << QDir(dataDir).filePath(QStringLiteral("applications"));
    }

    dirs.removeDuplicates();
    return dirs;
}

QString desktopFilePathForId(const QString &desktopId) {
    const QString normalizedId = desktopId.endsWith(QStringLiteral(".desktop"))
        ? desktopId
        : desktopId + QStringLiteral(".desktop");

    for (const QString &dir : applicationDesktopDirs()) {
        const QString path = QDir(dir).filePath(normalizedId);
        if (QFileInfo::exists(path)) {
            return path;
        }
    }

    return {};
}

QString desktopValue(const QString &line, const QString &key) {
    const QString prefix = key + QLatin1Char('=');
    if (!line.startsWith(prefix)) {
        return {};
    }
    return line.mid(prefix.size()).trimmed();
}

bool appendDesktopFile(QList<OpenWithApp> *apps, QSet<QString> *seenDesktopIds, const QString &desktopId) {
    if (!apps || !seenDesktopIds) {
        return false;
    }

    const QString normalizedId = desktopId.endsWith(QStringLiteral(".desktop"))
        ? desktopId.chopped(QStringLiteral(".desktop").size())
        : desktopId;
    if (seenDesktopIds->contains(normalizedId)) {
        return false;
    }

    const QString path = desktopFilePathForId(desktopId);
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString name;
    QString comment;
    QString exec;
    QString iconName;
    bool inDesktopEntry = false;
    bool hidden = false;

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            inDesktopEntry = line == QStringLiteral("[Desktop Entry]");
            continue;
        }
        if (!inDesktopEntry) {
            continue;
        }

        const QString nameValue = desktopValue(line, QStringLiteral("Name"));
        if (!nameValue.isEmpty() && name.isEmpty()) {
            name = nameValue;
            continue;
        }
        const QString commentValue = desktopValue(line, QStringLiteral("Comment"));
        if (!commentValue.isEmpty() && comment.isEmpty()) {
            comment = commentValue;
            continue;
        }
        const QString execValue = desktopValue(line, QStringLiteral("Exec"));
        if (!execValue.isEmpty() && exec.isEmpty()) {
            exec = execValue;
            continue;
        }
        const QString iconValue = desktopValue(line, QStringLiteral("Icon"));
        if (!iconValue.isEmpty() && iconName.isEmpty()) {
            iconName = iconValue;
            continue;
        }

        const QString hiddenValue = desktopValue(line, QStringLiteral("Hidden"));
        const QString noDisplayValue = desktopValue(line, QStringLiteral("NoDisplay"));
        hidden = hidden
            || hiddenValue.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0
            || noDisplayValue.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
    }

    if (hidden || name.isEmpty() || exec.isEmpty()) {
        return false;
    }

    seenDesktopIds->insert(normalizedId);
    apps->append({
        normalizedId,
        name,
        comment,
        exec,
        QIcon::fromTheme(iconName)
    });
    return true;
}

QStringList commandForDesktopExec(const QString &exec, const QList<QUrl> &urls) {
    QStringList parts = QProcess::splitCommand(exec);
    if (parts.isEmpty()) {
        return {};
    }

    const QString localPath = !urls.isEmpty() && urls.first().isLocalFile() ? urls.first().toLocalFile() : QString();
    const QString uri = urls.isEmpty() ? QString() : urls.first().toString();
    bool injectedTarget = false;

    QStringList resolved;
    resolved.reserve(parts.size() + 1);
    for (QString part : parts) {
        if (part == QStringLiteral("%f") || part == QStringLiteral("%F")) {
            if (!localPath.isEmpty()) {
                resolved << localPath;
                injectedTarget = true;
            }
            continue;
        }
        if (part == QStringLiteral("%u") || part == QStringLiteral("%U")) {
            if (!uri.isEmpty()) {
                resolved << uri;
                injectedTarget = true;
            }
            continue;
        }
        if (part == QStringLiteral("%i") || part == QStringLiteral("%c") || part == QStringLiteral("%k")) {
            continue;
        }
        part.replace(QStringLiteral("%%"), QStringLiteral("%"));
        resolved << part;
    }

    if (!injectedTarget && !localPath.isEmpty()) {
        resolved << localPath;
    }

    return resolved;
}

} // namespace

QList<OpenWithApp> OpenWithService::applicationsForFile(const QString &filePath) {
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        return {};
    }

    static const QMimeDatabase db;
    const QMimeType mimeType = db.mimeTypeForFile(filePath);
    const QString mimeTypeName = mimeType.name();

    QList<OpenWithApp> apps;
    QSet<QString> seenDesktopIds;

    const KService::List services = KApplicationTrader::queryByMimeType(mimeTypeName);
    for (const KService::Ptr &service : services) {
        appendService(&apps, &seenDesktopIds, service);
    }

    for (const QString &desktopId : gioMimeDesktopIds(mimeTypeName)) {
        if (!appendService(&apps, &seenDesktopIds, serviceForDesktopId(desktopId))) {
            appendDesktopFile(&apps, &seenDesktopIds, desktopId);
        }
    }

    if (apps.size() < 5) {
        const QStringList commonApps = {
            QStringLiteral("org.kde.kate"),
            QStringLiteral("org.kde.dolphin"),
            QStringLiteral("firefox"),
            QStringLiteral("org.gnome.gedit")
        };
        for (const QString &desktopId : commonApps) {
            appendService(&apps, &seenDesktopIds, serviceForDesktopId(desktopId));
        }
    }

    return apps;
}

bool OpenWithService::launch(const QString &desktopId, const QList<QUrl> &urls, QObject *parent) {
    KService::Ptr service = serviceForDesktopId(desktopId);
    if (urls.isEmpty()) {
        return false;
    }

    if (service) {
        auto *job = new KIO::ApplicationLauncherJob(service, parent);
        job->setUrls(urls);
        job->start();
        return true;
    }

    QList<OpenWithApp> apps;
    QSet<QString> seen;
    if (!appendDesktopFile(&apps, &seen, desktopId) || apps.isEmpty()) {
        return false;
    }

    QStringList command = commandForDesktopExec(apps.first().exec, urls);
    if (command.isEmpty()) {
        return false;
    }

    const QString program = command.takeFirst();
    return QProcess::startDetached(program, command);
}
