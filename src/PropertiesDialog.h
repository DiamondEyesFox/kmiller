#pragma once
#include <QDialog>
#include <QUrl>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QTabWidget;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QProgressBar;

class PropertiesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PropertiesDialog(const QUrl &url, QWidget *parent = nullptr);
    explicit PropertiesDialog(const QList<QUrl> &urls, QWidget *parent = nullptr);

private slots:
    void applyChanges();

private:
    void setupUI();
    void setupGeneralTab();
    void setupPermissionsTab();
    void loadFileInfo();
    void loadMultipleFilesInfo();
    
    QUrl m_url;
    QList<QUrl> m_urls;
    bool m_multipleFiles;
    
    // UI components
    QTabWidget *m_tabs;
    
    // General tab
    QWidget *m_generalTab;
    QLabel *m_iconLabel;
    QLineEdit *m_nameEdit;
    QLabel *m_typeLabel;
    QLabel *m_sizeLabel;
    QLabel *m_locationLabel;
    QLabel *m_createdLabel;
    QLabel *m_modifiedLabel;
    QLabel *m_accessedLabel;
    
    // Permissions tab
    QWidget *m_permissionsTab;
    QCheckBox *m_ownerRead;
    QCheckBox *m_ownerWrite;
    QCheckBox *m_ownerExecute;
    QCheckBox *m_groupRead;
    QCheckBox *m_groupWrite;
    QCheckBox *m_groupExecute;
    QCheckBox *m_otherRead;
    QCheckBox *m_otherWrite;
    QCheckBox *m_otherExecute;
    QLabel *m_ownerLabel;
    QLabel *m_groupLabel;
    
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    QPushButton *m_applyButton;
};