#pragma once

#include <QList>
#include <QUrl>

class PaneNavigationState {
public:
    void navigateTo(const QUrl &url);
    bool canGoBack() const;
    bool canGoForward() const;
    QUrl goBack();
    QUrl goForward();
    QUrl current() const;

private:
    QList<QUrl> m_history;
    int m_historyIndex = -1;
};
