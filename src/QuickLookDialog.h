#pragma once
#include <QDialog>
#include <QString>
class QPushButton;
class QStackedWidget;
class QLabel;
class QShortcut;
class Pane;

class QuickLookDialog : public QDialog {
    Q_OBJECT
public:
    explicit QuickLookDialog(Pane *parentPane = nullptr);
    void showFile(const QString &path);

private slots:
    void navigateNext();
    void navigatePrevious();

private:
    void showImage(const QString &path);
    void showPdf(const QString &path);
    void showText(const QString &path);
    void clearView();

    Pane *pane = nullptr;
    QString currentFilePath;
    QPushButton *closeBtn = nullptr;
    QPushButton *prevBtn = nullptr;
    QPushButton *nextBtn = nullptr;
    QStackedWidget *stack = nullptr;
    QLabel *info = nullptr;
    QShortcut *escShortcut = nullptr;
    QShortcut *spaceShortcut = nullptr;
    QShortcut *leftShortcut = nullptr;
    QShortcut *rightShortcut = nullptr;
};
