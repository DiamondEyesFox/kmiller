#pragma once
#include <QDialog>

class QTabWidget;
class QCheckBox;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QSlider;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent=nullptr);

signals:
    void settingsApplied();

private slots:
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

private:
    void setupGeneralTab();
    void setupViewTab();
    void setupAdvancedTab();
    
    QTabWidget *m_tabs;
    
    // General settings
    QCheckBox *m_showHiddenFiles;
    QCheckBox *m_showPreviewPane;
    QCheckBox *m_showToolbar;
    QComboBox *m_defaultView;
    QComboBox *m_theme;
    
    // View settings
    QSpinBox *m_iconSize;
    QSlider *m_iconSizeSlider;
    QCheckBox *m_showThumbnails;
    QCheckBox *m_showFileExtensions;
    QSpinBox *m_millerColumns;
    
    // Advanced settings
    QLineEdit *m_defaultTerminal;
    QCheckBox *m_confirmDelete;
    QCheckBox *m_moveToTrash;
    QCheckBox *m_followSymlinks;
};
