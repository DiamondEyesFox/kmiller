#include "PaneNavigationState.h"

void PaneNavigationState::navigateTo(const QUrl &url) {
    if (m_historyIndex < 0) {
        m_history = {url};
        m_historyIndex = 0;
        return;
    }

    if (current() == url) {
        return;
    }

    while (m_history.size() > m_historyIndex + 1) {
        m_history.removeLast();
    }
    m_history.append(url);
    m_historyIndex = m_history.size() - 1;
}

bool PaneNavigationState::canGoBack() const {
    return m_historyIndex > 0;
}

bool PaneNavigationState::canGoForward() const {
    return m_historyIndex >= 0 && m_historyIndex < m_history.size() - 1;
}

QUrl PaneNavigationState::goBack() {
    if (!canGoBack()) {
        return current();
    }

    --m_historyIndex;
    return current();
}

QUrl PaneNavigationState::goForward() {
    if (!canGoForward()) {
        return current();
    }

    ++m_historyIndex;
    return current();
}

QUrl PaneNavigationState::current() const {
    if (m_historyIndex < 0 || m_historyIndex >= m_history.size()) {
        return {};
    }
    return m_history[m_historyIndex];
}
