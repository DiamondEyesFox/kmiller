#pragma once
#include <QWidget>
#include <QUrl>
#include <QVector>
#include <QElapsedTimer>
#include <QMetaObject>
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
    void setFollowSymlinks(bool follow);
    void setSort(int column, Qt::SortOrder order);
    void setColumnWidth(int width);
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

    static constexpr int MaxColumns = 20;

    QHBoxLayout *layout = nullptr;
    QVector<QListView*> columns;
    QUrl root;
    bool m_showHiddenFiles = false;
    bool m_followSymlinks = false;
    int m_columnWidth = 200;

    // Type-to-select state
    QString m_searchString;
    QElapsedTimer m_searchTimer;

    // Finder-like slow second-click rename state
    QPointer<QListView> m_renameClickView;
    QPersistentModelIndex m_renameClickIndex;
    QElapsedTimer m_renameClickTimer;

    // Inline rename state
    bool m_isEditing = false;
    QMetaObject::Connection m_closeEditorConnection;

    // Sort state kept in sync with Pane sort actions.
    int m_sortColumn = 0;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};
