#pragma once
#include <QWidget>
#include <QUrl>
#include <QVector>

class QHBoxLayout;
class QListView;

class MillerView : public QWidget {
    Q_OBJECT
public:
    explicit MillerView(QWidget *parent = nullptr);
    void setRootUrl(const QUrl &url);

signals:
    void quickLookRequested(const QString &path);
    void contextMenuRequested(const QUrl &url, const QPoint &globalPos);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void addColumn(const QUrl &url);
    void pruneColumnsAfter(QListView *view);

    QHBoxLayout *layout = nullptr;
    QVector<QListView*> columns;
    QUrl root;
};
