#pragma once

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDBusConnection>
#include <QVariantMap>

class FileChooserDialog;

/**
 * D-Bus portal implementation for org.freedesktop.impl.portal.FileChooser
 *
 * This allows kmiller to be used as the system file picker when applications
 * request file dialogs through xdg-desktop-portal.
 */
class FileChooserPortal : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.FileChooser")

public:
    explicit FileChooserPortal(QObject *parent = nullptr);
    ~FileChooserPortal();

    bool registerService();

public Q_SLOTS:
    /**
     * Open file picker for selecting files to open
     * Returns: 0 = success, 1 = cancelled, 2 = error
     */
    uint OpenFile(const QDBusObjectPath &handle,
                  const QString &app_id,
                  const QString &parent_window,
                  const QString &title,
                  const QVariantMap &options,
                  QVariantMap &results);

    /**
     * Save file picker for choosing save location
     * Returns: 0 = success, 1 = cancelled, 2 = error
     */
    uint SaveFile(const QDBusObjectPath &handle,
                  const QString &app_id,
                  const QString &parent_window,
                  const QString &title,
                  const QVariantMap &options,
                  QVariantMap &results);

    /**
     * Save multiple files to a directory
     * Returns: 0 = success, 1 = cancelled, 2 = error
     */
    uint SaveFiles(const QDBusObjectPath &handle,
                   const QString &app_id,
                   const QString &parent_window,
                   const QString &title,
                   const QVariantMap &options,
                   QVariantMap &results);

private:
    uint showPicker(const QString &title,
                    const QVariantMap &options,
                    bool saveMode,
                    bool directoryMode,
                    bool multipleFiles,
                    QVariantMap &results);
};
