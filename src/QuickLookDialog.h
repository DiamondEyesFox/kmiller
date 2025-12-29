#pragma once
#include <QDialog>
#include <QString>
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

    Pane *pane = nullptr;
    QString currentFilePath;
    QStackedWidget *stack = nullptr;
    QLabel *filenameLabel = nullptr;
    QShortcut *escShortcut = nullptr;
    QShortcut *spaceShortcut = nullptr;
    QShortcut *upShortcut = nullptr;
    QShortcut *downShortcut = nullptr;
};
