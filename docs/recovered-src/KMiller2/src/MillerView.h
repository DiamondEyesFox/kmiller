#pragma once
#include <QWidget>
#include <QSplitter>
#include <QListView>
#include <QFileSystemModel>

class MillerView : public QWidget {
    Q_OBJECT
public:
    explicit MillerView(QWidget *parent=nullptr);
    void setRootPath(const QString &path);
    QString currentFilePath() const;

private:
    QSplitter *splitter;
    QList<QListView*> columns;
    QList<QFileSystemModel*> models;

    void addColumn(const QString &path);
    void removeColumnsAfter(int col);
};
