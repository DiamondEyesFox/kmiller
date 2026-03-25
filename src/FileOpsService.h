#pragma once

#include <QList>
#include <QUrl>

class QObject;
class QMimeData;

namespace KIO {
class OpenUrlJob;
class CopyJob;
class DeleteJob;
class SimpleJob;
class MkdirJob;
}

class FileOpsService {
public:
    static void setClipboardUrls(const QList<QUrl> &urls, bool cut);
    static bool isClipboardCutOperation(const QMimeData *mimeData);

    static KIO::OpenUrlJob* openUrl(const QUrl &url, QObject *parent = nullptr);
    static KIO::CopyJob* copy(const QList<QUrl> &urls, const QUrl &destination, QObject *parent = nullptr);
    static KIO::CopyJob* copyAs(const QUrl &src, const QUrl &destination, QObject *parent = nullptr);
    static KIO::CopyJob* move(const QList<QUrl> &urls, const QUrl &destination, QObject *parent = nullptr);
    static KIO::CopyJob* trash(const QList<QUrl> &urls, QObject *parent = nullptr);
    static KIO::DeleteJob* del(const QList<QUrl> &urls, QObject *parent = nullptr);
    static KIO::SimpleJob* rename(const QUrl &src, const QUrl &destination, QObject *parent = nullptr);
    static KIO::MkdirJob* mkdir(const QUrl &url, QObject *parent = nullptr);
};
