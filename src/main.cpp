#include "MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("KMiller");
    app.setOrganizationName("DiamondEyesFox");
    app.setApplicationVersion(KMILLER_VERSION_STR);
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Finder-style file manager with Miller Columns and Quick Look");
    parser.addVersionOption();
    parser.process(app);
    
    MainWindow w;
    w.show();
    return app.exec();
}
