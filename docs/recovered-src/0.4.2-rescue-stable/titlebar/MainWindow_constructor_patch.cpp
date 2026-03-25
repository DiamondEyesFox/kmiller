#include <QCoreApplication>

// inside MainWindow::MainWindow(...)
setWindowTitle(QStringLiteral("KMiller %1").arg(QCoreApplication::applicationVersion()));
