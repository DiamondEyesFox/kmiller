#pragma once
#include <QWidget>
#include <QSplitter>
#include <QListView>
#include <KDirModel>

class MillerView : public QWidget {
    Q_OBJECT
public:
    explicit MillerView(QWidget *parent=nullptr);
    void setRootUrl(const QUrl &url);

private:
    QSplitter *splitter;
    QList<QListView*> columns;
    QList<KDirModel*> models;
    void addColumn(const QUrl &url);
    void removeColumnsAfter(int col);
};
