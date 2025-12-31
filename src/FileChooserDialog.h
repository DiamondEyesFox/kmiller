#pragma once

#include <QDialog>
#include <QUrl>
#include <QStringList>

class Pane;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class KFilePlacesView;
class KFilePlacesModel;

/**
 * File chooser dialog using the full Pane widget
 * Includes places sidebar, preview pane, Quick Look, all view modes
 * Used by the FileChooser portal
 */
class FileChooserDialog : public QDialog {
    Q_OBJECT

public:
    explicit FileChooserDialog(QWidget *parent = nullptr);
    ~FileChooserDialog();

    // Configuration
    void setSaveMode(bool save);
    void setDirectoryMode(bool directory);
    void setMultipleSelection(bool multiple);
    void setCurrentFolder(const QString &path);
    void setCurrentName(const QString &name);
    void setFilters(const QStringList &filters);

    // Results
    QList<QUrl> selectedUrls() const;
    QString selectedFilter() const;

private slots:
    void onPlaceActivated(const QUrl &url);
    void onNavigatedTo(const QUrl &url);
    void onAccept();
    void updateAcceptButton();

private:
    void setupUI();
    bool validateSelection();

    // Sidebar
    KFilePlacesModel *m_placesModel = nullptr;
    KFilePlacesView *m_placesView = nullptr;

    // Main pane
    Pane *m_pane = nullptr;

    // Bottom bar
    QLabel *m_filenameLabel = nullptr;
    QLineEdit *m_filenameEdit = nullptr;
    QComboBox *m_filterCombo = nullptr;
    QPushButton *m_acceptButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    bool m_saveMode = false;
    bool m_directoryMode = false;
    bool m_multipleSelection = false;

    QUrl m_currentFolder;
    QList<QUrl> m_selectedUrls;
};
