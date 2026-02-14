#pragma once

#include <QList>
#include <QUrl>

class QObject;

namespace KIO {
class OpenUrlJob;
class CopyJob;
class DeleteJob;
}

class FileOpsService {
public:
    static KIO::OpenUrlJob* openUrl(const QUrl &url, QObject *parent = nullptr);
    static KIO::CopyJob* copy(const QList<QUrl> &urls, const QUrl &destination, QObject *parent = nullptr);
    static KIO::CopyJob* move(const QList<QUrl> &urls, const QUrl &destination, QObject *parent = nullptr);
    static KIO::CopyJob* trash(const QList<QUrl> &urls, QObject *parent = nullptr);
    static KIO::DeleteJob* del(const QList<QUrl> &urls, QObject *parent = nullptr);
};
