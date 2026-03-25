#include "FileOpsService.h"

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/FileUndoManager>
#include <KIO/JobUiDelegateFactory>
#include <KIO/MkdirJob>
#include <KIO/OpenUrlJob>
#include <KIO/Paste>
#include <KIO/SimpleJob>
#include <KJobUiDelegate>

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QWidget>

template <typename T>
static T* adoptJobParent(T *job, QObject *parent) {
    if (job && parent) {
        job->setParent(parent);
    }
    return job;
}

template <typename T>
static T* configureJobUi(T *job, QObject *parent) {
    job = adoptJobParent(job, parent);
    if (!job) {
        return nullptr;
    }

    QWidget *window = nullptr;
    if (auto *widget = qobject_cast<QWidget *>(parent)) {
        window = widget->window();
    }

    if (KJobUiDelegate *delegate = KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window)) {
        job->setUiDelegate(delegate);
    }

    return job;
}

static KIO::CopyJob* configureCopyJob(KIO::CopyJob *job, QObject *parent) {
    job = configureJobUi(job, parent);
    if (job) {
        KIO::FileUndoManager::self()->recordCopyJob(job);
    }
    return job;
}

template <typename T>
static T* configureUndoableJob(T *job,
                               KIO::FileUndoManager::CommandType commandType,
                               const QList<QUrl> &src,
                               const QUrl &dst,
                               QObject *parent) {
    job = configureJobUi(job, parent);
    if (job) {
        KIO::FileUndoManager::self()->recordJob(commandType, src, dst, job);
    }
    return job;
}

void FileOpsService::setClipboardUrls(const QList<QUrl> &urls, bool cut) {
    if (urls.isEmpty()) {
        if (QClipboard *clipboard = QGuiApplication::clipboard()) {
            clipboard->clear();
        }
        return;
    }

    auto *mimeData = new QMimeData();
    mimeData->setUrls(urls);
    KIO::setClipboardDataCut(mimeData, cut);

    QByteArray gnomeData = cut ? "cut" : "copy";
    for (const QUrl &url : urls) {
        gnomeData += '\n' + url.toEncoded();
    }
    mimeData->setData("x-special/gnome-copied-files", gnomeData);

    QGuiApplication::clipboard()->setMimeData(mimeData);
}

bool FileOpsService::isClipboardCutOperation(const QMimeData *mimeData) {
    if (!mimeData) {
        return false;
    }

    if (KIO::isClipboardDataCut(mimeData)) {
        return true;
    }

    const QByteArray gnomeData = mimeData->data("x-special/gnome-copied-files");
    return gnomeData.startsWith("cut\n") || gnomeData.startsWith("cut\r");
}

KIO::OpenUrlJob* FileOpsService::openUrl(const QUrl &url, QObject *parent) {
    auto *job = configureJobUi(new KIO::OpenUrlJob(url), parent);
    job->start();
    return job;
}

KIO::CopyJob* FileOpsService::copy(const QList<QUrl> &urls, const QUrl &destination, QObject *parent) {
    return configureCopyJob(KIO::copy(urls, destination), parent);
}

KIO::CopyJob* FileOpsService::copyAs(const QUrl &src, const QUrl &destination, QObject *parent) {
    return configureCopyJob(KIO::copyAs(src, destination), parent);
}

KIO::CopyJob* FileOpsService::move(const QList<QUrl> &urls, const QUrl &destination, QObject *parent) {
    return configureCopyJob(KIO::move(urls, destination), parent);
}

KIO::CopyJob* FileOpsService::trash(const QList<QUrl> &urls, QObject *parent) {
    return configureCopyJob(KIO::trash(urls), parent);
}

KIO::DeleteJob* FileOpsService::del(const QList<QUrl> &urls, QObject *parent) {
    return configureJobUi(KIO::del(urls), parent);
}

KIO::SimpleJob* FileOpsService::rename(const QUrl &src, const QUrl &destination, QObject *parent) {
    return configureUndoableJob(
        KIO::rename(src, destination, KIO::HideProgressInfo),
        KIO::FileUndoManager::Rename,
        {src},
        destination,
        parent);
}

KIO::MkdirJob* FileOpsService::mkdir(const QUrl &url, QObject *parent) {
    return configureUndoableJob(
        KIO::mkdir(url),
        KIO::FileUndoManager::Mkdir,
        {},
        url,
        parent);
}
