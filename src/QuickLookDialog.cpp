#include "QuickLookDialog.h"
#include "Pane.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
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
#include <poppler-qt6.h>

static QWidget* makeScroll(QWidget *child) {
    auto *sa = new QScrollArea;
    sa->setWidgetResizable(true);
    child->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sa->setWidget(child);
    return sa;
}

QuickLookDialog::QuickLookDialog(Pane *parentPane) : QDialog(parentPane), pane(parentPane) {
    setWindowTitle(QStringLiteral("Quick Look"));
    setModal(false); // modeless
    setWindowFlag(Qt::WindowCloseButtonHint, true);
    resize(900, 700);

    auto *v = new QVBoxLayout(this);
    auto *top = new QHBoxLayout();

    info = new QLabel(this);
    info->setMinimumHeight(24);

    prevBtn = new QPushButton(QStringLiteral("◀ Previous"), this);
    nextBtn = new QPushButton(QStringLiteral("Next ▶"), this);
    closeBtn = new QPushButton(QStringLiteral("Close"), this);
    
    connect(prevBtn, &QPushButton::clicked, this, &QuickLookDialog::navigatePrevious);
    connect(nextBtn, &QPushButton::clicked, this, &QuickLookDialog::navigateNext);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    top->addWidget(info, 1);
    top->addWidget(prevBtn, 0);
    top->addWidget(nextBtn, 0);
    top->addWidget(closeBtn, 0);
    v->addLayout(top);

    stack = new QStackedWidget(this);
    v->addWidget(stack, 1);

    // Pages: 0=image, 1=pdf, 2=text, 3=empty
    auto *imageLabel = new QLabel;      imageLabel->setAlignment(Qt::AlignCenter);
    auto *pdfLabel   = new QLabel;      pdfLabel->setAlignment(Qt::AlignCenter);
    auto *textEdit   = new QTextEdit;   textEdit->setReadOnly(true);
    auto *emptyInfo  = new QLabel(QStringLiteral("No preview")); emptyInfo->setAlignment(Qt::AlignCenter);

    stack->addWidget(makeScroll(imageLabel)); // 0
    stack->addWidget(makeScroll(pdfLabel));   // 1
    stack->addWidget(textEdit);               // 2
    stack->addWidget(emptyInfo);              // 3

    // Keyboard shortcuts
    escShortcut = new QShortcut(QKeySequence::Cancel, this);
    connect(escShortcut, &QShortcut::activated, this, &QDialog::close);
    spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(spaceShortcut, &QShortcut::activated, this, &QDialog::close);
    leftShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(leftShortcut, &QShortcut::activated, this, &QuickLookDialog::navigatePrevious);
    rightShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(rightShortcut, &QShortcut::activated, this, &QuickLookDialog::navigateNext);
    upShortcut = new QShortcut(QKeySequence(Qt::Key_Up), this);
    connect(upShortcut, &QShortcut::activated, this, &QuickLookDialog::navigatePrevious);
    downShortcut = new QShortcut(QKeySequence(Qt::Key_Down), this);
    connect(downShortcut, &QShortcut::activated, this, &QuickLookDialog::navigateNext);
}

void QuickLookDialog::clearView() {
    // keep widgets; we just swap content/pages
}

