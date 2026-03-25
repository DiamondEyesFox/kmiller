#include "PropertiesDialog.h"
#include "FileOpsService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QProgressBar>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QMimeDatabase>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QDirIterator>
#include <QLocale>
#include <QStyle>
#include <QtConcurrent/QtConcurrentRun>
#include <KJob>
#include <KIO/SimpleJob>

namespace {

QString formatBytes(qint64 size) {
    const QStringList units = {"bytes", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(qMax<qint64>(0, size));
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < units.size() - 1) {
        value /= 1024.0;
        ++unitIndex;
    }

    if (unitIndex == 0) {
        return QString("%1 %2").arg(static_cast<qint64>(value)).arg(units[unitIndex]);
    }
    return QString("%1 %2").arg(value, 0, 'f', 1).arg(units[unitIndex]);
}

QString formatSizeLabel(qint64 size) {
    return PropertiesDialog::tr("%1 (%2 bytes)")
        .arg(formatBytes(size))
        .arg(QLocale().toString(size));
}

qint64 computeDirectorySizeBytes(const QString &dirPath) {
    qint64 totalSize = 0;
    QDirIterator it(
        dirPath,
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden | QDir::System,
        QDirIterator::Subdirectories
    );

    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        if (fi.isFile()) {
            totalSize += fi.size();
        }
    }
    return totalSize;
}

} // namespace

PropertiesDialog::PropertiesDialog(const QUrl &url, QWidget *parent)
    : QDialog(parent), m_url(url), m_multipleFiles(false)
{
    m_urls = {url};
    setupUI();
    loadFileInfo();
}

PropertiesDialog::PropertiesDialog(const QList<QUrl> &urls, QWidget *parent)
    : QDialog(parent), m_urls(urls), m_multipleFiles(urls.size() > 1)
{
    if (!urls.isEmpty()) {
        m_url = urls.first();
    }
    setupUI();
    if (m_multipleFiles) {
        loadMultipleFilesInfo();
    } else {
        loadFileInfo();
    }
}

