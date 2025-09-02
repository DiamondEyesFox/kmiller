#include "SettingsDialog.h"
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSlider>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>
#include <QSettings>
#include <QApplication>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Preferences"));
    setModal(true);
    resize(500, 400);

    auto *mainLayout = new QVBoxLayout(this);
    
    m_tabs = new QTabWidget(this);
    mainLayout->addWidget(m_tabs);
    
    setupGeneralTab();
    setupViewTab();
    setupAdvancedTab();
    
    // Buttons
    auto *buttonLayout = new QHBoxLayout;
    
    auto *resetButton = new QPushButton(tr("Reset to Defaults"), this);
    connect(resetButton, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
    
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]{ saveSettings(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    
    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(buttons);
    
    mainLayout->addLayout(buttonLayout);
    
    // Load current settings
    loadSettings();
}

void SettingsDialog::setupGeneralTab() {
    auto *generalTab = new QWidget;
    m_tabs->addTab(generalTab, tr("General"));
    
    auto *layout = new QVBoxLayout(generalTab);
    
    // File manager behavior
    auto *behaviorGroup = new QGroupBox(tr("Behavior"));
    layout->addWidget(behaviorGroup);
    
    auto *behaviorLayout = new QVBoxLayout(behaviorGroup);
    
    m_showHiddenFiles = new QCheckBox(tr("Show hidden files by default"));
    m_showHiddenFiles->setToolTip(tr("Show files and folders that start with a dot"));
    behaviorLayout->addWidget(m_showHiddenFiles);
    
    m_confirmDelete = new QCheckBox(tr("Confirm before deleting files"));
    m_confirmDelete->setToolTip(tr("Show confirmation dialog when deleting files"));
    behaviorLayout->addWidget(m_confirmDelete);
    
    m_moveToTrash = new QCheckBox(tr("Move files to trash instead of permanent deletion"));
    m_moveToTrash->setToolTip(tr("Files will be moved to trash instead of being permanently deleted"));
    behaviorLayout->addWidget(m_moveToTrash);
    
    m_followSymlinks = new QCheckBox(tr("Follow symbolic links"));
    m_followSymlinks->setToolTip(tr("Navigate into directories that are symbolic links"));
    behaviorLayout->addWidget(m_followSymlinks);
    
    // Interface
    auto *interfaceGroup = new QGroupBox(tr("Interface"));
    layout->addWidget(interfaceGroup);
    
    auto *interfaceLayout = new QVBoxLayout(interfaceGroup);
    
    m_showToolbar = new QCheckBox(tr("Show toolbar by default"));
    interfaceLayout->addWidget(m_showToolbar);
    
    m_showPreviewPane = new QCheckBox(tr("Show preview pane by default"));
    interfaceLayout->addWidget(m_showPreviewPane);
    
    // Theme selection
    auto *themeLayout = new QHBoxLayout;
    themeLayout->addWidget(new QLabel(tr("Theme:")));
    
    m_theme = new QComboBox;
    m_theme->addItems({tr("Default"), tr("Dark"), tr("Light"), tr("Finder")});
    m_theme->setToolTip(tr("Choose the application theme"));
    themeLayout->addWidget(m_theme);
    themeLayout->addStretch();
    
    interfaceLayout->addLayout(themeLayout);
    
    // Default view
    auto *viewLayout = new QHBoxLayout;
    viewLayout->addWidget(new QLabel(tr("Default view mode:")));
    
    m_defaultView = new QComboBox;
    m_defaultView->addItems({tr("Icons"), tr("Details"), tr("Compact"), tr("Miller Columns")});
    viewLayout->addWidget(m_defaultView);
    viewLayout->addStretch();
    
    interfaceLayout->addLayout(viewLayout);
    
    layout->addStretch();
}

void SettingsDialog::setupViewTab() {
    auto *viewTab = new QWidget;
    m_tabs->addTab(viewTab, tr("View"));
    
    auto *layout = new QVBoxLayout(viewTab);
    
    // Icon settings
    auto *iconGroup = new QGroupBox(tr("Icons"));
    layout->addWidget(iconGroup);
    
    auto *iconLayout = new QGridLayout(iconGroup);
    
    iconLayout->addWidget(new QLabel(tr("Icon size:")), 0, 0);
    
    m_iconSize = new QSpinBox;
    m_iconSize->setRange(16, 256);
    m_iconSize->setSuffix(" px");
    iconLayout->addWidget(m_iconSize, 0, 1);
    
    m_iconSizeSlider = new QSlider(Qt::Horizontal);
    m_iconSizeSlider->setRange(16, 256);
    iconLayout->addWidget(m_iconSizeSlider, 0, 2, 1, 2);
    
    // Connect slider and spinbox
    connect(m_iconSize, QOverload<int>::of(&QSpinBox::valueChanged), m_iconSizeSlider, &QSlider::setValue);
    connect(m_iconSizeSlider, &QSlider::valueChanged, m_iconSize, &QSpinBox::setValue);
    
    m_showThumbnails = new QCheckBox(tr("Show thumbnails for images"));
    m_showThumbnails->setToolTip(tr("Display thumbnail previews for image files"));
    iconLayout->addWidget(m_showThumbnails, 1, 0, 1, 4);
    
    // File display
    auto *fileGroup = new QGroupBox(tr("File Display"));
    layout->addWidget(fileGroup);
    
    auto *fileLayout = new QVBoxLayout(fileGroup);
    
    m_showFileExtensions = new QCheckBox(tr("Always show file extensions"));
    m_showFileExtensions->setToolTip(tr("Show file extensions even for known file types"));
    fileLayout->addWidget(m_showFileExtensions);
    
    // Miller columns specific
    auto *millerLayout = new QHBoxLayout;
    millerLayout->addWidget(new QLabel(tr("Miller columns width:")));
    
    m_millerColumns = new QSpinBox;
    m_millerColumns->setRange(150, 400);
    m_millerColumns->setSuffix(" px");
    m_millerColumns->setToolTip(tr("Width of each column in Miller view"));
    millerLayout->addWidget(m_millerColumns);
    millerLayout->addStretch();
    
    fileLayout->addLayout(millerLayout);
    
    layout->addStretch();
}

void SettingsDialog::setupAdvancedTab() {
    auto *advancedTab = new QWidget;
    m_tabs->addTab(advancedTab, tr("Advanced"));
    
    auto *layout = new QVBoxLayout(advancedTab);
    
    // External programs
    auto *programsGroup = new QGroupBox(tr("External Programs"));
    layout->addWidget(programsGroup);
    
    auto *programsLayout = new QGridLayout(programsGroup);
    
    programsLayout->addWidget(new QLabel(tr("Terminal:")), 0, 0);
    m_defaultTerminal = new QLineEdit;
    m_defaultTerminal->setPlaceholderText(tr("e.g., konsole, gnome-terminal, xterm"));
    m_defaultTerminal->setToolTip(tr("Terminal program to use for 'Open Terminal Here'"));
    programsLayout->addWidget(m_defaultTerminal, 0, 1);
    
    layout->addStretch();
    
    // Performance note
    auto *noteLabel = new QLabel(tr("<i>Note: Some settings may require restarting KMiller to take effect.</i>"));
    noteLabel->setWordWrap(true);
    layout->addWidget(noteLabel);
}

void SettingsDialog::loadSettings() {
    QSettings settings;
    
    // General settings - get current state from MainWindow if possible, otherwise from settings
    if (auto *mainWin = qobject_cast<MainWindow*>(parentWidget())) {
        m_showHiddenFiles->setChecked(mainWin->areHiddenFilesVisible());
        m_showToolbar->setChecked(mainWin->isToolbarVisible());
        m_showPreviewPane->setChecked(mainWin->isPreviewPaneVisible());
        m_defaultView->setCurrentIndex(mainWin->getCurrentViewMode());
        m_theme->setCurrentIndex(mainWin->getCurrentTheme());
    } else {
        // Fallback to settings file
        m_showHiddenFiles->setChecked(settings.value("general/showHiddenFiles", false).toBool());
        m_showToolbar->setChecked(settings.value("general/showToolbar", false).toBool());
        m_showPreviewPane->setChecked(settings.value("general/showPreviewPane", false).toBool());
        m_defaultView->setCurrentIndex(settings.value("general/defaultView", 0).toInt());
        m_theme->setCurrentIndex(settings.value("general/theme", 0).toInt());
    }
    
    // View settings
    int iconSize = settings.value("view/iconSize", 64).toInt();
    m_iconSize->setValue(iconSize);
    m_iconSizeSlider->setValue(iconSize);
    m_showThumbnails->setChecked(settings.value("view/showThumbnails", true).toBool());
    m_showFileExtensions->setChecked(settings.value("view/showFileExtensions", true).toBool());
    m_millerColumns->setValue(settings.value("view/millerColumnWidth", 200).toInt());
    
    // Advanced settings
    m_defaultTerminal->setText(settings.value("advanced/defaultTerminal", "konsole").toString());
    m_confirmDelete->setChecked(settings.value("advanced/confirmDelete", true).toBool());
    m_moveToTrash->setChecked(settings.value("advanced/moveToTrash", true).toBool());
    m_followSymlinks->setChecked(settings.value("advanced/followSymlinks", false).toBool());
}

void SettingsDialog::saveSettings() {
    QSettings settings;
    
    // General settings
    settings.setValue("general/showHiddenFiles", m_showHiddenFiles->isChecked());
    settings.setValue("general/showToolbar", m_showToolbar->isChecked());
    settings.setValue("general/showPreviewPane", m_showPreviewPane->isChecked());
    settings.setValue("general/defaultView", m_defaultView->currentIndex());
    settings.setValue("general/theme", m_theme->currentIndex());
    
    // View settings
    settings.setValue("view/iconSize", m_iconSize->value());
    settings.setValue("view/showThumbnails", m_showThumbnails->isChecked());
    settings.setValue("view/showFileExtensions", m_showFileExtensions->isChecked());
    settings.setValue("view/millerColumnWidth", m_millerColumns->value());
    
    // Advanced settings
    settings.setValue("advanced/defaultTerminal", m_defaultTerminal->text());
    settings.setValue("advanced/confirmDelete", m_confirmDelete->isChecked());
    settings.setValue("advanced/moveToTrash", m_moveToTrash->isChecked());
    settings.setValue("advanced/followSymlinks", m_followSymlinks->isChecked());
    
    settings.sync();
    emit settingsApplied();
}

void SettingsDialog::resetToDefaults() {
    // General defaults
    m_showHiddenFiles->setChecked(false);
    m_showToolbar->setChecked(false);
    m_showPreviewPane->setChecked(false);
    m_defaultView->setCurrentIndex(0); // Icons
    m_theme->setCurrentIndex(0); // Default
    
    // View defaults
    m_iconSize->setValue(64);
    m_iconSizeSlider->setValue(64);
    m_showThumbnails->setChecked(true);
    m_showFileExtensions->setChecked(true);
    m_millerColumns->setValue(200);
    
    // Advanced defaults
    m_defaultTerminal->setText("konsole");
    m_confirmDelete->setChecked(true);
    m_moveToTrash->setChecked(true);
    m_followSymlinks->setChecked(false);
}