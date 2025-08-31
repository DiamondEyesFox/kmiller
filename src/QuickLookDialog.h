#pragma once
#include <QDialog>
#include <QString>
class QPushButton;
class QStackedWidget;
class QLabel;
class QShortcut;

class QuickLookDialog : public QDialog {
    Q_OBJECT
public:
    explicit QuickLookDialog(QWidget *parent = nullptr);
    void showFile(const QString &path);

private:
    void showImage(const QString &path);
    void showPdf(const QString &path);
    void showText(const QString &path);
    void clearView();

    QPushButton *closeBtn = nullptr;
    QStackedWidget *stack = nullptr;
    QLabel *info = nullptr;
    QShortcut *escShortcut = nullptr;
    QShortcut *spaceShortcut = nullptr;
};
