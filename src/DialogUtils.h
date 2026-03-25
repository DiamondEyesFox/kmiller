#pragma once

#include <QSize>
#include <QString>

class QDialog;
class QWidget;

namespace DialogUtils {

QString finderDialogStyleSheet();
void prepareModalDialog(QDialog *dialog, QWidget *owner, const QString &title, const QSize &fixedSize);
void presentDialog(QDialog *dialog, QWidget *focusTarget = nullptr);
bool promptTextInput(QWidget *owner,
                     const QString &title,
                     const QString &label,
                     const QString &initialText,
                     QString *text,
                     int selectionStart = 0,
                     int selectionLength = -1);

}
