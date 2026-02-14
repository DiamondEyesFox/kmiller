#pragma once
#include <QWidget>
#include <QUrl>
#include <QVector>
#include <QElapsedTimer>
#include <QPersistentModelIndex>
#include <QPointer>

class QHBoxLayout;
class QListView;

class MillerView : public QWidget {
    Q_OBJECT
public:
    explicit MillerView(QWidget *parent = nullptr);
    void setRootUrl(const QUrl &url);
    void setShowHiddenFiles(bool show);
    void focusLastColumn();
    QList<QUrl> getSelectedUrls() const;
    void renameSelected();

signals:
    void quickLookRequested(const QString &path);
    void contextMenuRequested(const QList<QUrl> &urls, const QPoint &globalPos);
    void emptySpaceContextMenuRequested(const QUrl &folderUrl, const QPoint &globalPos);
    void selectionChanged(const QUrl &url);
    void navigatedTo(const QUrl &url);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void addColumn(const QUrl &url);
    void pruneColumnsAfter(QListView *view);
    void typeToSelect(QListView *view, const QString &text);
    void beginInlineRename(QListView *view, const QModelIndex &idx);

    QHBoxLayout *layout = nullptr;
    QVector<QListView*> columns;
    QUrl root;
    bool m_showHiddenFiles = false;

    // Type-to-select state
    QString m_searchString;
    QElapsedTimer m_searchTimer;

    // Finder-like slow second-click rename state
    QPointer<QListView> m_renameClickView;
    QPersistentModelIndex m_renameClickIndex;
    QElapsedTimer m_renameClickTimer;
};
