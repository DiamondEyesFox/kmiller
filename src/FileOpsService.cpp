#include "FileOpsService.h"

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/OpenUrlJob>

template <typename T>
static T* adoptJobParent(T *job, QObject *parent) {
    if (job && parent) {
        job->setParent(parent);
    }
    return job;
}

KIO::OpenUrlJob* FileOpsService::openUrl(const QUrl &url, QObject *parent) {
    auto *job = new KIO::OpenUrlJob(url);
    adoptJobParent(job, parent);
    job->start();
    return job;
}

KIO::CopyJob* FileOpsService::copy(const QList<QUrl> &urls, const QUrl &destination, QObject *parent) {
    auto *job = KIO::copy(urls, destination);
    return adoptJobParent(job, parent);
}

KIO::CopyJob* FileOpsService::move(const QList<QUrl> &urls, const QUrl &destination, QObject *parent) {
    auto *job = KIO::move(urls, destination);
    return adoptJobParent(job, parent);
}

KIO::CopyJob* FileOpsService::trash(const QList<QUrl> &urls, QObject *parent) {
    auto *job = KIO::trash(urls);
    return adoptJobParent(job, parent);
}

KIO::DeleteJob* FileOpsService::del(const QList<QUrl> &urls, QObject *parent) {
    auto *job = KIO::del(urls);
    return adoptJobParent(job, parent);
}
