#include "MainWindow.h"
#include "FileChooserPortal.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QGuiApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("KMiller");
    app.setOrganizationName("DiamondEyesFox");
    app.setApplicationVersion(KMILLER_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Finder-style file manager with Miller Columns and Quick Look");
    parser.addVersionOption();
    parser.addHelpOption();

    // Portal mode option
    QCommandLineOption portalOption(
        QStringList() << "portal",
        "Run as xdg-desktop-portal FileChooser backend"
    );
    parser.addOption(portalOption);

    QCommandLineOption appIdOption(
        QStringList() << "app-id",
        "Override the Wayland app id / desktop file name for this instance.",
        "app-id"
    );
    parser.addOption(appIdOption);

    parser.process(app);

    if (parser.isSet(appIdOption)) {
        const QString appId = parser.value(appIdOption).trimmed();
        if (!appId.isEmpty()) {
            QGuiApplication::setDesktopFileName(appId);
        }
    }

    if (parser.isSet(portalOption)) {
        // Portal mode: register D-Bus service and wait for requests
        qDebug() << "Starting KMiller in FileChooser portal mode...";

        FileChooserPortal portal;
        if (!portal.registerService()) {
            qCritical() << "Failed to register FileChooser portal service";
            return 1;
        }

        qDebug() << "FileChooser portal ready, waiting for requests...";
        return app.exec();
    }

    // Normal file manager mode
    MainWindow w;
    w.show();
    return app.exec();
}
