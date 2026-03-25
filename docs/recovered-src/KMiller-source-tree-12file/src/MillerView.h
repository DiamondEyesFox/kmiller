#pragma once
#include <QWidget>
#include <QListView>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMenu>

class MillerView : public QWidget {
    Q_OBJECT
public:
    explicit MillerView(QWidget *parent=nullptr);
    void setRootUrl(const QUrl &url);

signals:
    void activated(const QModelIndex &idx);
    void contextMenuRequested(const QPoint &pos);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QHBoxLayout *layout;
    QList<QListView*> columns;
    QUrl root;
    void addColumn(const QUrl &url);
};
