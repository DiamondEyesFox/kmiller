#include "ThumbCache.h"

ThumbCache::ThumbCache(QObject *parent) : QObject(parent) {}

bool ThumbCache::has(const QUrl &url) const {
    return cache.contains(url.toString());
}

QPixmap ThumbCache::get(const QUrl &url) const {
    return cache.value(url.toString());
}

void ThumbCache::put(const QUrl &url, const QPixmap &pix) {
    cache.insert(url.toString(), pix);
}
