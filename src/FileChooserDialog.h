#pragma once

#include <QDialog>
#include <QUrl>
#include <QStringList>

class MillerView;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;

/**
 * File chooser dialog using Miller columns
 * Used by the FileChooser portal and can be invoked standalone
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
    void onSelectionChanged(const QUrl &url);
    void onNavigatedTo(const QUrl &url);
    void onAccept();
    void updateAcceptButton();

private:
    void setupUI();
    bool validateSelection();

    MillerView *m_millerView = nullptr;
    QLineEdit *m_filenameEdit = nullptr;
    QComboBox *m_filterCombo = nullptr;
    QPushButton *m_acceptButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QLabel *m_pathLabel = nullptr;

    bool m_saveMode = false;
    bool m_directoryMode = false;
    bool m_multipleSelection = false;

    QUrl m_currentFolder;
    QList<QUrl> m_selectedUrls;
};
