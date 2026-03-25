#pragma once
#include <QObject>
#include <QHash>
#include <QPixmap>
#include <QUrl>

class ThumbCache : public QObject {
    Q_OBJECT
public:
    explicit ThumbCache(QObject *parent=nullptr);
    bool has(const QUrl &url) const;
    QPixmap get(const QUrl &url) const;
    void put(const QUrl &url, const QPixmap &pix);

private:
    QHash<QString, QPixmap> cache;
};
