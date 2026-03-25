#pragma once
#include <QWidget>
#include <QUrl>
#include <QVector>
#include <QElapsedTimer>
#include <QPersistentModelIndex>
#include <QPointer>

class QHBoxLayout;
class QListView;
class ThumbCache;

class MillerView : public QWidget {
    Q_OBJECT
public:
    explicit MillerView(QWidget *parent = nullptr);
    void setRootUrl(const QUrl &url);
    void setShowHiddenFiles(bool show);
    void setShowThumbnails(bool show);
    void setShowFileExtensions(bool show);
    void setColumnWidth(int width);
    void setFollowSymlinks(bool follow);
    void setSort(int column, Qt::SortOrder order);
    void focusLastColumn();
    QList<QUrl> getSelectedUrls() const;
    void renameSelected();

signals:
    void quickLookRequested(const QString &path);
    void contextMenuRequested(const QList<QUrl> &urls, const QPoint &globalPos);
    void emptySpaceContextMenuRequested(const QUrl &folderUrl, const QPoint &globalPos);
    void selectionChanged(const QUrl &url);
    void navigatedTo(const QUrl &url);
    void renameRequested();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void addColumn(const QUrl &url);
    void pruneColumnsAfter(QListView *view);
    void typeToSelect(QListView *view, const QString &text);
    void beginInlineRename(QListView *view, const QModelIndex &idx);
    void generateThumbnail(const QUrl &url) const;
    QIcon getIconForFile(const QUrl &url) const;
    bool isDirectorySymlinkPath(const QString &path) const;
    bool tryNavigateToPath(QListView *view, const QString &path, bool showBlockedMessage);

    QHBoxLayout *layout = nullptr;
    QVector<QListView*> columns;
    QUrl root;
    bool m_showHiddenFiles = false;
    bool m_showThumbnails = true;
    bool m_showFileExtensions = true;
    int m_columnWidth = 200;
    bool m_followSymlinks = false;
    mutable ThumbCache *m_thumbs = nullptr;

    // Type-to-select state
    QString m_searchString;
    QElapsedTimer m_searchTimer;

    // Finder-like slow second-click rename state
    QPointer<QListView> m_renameClickView;
    QPersistentModelIndex m_renameClickIndex;
    QElapsedTimer m_renameClickTimer;

    // Sort state kept in sync with Pane sort actions.
    int m_sortColumn = 0;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};
