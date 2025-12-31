#include "MainWindow.h"
#include "FileChooserPortal.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>

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

    parser.process(app);

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
