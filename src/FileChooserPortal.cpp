#include "FileChooserPortal.h"
#include "FileChooserDialog.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QApplication>
#include <QUrl>
#include <QDebug>

FileChooserPortal::FileChooserPortal(QObject *parent)
    : QObject(parent)
{
}

FileChooserPortal::~FileChooserPortal() = default;

bool FileChooserPortal::registerService()
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    // Register the object
    if (!bus.registerObject("/org/freedesktop/portal/desktop",
                            this,
                            QDBusConnection::ExportAllSlots)) {
        qWarning() << "Failed to register FileChooser portal object:" << bus.lastError().message();
        return false;
    }

    // Register the service name
    if (!bus.registerService("org.freedesktop.impl.portal.desktop.kmiller")) {
        qWarning() << "Failed to register FileChooser portal service:" << bus.lastError().message();
        return false;
    }

    qDebug() << "KMiller FileChooser portal registered successfully";
    return true;
}

uint FileChooserPortal::OpenFile(const QDBusObjectPath &handle,
                                  const QString &app_id,
                                  const QString &parent_window,
                                  const QString &title,
                                  const QVariantMap &options,
                                  QVariantMap &results)
{
    Q_UNUSED(handle);
    Q_UNUSED(app_id);
    Q_UNUSED(parent_window);

    bool multiple = options.value("multiple", false).toBool();
    bool directory = options.value("directory", false).toBool();

    return showPicker(title.isEmpty() ? "Open" : title,
                      options,
                      false,     // not save mode
                      directory,
                      multiple,
                      results);
}

uint FileChooserPortal::SaveFile(const QDBusObjectPath &handle,
                                  const QString &app_id,
                                  const QString &parent_window,
                                  const QString &title,
                                  const QVariantMap &options,
                                  QVariantMap &results)
{
    Q_UNUSED(handle);
    Q_UNUSED(app_id);
    Q_UNUSED(parent_window);

    return showPicker(title.isEmpty() ? "Save" : title,
                      options,
                      true,      // save mode
                      false,
                      false,
                      results);
}

uint FileChooserPortal::SaveFiles(const QDBusObjectPath &handle,
                                   const QString &app_id,
                                   const QString &parent_window,
                                   const QString &title,
                                   const QVariantMap &options,
                                   QVariantMap &results)
{
    Q_UNUSED(handle);
    Q_UNUSED(app_id);
    Q_UNUSED(parent_window);

    // SaveFiles is for saving multiple files to a directory
    return showPicker(title.isEmpty() ? "Save Files" : title,
                      options,
                      true,      // save mode
                      true,      // directory mode (pick folder)
                      false,
                      results);
}

uint FileChooserPortal::showPicker(const QString &title,
                                    const QVariantMap &options,
                                    bool saveMode,
                                    bool directoryMode,
                                    bool multipleFiles,
                                    QVariantMap &results)
{
    // Extract filters from options
    QStringList filters;
    if (options.contains("filters")) {
        // Filters are in format: [(name, [(type, pattern), ...]), ...]
        // We simplify to just the patterns for now
        QVariantList filterList = options.value("filters").toList();
        for (const QVariant &f : filterList) {
            QVariantList filter = f.toList();
            if (filter.size() >= 2) {
                QString name = filter[0].toString();
                QVariantList patterns = filter[1].toList();
                QStringList patternStrs;
                for (const QVariant &p : patterns) {
                    QVariantList pat = p.toList();
                    if (pat.size() >= 2) {
                        patternStrs << pat[1].toString();
                    }
                }
                if (!patternStrs.isEmpty()) {
                    filters << QString("%1 (%2)").arg(name).arg(patternStrs.join(" "));
                }
            }
        }
    }

    // Get current folder if specified
    QString currentFolder;
    if (options.contains("current_folder")) {
        QByteArray folderBytes = options.value("current_folder").toByteArray();
        currentFolder = QString::fromUtf8(folderBytes);
    }

    // Get suggested filename for save dialogs
    QString currentName;
    if (options.contains("current_name")) {
        currentName = options.value("current_name").toString();
    }

    // Create and show the picker dialog
    FileChooserDialog dialog(nullptr);
    dialog.setWindowTitle(title);
    dialog.setSaveMode(saveMode);
    dialog.setDirectoryMode(directoryMode);
    dialog.setMultipleSelection(multipleFiles);

    if (!currentFolder.isEmpty()) {
        dialog.setCurrentFolder(currentFolder);
    }
    if (!currentName.isEmpty()) {
        dialog.setCurrentName(currentName);
    }
    if (!filters.isEmpty()) {
        dialog.setFilters(filters);
    }

    // Show dialog and wait for result
    if (dialog.exec() == QDialog::Accepted) {
        QList<QUrl> selectedUrls = dialog.selectedUrls();

        if (selectedUrls.isEmpty()) {
            return 1; // Cancelled
        }

        // Convert to URI list for D-Bus response
        QStringList uris;
        for (const QUrl &url : selectedUrls) {
            uris << url.toString();
        }

        results["uris"] = uris;

        // For save mode, also include the writable flag
        if (saveMode) {
            results["writable"] = true;
        }

        return 0; // Success
    }

    return 1; // Cancelled
}
