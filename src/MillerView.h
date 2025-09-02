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
    void setShowHiddenFiles(bool show);

signals:
    void quickLookRequested(const QString &path);
    void contextMenuRequested(const QUrl &url, const QPoint &globalPos);
    void emptySpaceContextMenuRequested(const QPoint &globalPos);
    void selectionChanged(const QUrl &url);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void addColumn(const QUrl &url);
    void pruneColumnsAfter(QListView *view);

    QHBoxLayout *layout = nullptr;
    QVector<QListView*> columns;
    QUrl root;
    bool m_showHiddenFiles = false;
};
