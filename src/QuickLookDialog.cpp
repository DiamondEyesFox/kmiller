#include "QuickLookDialog.h"
#include "Pane.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QImageReader>
#include <QPixmap>
#include <QScrollArea>
#include <QTextEdit>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QShortcut>
#include <QFile>
#include <QDir>
#include <QScreen>
#include <QGuiApplication>
#include <poppler-qt6.h>

QuickLookDialog::QuickLookDialog(Pane *parentPane) : QDialog(parentPane), pane(parentPane) {
    // Frameless, dark, minimal - like macOS Quick Look
    // Use Tool flag so it floats above main window but doesn't steal focus
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_ShowWithoutActivating, true);  // Don't steal focus when shown
    setModal(false);
    resize(800, 600);

    // Dark background with minimal padding
    setStyleSheet(
        "QuickLookDialog { background-color: #1e1e1e; border: 1px solid #3a3a3a; border-radius: 8px; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);

    // Filename at top - subtle, small
    filenameLabel = new QLabel(this);
    filenameLabel->setAlignment(Qt::AlignCenter);
    filenameLabel->setStyleSheet(
        "QLabel { color: #cccccc; background-color: #2a2a2a; padding: 6px; font-size: 12px; "
        "border-top-left-radius: 8px; border-top-right-radius: 8px; }"
    );
    layout->addWidget(filenameLabel);

    // Content stack
    stack = new QStackedWidget(this);
    stack->setStyleSheet("QStackedWidget { background-color: #1e1e1e; }");
    layout->addWidget(stack, 1);

    // Image view - no scroll, just centered label
    auto *imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("QLabel { background-color: #1e1e1e; }");
    stack->addWidget(imageLabel);  // 0

    // PDF view
    auto *pdfLabel = new QLabel;
    pdfLabel->setAlignment(Qt::AlignCenter);
    pdfLabel->setStyleSheet("QLabel { background-color: #1e1e1e; }");
    auto *pdfScroll = new QScrollArea;
    pdfScroll->setWidget(pdfLabel);
    pdfScroll->setWidgetResizable(true);
    pdfScroll->setStyleSheet("QScrollArea { background-color: #1e1e1e; border: none; }");
    stack->addWidget(pdfScroll);  // 1

    // Text view
    auto *textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; color: #e0e0e0; border: none; "
        "font-family: monospace; font-size: 13px; padding: 8px; }"
    );
    stack->addWidget(textEdit);  // 2

    // Empty/unsupported
    auto *emptyLabel = new QLabel("No preview available");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("QLabel { color: #888888; background-color: #1e1e1e; }");
    stack->addWidget(emptyLabel);  // 3

    // Keyboard shortcuts - Space/Esc to close, Up/Down to navigate
    escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &QDialog::close);

    spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(spaceShortcut, &QShortcut::activated, this, &QDialog::close);

    upShortcut = new QShortcut(QKeySequence(Qt::Key_Up), this);
    connect(upShortcut, &QShortcut::activated, this, &QuickLookDialog::navigatePrevious);

    downShortcut = new QShortcut(QKeySequence(Qt::Key_Down), this);
    connect(downShortcut, &QShortcut::activated, this, &QuickLookDialog::navigateNext);
}

void QuickLookDialog::showImage(const QString &path) {
    auto *label = qobject_cast<QLabel*>(stack->widget(0));
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage img = reader.read();
    if (img.isNull()) { stack->setCurrentIndex(3); return; }

    // Scale to fit dialog while preserving aspect ratio
    int availableWidth = width() - 2;
    int availableHeight = height() - filenameLabel->height() - 2;

    QPixmap pixmap = QPixmap::fromImage(img);
    if (pixmap.width() > availableWidth || pixmap.height() > availableHeight) {
        pixmap = pixmap.scaled(availableWidth, availableHeight,
                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    label->setPixmap(pixmap);
    stack->setCurrentIndex(0);
}

void QuickLookDialog::showPdf(const QString &path) {
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
    if (!doc) { stack->setCurrentIndex(3); return; }
    doc->setRenderHint(Poppler::Document::Antialiasing);
    doc->setRenderHint(Poppler::Document::TextAntialiasing);
    std::unique_ptr<Poppler::Page> page(doc->page(0));
    if (!page) { stack->setCurrentIndex(3); return; }

    const QImage img = page->renderToImage(150.0, 150.0);
    auto *scroll = qobject_cast<QScrollArea*>(stack->widget(1));
    auto *label = qobject_cast<QLabel*>(scroll->widget());
    label->setPixmap(QPixmap::fromImage(img));
    label->adjustSize();
    stack->setCurrentIndex(1);
}

void QuickLookDialog::showText(const QString &path) {
    auto *te = qobject_cast<QTextEdit*>(stack->widget(2));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        stack->setCurrentIndex(3);
        return;
    }
    // Limit text preview to first 100KB
    QByteArray data = f.read(100 * 1024);
    te->setPlainText(QString::fromUtf8(data));
    stack->setCurrentIndex(2);
}

void QuickLookDialog::showFile(const QString &path) {
    currentFilePath = path;
    QFileInfo fi(path);
    filenameLabel->setText(fi.fileName());

    QMimeDatabase db;
    const auto mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    const QString mime = mt.name();

    if (mime.startsWith("image/")) {
        showImage(path);
    } else if (mime == "application/pdf") {
        showPdf(path);
    } else if (mime.startsWith("text/") || mime.contains("json") || mime.contains("xml") ||
               mime.contains("javascript") || mime.contains("x-shellscript")) {
        showText(path);
    } else {
        // Try as image, fallback to text
        QImageReader test(path);
        if (test.canRead()) {
            showImage(path);
        } else {
            showText(path);
        }
    }

    show();
    raise();
    // Don't call activateWindow() - let main window keep focus so arrow keys work
}

void QuickLookDialog::navigateNext() {
    if (currentFilePath.isEmpty() || !pane) return;

    QFileInfo fi(currentFilePath);
    QDir dir(fi.absolutePath());
    QStringList files = dir.entryList(QDir::Files, QDir::Name);
    int idx = files.indexOf(fi.fileName());

    if (idx >= 0 && idx < files.count() - 1) {
        showFile(dir.absoluteFilePath(files[idx + 1]));
    }
}

void QuickLookDialog::navigatePrevious() {
    if (currentFilePath.isEmpty() || !pane) return;

    QFileInfo fi(currentFilePath);
    QDir dir(fi.absolutePath());
    QStringList files = dir.entryList(QDir::Files, QDir::Name);
    int idx = files.indexOf(fi.fileName());

    if (idx > 0) {
        showFile(dir.absoluteFilePath(files[idx - 1]));
    }
}