void PropertiesDialog::setupUI() {
    setWindowTitle(m_multipleFiles ? tr("Properties - %1 items").arg(m_urls.size()) 
                                   : tr("Properties"));
    setModal(true);
    resize(400, 300);

    auto *mainLayout = new QVBoxLayout(this);
    
    m_tabs = new QTabWidget(this);
    mainLayout->addWidget(m_tabs);
    
    setupGeneralTab();
    setupPermissionsTab();
    
    // Buttons
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    
    m_okButton = new QPushButton(tr("OK"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_applyButton = new QPushButton(tr("Apply"), this);

    if (m_multipleFiles) {
        m_okButton->setText(tr("Close"));
        m_applyButton->setEnabled(false);
    }
    
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(m_okButton, &QPushButton::clicked, this, [this]{
        if (applyChangesImpl()) {
            accept();
        }
    });
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &PropertiesDialog::applyChanges);
}

void PropertiesDialog::setupGeneralTab() {
    m_generalTab = new QWidget;
    m_tabs->addTab(m_generalTab, tr("General"));
    
    auto *layout = new QGridLayout(m_generalTab);
    
    // Icon and name
    auto *topLayout = new QHBoxLayout;
    m_iconLabel = new QLabel;
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setScaledContents(true);
    topLayout->addWidget(m_iconLabel);
    
    m_nameEdit = new QLineEdit;
    topLayout->addWidget(m_nameEdit, 1);
    
    layout->addLayout(topLayout, 0, 0, 1, 2);
    
    int row = 1;
    
    // Type
    layout->addWidget(new QLabel(tr("Type:")), row, 0);
    m_typeLabel = new QLabel;
    layout->addWidget(m_typeLabel, row++, 1);
    
    // Size
    layout->addWidget(new QLabel(tr("Size:")), row, 0);
    m_sizeLabel = new QLabel;
    layout->addWidget(m_sizeLabel, row++, 1);
    
    // Location
    layout->addWidget(new QLabel(tr("Location:")), row, 0);
    m_locationLabel = new QLabel;
    m_locationLabel->setWordWrap(true);
    layout->addWidget(m_locationLabel, row++, 1);
    
    // Created
    layout->addWidget(new QLabel(tr("Created:")), row, 0);
    m_createdLabel = new QLabel;
    layout->addWidget(m_createdLabel, row++, 1);
    
    // Modified
    layout->addWidget(new QLabel(tr("Modified:")), row, 0);
    m_modifiedLabel = new QLabel;
    layout->addWidget(m_modifiedLabel, row++, 1);
    
    // Accessed
    layout->addWidget(new QLabel(tr("Accessed:")), row, 0);
    m_accessedLabel = new QLabel;
    layout->addWidget(m_accessedLabel, row++, 1);
    
    layout->setRowStretch(row, 1);
}

void PropertiesDialog::setupPermissionsTab() {
    m_permissionsTab = new QWidget;
    m_tabs->addTab(m_permissionsTab, tr("Permissions"));
    
    auto *layout = new QVBoxLayout(m_permissionsTab);
    
    // Owner/Group info
    auto *ownerLayout = new QGridLayout;
    ownerLayout->addWidget(new QLabel(tr("Owner:")), 0, 0);
    m_ownerLabel = new QLabel;
    ownerLayout->addWidget(m_ownerLabel, 0, 1);
    
    ownerLayout->addWidget(new QLabel(tr("Group:")), 1, 0);
    m_groupLabel = new QLabel;
    ownerLayout->addWidget(m_groupLabel, 1, 1);
    
    layout->addLayout(ownerLayout);
    
    // Permissions grid
    auto *permGroup = new QGroupBox(tr("Permissions"));
    layout->addWidget(permGroup);
    
    auto *permLayout = new QGridLayout(permGroup);
    
    // Headers
    permLayout->addWidget(new QLabel(tr("Read")), 0, 1);
    permLayout->addWidget(new QLabel(tr("Write")), 0, 2);
    permLayout->addWidget(new QLabel(tr("Execute")), 0, 3);
    
    // Owner
    permLayout->addWidget(new QLabel(tr("Owner")), 1, 0);
    m_ownerRead = new QCheckBox;
    m_ownerWrite = new QCheckBox;
    m_ownerExecute = new QCheckBox;
    permLayout->addWidget(m_ownerRead, 1, 1);
    permLayout->addWidget(m_ownerWrite, 1, 2);
    permLayout->addWidget(m_ownerExecute, 1, 3);
    
    // Group
    permLayout->addWidget(new QLabel(tr("Group")), 2, 0);
    m_groupRead = new QCheckBox;
    m_groupWrite = new QCheckBox;
    m_groupExecute = new QCheckBox;
    permLayout->addWidget(m_groupRead, 2, 1);
    permLayout->addWidget(m_groupWrite, 2, 2);
    permLayout->addWidget(m_groupExecute, 2, 3);
    
    // Other
    permLayout->addWidget(new QLabel(tr("Other")), 3, 0);
    m_otherRead = new QCheckBox;
    m_otherWrite = new QCheckBox;
    m_otherExecute = new QCheckBox;
    permLayout->addWidget(m_otherRead, 3, 1);
    permLayout->addWidget(m_otherWrite, 3, 2);
    permLayout->addWidget(m_otherExecute, 3, 3);
    
    layout->addStretch();
}

void PropertiesDialog::loadFileInfo() {
    if (!m_url.isLocalFile()) return;
    
    QString path = m_url.toLocalFile();
    QFileInfo info(path);
    
    if (!info.exists()) {
        QMessageBox::warning(this, tr("Error"), tr("File does not exist: %1").arg(path));
        return;
    }
    
    // Icon
    QFileIconProvider iconProvider;
    QIcon icon = iconProvider.icon(info);
    m_iconLabel->setPixmap(icon.pixmap(48, 48));
    
    // Name
    m_nameEdit->setText(info.fileName());
    
    // Type
    QMimeDatabase mimeDb;
    auto mimeType = mimeDb.mimeTypeForFile(info);
    QString typeStr = mimeType.comment();
    if (info.isDir()) {
        typeStr = tr("Folder");
    } else if (typeStr.isEmpty()) {
        typeStr = tr("File");
    }
    m_typeLabel->setText(typeStr);
    
    // Size
    if (info.isDir()) {
        m_sizeLabel->setText(tr("Calculating folder size..."));
        m_sizeLabel->setToolTip(QString());

        auto *watcher = new QFutureWatcher<qint64>(this);
        connect(watcher, &QFutureWatcher<qint64>::finished, this, [this, watcher]() {
            const qint64 size = watcher->result();
            m_sizeLabel->setText(formatSizeLabel(size));
            m_sizeLabel->setToolTip(QLocale().toString(size));
            watcher->deleteLater();
        });
        watcher->setFuture(QtConcurrent::run([path]() {
            return computeDirectorySizeBytes(path);
        }));
    } else {
        const qint64 size = info.size();
        m_sizeLabel->setText(formatSizeLabel(size));
        m_sizeLabel->setToolTip(QLocale().toString(size));
    }
    
    // Location
    m_locationLabel->setText(info.absolutePath());
    
    // Dates
    m_createdLabel->setText(info.birthTime().toString());
    m_modifiedLabel->setText(info.lastModified().toString());
    m_accessedLabel->setText(info.lastRead().toString());
    
    // Permissions
    QFile::Permissions perms = info.permissions();
    m_ownerRead->setChecked(perms & QFile::ReadOwner);
    m_ownerWrite->setChecked(perms & QFile::WriteOwner);
    m_ownerExecute->setChecked(perms & QFile::ExeOwner);
    m_groupRead->setChecked(perms & QFile::ReadGroup);
    m_groupWrite->setChecked(perms & QFile::WriteGroup);
    m_groupExecute->setChecked(perms & QFile::ExeGroup);
    m_otherRead->setChecked(perms & QFile::ReadOther);
    m_otherWrite->setChecked(perms & QFile::WriteOther);
    m_otherExecute->setChecked(perms & QFile::ExeOther);
    
    // Owner/Group
    m_ownerLabel->setText(info.owner());
    m_groupLabel->setText(info.group());
}

void PropertiesDialog::loadMultipleFilesInfo() {
    // Set generic icon for multiple files
    m_iconLabel->setPixmap(style()->standardIcon(QStyle::SP_FileIcon).pixmap(48, 48));
    
    // Name field shows count
    m_nameEdit->setText(tr("%1 items selected").arg(m_urls.size()));
    m_nameEdit->setReadOnly(true);
    
    // Calculate total size and common properties
    qint64 directFileSize = 0;
    QStringList directoryPaths;
    int fileCount = 0, folderCount = 0;
    
    for (const QUrl &url : m_urls) {
        if (!url.isLocalFile()) continue;
        QFileInfo info(url.toLocalFile());
        if (!info.exists()) continue;
        
        if (info.isDir()) {
            folderCount++;
            directoryPaths.append(info.absoluteFilePath());
        } else {
            fileCount++;
            directFileSize += info.size();
        }
    }
    
    // Type
    QString typeStr;
    if (fileCount > 0 && folderCount > 0) {
        typeStr = tr("%1 files, %2 folders").arg(fileCount).arg(folderCount);
    } else if (fileCount > 0) {
        typeStr = tr("%1 files").arg(fileCount);
    } else {
        typeStr = tr("%1 folders").arg(folderCount);
    }
    m_typeLabel->setText(typeStr);
    
    // Size
    if (directoryPaths.isEmpty()) {
        m_sizeLabel->setText(formatSizeLabel(directFileSize));
        m_sizeLabel->setToolTip(QLocale().toString(directFileSize));
    } else {
        m_sizeLabel->setText(tr("Calculating selected folder sizes..."));
        m_sizeLabel->setToolTip(QString());

        auto *watcher = new QFutureWatcher<qint64>(this);
        connect(watcher, &QFutureWatcher<qint64>::finished, this, [this, watcher]() {
            const qint64 size = watcher->result();
            m_sizeLabel->setText(formatSizeLabel(size));
            m_sizeLabel->setToolTip(QLocale().toString(size));
            watcher->deleteLater();
        });
        watcher->setFuture(QtConcurrent::run([directFileSize, directoryPaths]() {
            qint64 totalSize = directFileSize;
            for (const QString &dirPath : directoryPaths) {
                totalSize += computeDirectorySizeBytes(dirPath);
            }
            return totalSize;
        }));
    }
    
    // Disable fields that don't make sense for multiple files
    m_locationLabel->setText(tr("Multiple locations"));
    m_createdLabel->setText(tr("Various dates"));
    m_modifiedLabel->setText(tr("Various dates"));
    m_accessedLabel->setText(tr("Various dates"));
    
    // Disable permission checkboxes for multiple files
    QList<QCheckBox*> checkBoxes = {
        m_ownerRead, m_ownerWrite, m_ownerExecute,
        m_groupRead, m_groupWrite, m_groupExecute,
        m_otherRead, m_otherWrite, m_otherExecute
    };
    
    for (QCheckBox *cb : checkBoxes) {
        cb->setEnabled(false);
    }
    
    m_ownerLabel->setText(tr("Various"));
    m_groupLabel->setText(tr("Various"));
}

void PropertiesDialog::applyChanges() {
    applyChangesImpl();
}

bool PropertiesDialog::applyChangesImpl() {
    if (m_multipleFiles) {
        // TODO: Handle bulk rename if needed
        return true;
    }
    
    if (!m_url.isLocalFile()) return false;
    
    QString path = m_url.toLocalFile();
    QFileInfo info(path);
    
    // Rename if name changed
    QString newName = m_nameEdit->text();
    if (newName != info.fileName() && !newName.isEmpty()) {
        if (newName == QStringLiteral(".") || newName == QStringLiteral("..") || newName.contains(QLatin1Char('/'))) {
            QMessageBox::warning(this, tr("Error"), tr("That name is not valid for a file or folder."));
            return false;
        }

        QString newPath = info.absolutePath() + "/" + newName;
        QEventLoop loop;
        KJob *job = FileOpsService::rename(m_url, QUrl::fromLocalFile(newPath), this);
        connect(job, &KJob::result, &loop, &QEventLoop::quit);
        loop.exec();

        if (job->error()) {
            return false;
        }

        // Update our URL for permission changes
        m_url = QUrl::fromLocalFile(newPath);
        path = newPath;
        info.setFile(path);
    }
    
    // Apply permissions
    QFile::Permissions perms;
    if (m_ownerRead->isChecked()) perms |= QFile::ReadOwner;
    if (m_ownerWrite->isChecked()) perms |= QFile::WriteOwner;
    if (m_ownerExecute->isChecked()) perms |= QFile::ExeOwner;
    if (m_groupRead->isChecked()) perms |= QFile::ReadGroup;
    if (m_groupWrite->isChecked()) perms |= QFile::WriteGroup;
    if (m_groupExecute->isChecked()) perms |= QFile::ExeGroup;
    if (m_otherRead->isChecked()) perms |= QFile::ReadOther;
    if (m_otherWrite->isChecked()) perms |= QFile::WriteOther;
    if (m_otherExecute->isChecked()) perms |= QFile::ExeOther;
    
    QFile file(path);
    if (!file.setPermissions(perms)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to change permissions"));
        return false;
    }

    loadFileInfo();
    return true;
}