void QuickLookDialog::showImage(const QString &path) {
    auto *sa = qobject_cast<QScrollArea*>(stack->widget(0));
    auto *label = qobject_cast<QLabel*>(sa->widget());
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage img = reader.read();
    if (img.isNull()) { stack->setCurrentIndex(3); return; }
    
    // Calculate available space (dialog size minus margins/chrome)
    int availableWidth = width() - 40;   // Account for margins
    int availableHeight = height() - 80; // Account for title bar, info bar, margins
    
    QPixmap pixmap = QPixmap::fromImage(img);
    
    // Scale image to fit available space while maintaining aspect ratio and quality
    if (pixmap.width() > availableWidth || pixmap.height() > availableHeight) {
        pixmap = pixmap.scaled(availableWidth, availableHeight, 
                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    label->setPixmap(pixmap);
    label->adjustSize();
    stack->setCurrentIndex(0);
}

void QuickLookDialog::showPdf(const QString &path) {
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
    if (!doc) { stack->setCurrentIndex(3); return; }
    std::unique_ptr<Poppler::Page> page(doc->page(0));
    if (!page) { stack->setCurrentIndex(3); return; }
    const QImage img = page->renderToImage(192.0, 192.0);
    auto *sa = qobject_cast<QScrollArea*>(stack->widget(1));
    auto *label = qobject_cast<QLabel*>(sa->widget());
    label->setPixmap(QPixmap::fromImage(img));
    label->adjustSize();
    stack->setCurrentIndex(1);
}

void QuickLookDialog::showText(const QString &path) {
    auto *te = qobject_cast<QTextEdit*>(stack->widget(2));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly|QIODevice::Text)) { stack->setCurrentIndex(3); return; }
    te->setPlainText(QString::fromUtf8(f.readAll()));
    stack->setCurrentIndex(2);
}

void QuickLookDialog::showFile(const QString &path) {
    currentFilePath = path;
    QFileInfo fi(path);
    info->setText(fi.fileName());
    clearView();

    // Enable/disable navigation buttons and shortcuts based on file availability and view mode
    if (pane) {
        QDir dir(fi.absolutePath());
        QStringList files = dir.entryList(QDir::Files, QDir::Name);
        int currentIndex = files.indexOf(fi.fileName());
        
        bool canGoPrev = currentIndex > 0;
        bool canGoNext = currentIndex >= 0 && currentIndex < files.count() - 1;
        
        prevBtn->setEnabled(canGoPrev);
        nextBtn->setEnabled(canGoNext);
        
        // Enable/disable shortcuts based on view mode
        int viewMode = pane->currentViewMode();
        bool isMillerView = (viewMode == 3);
        
        // In miller view, use up/down keys; in other views, use left/right keys
        leftShortcut->setEnabled(!isMillerView && canGoPrev);
        rightShortcut->setEnabled(!isMillerView && canGoNext);
        upShortcut->setEnabled(isMillerView && canGoPrev);
        downShortcut->setEnabled(isMillerView && canGoNext);
    } else {
        prevBtn->setEnabled(false);
        nextBtn->setEnabled(false);
        leftShortcut->setEnabled(false);
        rightShortcut->setEnabled(false);
        upShortcut->setEnabled(false);
        downShortcut->setEnabled(false);
    }

    QMimeDatabase db;
    const auto mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    const QString name = mt.name();

    if (name.startsWith("image/")) {
        showImage(path);
    } else if (name == "application/pdf") {
        showPdf(path);
    } else if (name.startsWith("text/") || name.contains("json") || name.contains("xml")) {
        showText(path);
    } else {
        QImageReader test(path);
        if (test.canRead()) showImage(path);
        else showText(path);
    }
    show(); raise(); activateWindow();
}

void QuickLookDialog::navigateNext() {
    if (currentFilePath.isEmpty() || !pane) return;
    
    QFileInfo fi(currentFilePath);
    QDir dir(fi.absolutePath());
    QStringList files = dir.entryList(QDir::Files, QDir::Name);
    int currentIndex = files.indexOf(fi.fileName());
    
    if (currentIndex >= 0 && currentIndex < files.count() - 1) {
        QString nextFile = dir.absoluteFilePath(files[currentIndex + 1]);
        showFile(nextFile);
    }
}

void QuickLookDialog::navigatePrevious() {
    if (currentFilePath.isEmpty() || !pane) return;
    
    QFileInfo fi(currentFilePath);
    QDir dir(fi.absolutePath());
    QStringList files = dir.entryList(QDir::Files, QDir::Name);
    int currentIndex = files.indexOf(fi.fileName());
    
    if (currentIndex > 0) {
        QString prevFile = dir.absoluteFilePath(files[currentIndex - 1]);
        showFile(prevFile);
    }
}
