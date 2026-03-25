#pragma once
#include <QWidget>
#include <QToolBar>
#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QListView>
#include <QTreeView>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QKeyEvent>
#include "MillerView.h"
#include "QuickLookDialog.h"

class TabPage : public QWidget {
    Q_OBJECT
public:
    explicit TabPage(const QString &startPath, QWidget *parent=nullptr);

    void setPath(const QString &path);

signals:
    void titlePathChanged(const QString &path);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void goBack();
    void goForward();
    void goUp();
    void onPathEdited();
    void onViewModeChanged(int index);
    void onSearchChanged(const QString &text);
    void onActivated(const QModelIndex &idx);

private:
    enum ViewMode { Icons=0, Details=1, Compact=2, Miller=3 };

    // UI
    QToolBar *toolbar;
    QAction *actBack;
    QAction *actForward;
    QAction *actUp;
    QLineEdit *pathEdit;
    QLineEdit *searchEdit;
    QComboBox *viewModeBox;

    QStackedWidget *viewStack;
    QListView *iconView;
    QTreeView *detailsView;
    QListView *compactView;
    MillerView *millerView;

    // Models
    QFileSystemModel *fsModel;
    QSortFilterProxyModel *proxy;

    // History
    QStringList history;
    int historyIndex = -1;

    // Quick Look
    QuickLookDialog *ql;

    void navigateTo(const QString &path, bool recordHistory=true);
    void updateViewsRoot(const QString &path);
};
