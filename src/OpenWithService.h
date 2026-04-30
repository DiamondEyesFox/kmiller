#pragma once

#include <QIcon>
#include <QList>
#include <QString>
#include <QUrl>

class QObject;

struct OpenWithApp {
    QString desktopId;
    QString name;
    QString comment;
    QString exec;
    QIcon icon;
};

class OpenWithService {
public:
    static QList<OpenWithApp> applicationsForFile(const QString &filePath);
    static bool launch(const QString &desktopId, const QList<QUrl> &urls, QObject *parent = nullptr);
};
