#include "FileChooserDialog.h"
#include "MillerView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

FileChooserDialog::FileChooserDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle(tr("Select File"));
    resize(900, 600);

    // Start in home directory
    setCurrentFolder(QDir::homePath());
}

FileChooserDialog::~FileChooserDialog() = default;

void FileChooserDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // Path label at top
    m_pathLabel = new QLabel(this);
    m_pathLabel->setStyleSheet("QLabel { color: #888; padding: 4px; }");
    mainLayout->addWidget(m_pathLabel);

    // Miller columns view (main area)
    m_millerView = new MillerView(this);
    m_millerView->setMinimumHeight(400);
    mainLayout->addWidget(m_millerView, 1);

    // Bottom bar: filename + filter + buttons
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->setSpacing(8);

    // Filename input (for save mode)
    m_filenameEdit = new QLineEdit(this);
    m_filenameEdit->setPlaceholderText(tr("Filename"));
    m_filenameEdit->setMinimumWidth(200);
    bottomLayout->addWidget(m_filenameEdit);

    // Filter dropdown
    m_filterCombo = new QComboBox(this);
    m_filterCombo->setMinimumWidth(150);
    m_filterCombo->addItem(tr("All Files (*)"));
    bottomLayout->addWidget(m_filterCombo);

    bottomLayout->addStretch();

    // Buttons
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_acceptButton = new QPushButton(tr("Open"), this);
    m_acceptButton->setDefault(true);

    bottomLayout->addWidget(m_cancelButton);
    bottomLayout->addWidget(m_acceptButton);

    mainLayout->addLayout(bottomLayout);

    // Connections
    connect(m_millerView, &MillerView::selectionChanged,
            this, &FileChooserDialog::onSelectionChanged);
    connect(m_millerView, &MillerView::navigatedTo,
            this, &FileChooserDialog::onNavigatedTo);
    connect(m_filenameEdit, &QLineEdit::textChanged,
            this, &FileChooserDialog::updateAcceptButton);
    connect(m_acceptButton, &QPushButton::clicked,
            this, &FileChooserDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);

    // Initial state
    updateAcceptButton();
}

void FileChooserDialog::setSaveMode(bool save)
{
    m_saveMode = save;
    m_filenameEdit->setVisible(save);
    m_acceptButton->setText(save ? tr("Save") : tr("Open"));
    updateAcceptButton();
}

void FileChooserDialog::setDirectoryMode(bool directory)
{
    m_directoryMode = directory;
    m_filenameEdit->setVisible(!directory && m_saveMode);
    if (directory) {
        m_acceptButton->setText(tr("Select Folder"));
    }
    updateAcceptButton();
}

void FileChooserDialog::setMultipleSelection(bool multiple)
{
    m_multipleSelection = multiple;
    // MillerView already supports extended selection
}

void FileChooserDialog::setCurrentFolder(const QString &path)
{
    m_currentFolder = QUrl::fromLocalFile(path);
    m_millerView->setRootUrl(m_currentFolder);
    m_pathLabel->setText(path);
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

void FileChooserDialog::onSelectionChanged(const QUrl &url)
{
    if (!url.isValid() || !url.isLocalFile()) return;

    QFileInfo fi(url.toLocalFile());

    if (!m_directoryMode && fi.isFile()) {
        // In file mode, update filename field with selected file
        if (m_saveMode) {
            m_filenameEdit->setText(fi.fileName());
        }
    }

    updateAcceptButton();
}

void FileChooserDialog::onNavigatedTo(const QUrl &url)
{
    if (!url.isValid() || !url.isLocalFile()) return;

    m_currentFolder = url;
    m_pathLabel->setText(url.toLocalFile());
}

void FileChooserDialog::updateAcceptButton()
{
    bool enabled = false;

    if (m_directoryMode) {
        // Can always select current folder
        enabled = m_currentFolder.isValid();
    } else if (m_saveMode) {
        // Need filename for save
        enabled = !m_filenameEdit->text().isEmpty();
    } else {
        // Need selection for open
        QList<QUrl> selected = m_millerView->getSelectedUrls();
        enabled = !selected.isEmpty();
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
        // Return selected files from MillerView
        m_selectedUrls = m_millerView->getSelectedUrls();

        // Filter out directories unless in directory mode
        if (!m_directoryMode) {
            QList<QUrl> filesOnly;
            for (const QUrl &url : m_selectedUrls) {
                if (url.isLocalFile()) {
                    QFileInfo fi(url.toLocalFile());
                    if (fi.isFile()) {
                        filesOnly.append(url);
                    }
                }
            }
            m_selectedUrls = filesOnly;
        }

        if (m_selectedUrls.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Please select a file."));
            return;
        }
    }

    accept();
}
