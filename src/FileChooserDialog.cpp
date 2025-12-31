#include "FileChooserDialog.h"
#include "Pane.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QShortcut>
#include <QTimer>
#include <KFilePlacesView>
#include <KFilePlacesModel>

FileChooserDialog::FileChooserDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle(tr("Select File"));
    resize(1200, 750);

    // Start in home directory
    setCurrentFolder(QDir::homePath());

    // Focus the pane after dialog is shown
    QTimer::singleShot(100, this, [this]() {
        if (m_pane) {
            m_pane->setFocus();
        }
    });
}

FileChooserDialog::~FileChooserDialog() = default;

void FileChooserDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Splitter for sidebar + pane
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // Places sidebar (KDE integration)
    m_placesModel = new KFilePlacesModel(this);
    m_placesView = new KFilePlacesView(splitter);
    m_placesView->setModel(m_placesModel);
    m_placesView->setAutoResizeItemsEnabled(false);
    m_placesView->setMinimumWidth(150);
    m_placesView->setMaximumWidth(200);

    // Style the places sidebar
    m_placesView->setStyleSheet(
        "KFilePlacesView { background-color: palette(window); border: none; border-right: 1px solid palette(mid); }"
    );

    splitter->addWidget(m_placesView);

    // Full Pane widget (includes toolbar, nav, miller view, preview)
    m_pane = new Pane(QUrl::fromLocalFile(QDir::homePath()), splitter);
    m_pane->setPreviewVisible(true);  // Enable preview by default for file picking
    splitter->addWidget(m_pane);

    // Set splitter sizes (sidebar smaller)
    splitter->setSizes({180, 1000});
    splitter->setStretchFactor(0, 0);  // Sidebar doesn't stretch
    splitter->setStretchFactor(1, 1);  // Pane stretches

    mainLayout->addWidget(splitter, 1);

    // Bottom bar: filename + filter + buttons
    auto *bottomBar = new QWidget(this);
    bottomBar->setStyleSheet("QWidget { background: palette(window); border-top: 1px solid palette(mid); }");
    auto *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(12, 8, 12, 8);
    bottomLayout->setSpacing(8);

    // Filename label and input (for save mode)
    m_filenameLabel = new QLabel(tr("Name:"), this);
    m_filenameEdit = new QLineEdit(this);
    m_filenameEdit->setPlaceholderText(tr("Filename"));
    m_filenameEdit->setMinimumWidth(200);
    bottomLayout->addWidget(m_filenameLabel);
    bottomLayout->addWidget(m_filenameEdit);

    // Hide filename fields by default (shown only in save mode)
    m_filenameLabel->setVisible(false);
    m_filenameEdit->setVisible(false);

    // Filter dropdown
    m_filterCombo = new QComboBox(this);
    m_filterCombo->setMinimumWidth(180);
    m_filterCombo->addItem(tr("All Files (*)"));
    bottomLayout->addWidget(m_filterCombo);

    bottomLayout->addStretch();

    // Buttons
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_acceptButton = new QPushButton(tr("Open"), this);
    m_acceptButton->setDefault(true);

    bottomLayout->addWidget(m_cancelButton);
    bottomLayout->addWidget(m_acceptButton);

    mainLayout->addWidget(bottomBar);

    // Keyboard shortcuts
    auto *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &QDialog::reject);

    // Connections
    connect(m_placesView, &KFilePlacesView::urlChanged, this, &FileChooserDialog::onPlaceActivated);
    connect(m_pane, &Pane::urlChanged, this, &FileChooserDialog::onNavigatedTo);
    connect(m_filenameEdit, &QLineEdit::textChanged, this, &FileChooserDialog::updateAcceptButton);
    connect(m_filenameEdit, &QLineEdit::returnPressed, this, &FileChooserDialog::onAccept);
    connect(m_acceptButton, &QPushButton::clicked, this, &FileChooserDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Initial state
    updateAcceptButton();
}

void FileChooserDialog::onPlaceActivated(const QUrl &url)
{
    if (m_pane && url.isValid()) {
        m_pane->setUrl(url);
        m_currentFolder = url;
    }
}

void FileChooserDialog::setSaveMode(bool save)
{
    m_saveMode = save;
    m_filenameLabel->setVisible(save);
    m_filenameEdit->setVisible(save);
    m_acceptButton->setText(save ? tr("Save") : tr("Open"));
    updateAcceptButton();
}

void FileChooserDialog::setDirectoryMode(bool directory)
{
    m_directoryMode = directory;
    m_filenameLabel->setVisible(!directory && m_saveMode);
    m_filenameEdit->setVisible(!directory && m_saveMode);
    if (directory) {
        m_acceptButton->setText(tr("Select Folder"));
    }
    updateAcceptButton();
}

void FileChooserDialog::setMultipleSelection(bool multiple)
{
    m_multipleSelection = multiple;
    // Pane already supports extended selection
}

void FileChooserDialog::setCurrentFolder(const QString &path)
{
    m_currentFolder = QUrl::fromLocalFile(path);
    if (m_pane) {
        m_pane->setUrl(m_currentFolder);
    }
}

void FileChooserDialog::setCurrentName(const QString &name)
{
    m_filenameEdit->setText(name);
}

void FileChooserDialog::setFilters(const QStringList &filters)
{
    m_filterCombo->clear();
    for (const QString &filter : filters) {
        m_filterCombo->addItem(filter);
    }
    if (filters.isEmpty()) {
        m_filterCombo->addItem(tr("All Files (*)"));
    }
}

QList<QUrl> FileChooserDialog::selectedUrls() const
{
    return m_selectedUrls;
}

QString FileChooserDialog::selectedFilter() const
{
    return m_filterCombo->currentText();
}

void FileChooserDialog::onNavigatedTo(const QUrl &url)
{
    if (!url.isValid() || !url.isLocalFile()) return;
    m_currentFolder = url;
}

void FileChooserDialog::updateAcceptButton()
{
    bool enabled = true;

    if (m_saveMode && !m_directoryMode) {
        // Need filename for save
        enabled = !m_filenameEdit->text().trimmed().isEmpty();
    }

    m_acceptButton->setEnabled(enabled);
}

bool FileChooserDialog::validateSelection()
{
    if (m_saveMode && !m_directoryMode) {
        QString filename = m_filenameEdit->text().trimmed();
        if (filename.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Please enter a filename."));
            return false;
        }

        QString fullPath = m_currentFolder.toLocalFile() + "/" + filename;
        if (QFileInfo::exists(fullPath)) {
            int ret = QMessageBox::question(this, tr("File Exists"),
                tr("The file \"%1\" already exists. Do you want to replace it?").arg(filename),
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                return false;
            }
        }
    }

    return true;
}

void FileChooserDialog::onAccept()
{
    m_selectedUrls.clear();

    if (m_directoryMode) {
        // Return the current folder
        m_selectedUrls.append(m_currentFolder);
    } else if (m_saveMode) {
        // Return the constructed save path
        if (!validateSelection()) return;

        QString filename = m_filenameEdit->text().trimmed();
        QString fullPath = m_currentFolder.toLocalFile() + "/" + filename;
        m_selectedUrls.append(QUrl::fromLocalFile(fullPath));
    } else {
        // Get selection from Pane - use the currently viewed file/folder
        QUrl currentUrl = m_pane->currentUrl();

        if (currentUrl.isValid()) {
            m_selectedUrls.append(currentUrl);
        }

        if (m_selectedUrls.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Please select a file."));
            return;
        }
    }

    accept();
}
