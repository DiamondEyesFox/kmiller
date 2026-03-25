#include "DialogUtils.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace DialogUtils {

QString finderDialogStyleSheet() {
    return QStringLiteral(
        "QDialog { "
            "background-color: rgba(42, 44, 49, 235); "
            "color: #e8eaed; "
            "border: 1px solid rgba(95, 102, 114, 200); "
            "border-radius: 12px; "
        "}"
        "QLabel { color: #e8eaed; background: transparent; }"
        "QPushButton { "
            "background-color: rgba(58, 62, 68, 235); "
            "color: #e8eaed; "
            "border: 1px solid #6f7785; "
            "border-radius: 6px; "
            "padding: 4px 12px; "
        "}"
        "QPushButton:hover { border-color: #8bbcff; }"
        "QPushButton:pressed { background-color: rgba(72, 77, 85, 245); }"
    );
}

void prepareModalDialog(QDialog *dialog, QWidget *owner, const QString &title, const QSize &fixedSize) {
    if (!dialog) {
        return;
    }

    dialog->setParent(owner);
    dialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setWindowTitle(title);
    dialog->setModal(true);
    dialog->setWindowModality(Qt::ApplicationModal);
    if (fixedSize.isValid()) {
        dialog->setFixedSize(fixedSize);
    }
    dialog->setStyleSheet(finderDialogStyleSheet());
}

void presentDialog(QDialog *dialog, QWidget *focusTarget) {
    if (!dialog) {
        return;
    }

    if (QWidget *owner = dialog->parentWidget()) {
        const QRect parentFrame = owner->window()->frameGeometry();
        const QPoint centeredTopLeft = parentFrame.center()
            - QPoint(dialog->width() / 2, dialog->height() / 2);
        dialog->move(centeredTopLeft);
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    if (focusTarget) {
        focusTarget->setFocus(Qt::OtherFocusReason);
    }
}

bool promptTextInput(QWidget *owner,
                     const QString &title,
                     const QString &label,
                     const QString &initialText,
                     QString *text,
                     int selectionStart,
                     int selectionLength) {
    if (!text) {
        return false;
    }

    QDialog dialog(owner);
    dialog.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setFixedSize(420, 130);
    dialog.setStyleSheet(
        finderDialogStyleSheet() +
        QStringLiteral(
            "QLineEdit { "
                "background-color: rgba(74, 78, 86, 240); "
                "color: #f5f7fa; "
                "border: 1px solid #6f7785; "
                "border-radius: 6px; "
                "padding: 4px 8px; "
                "selection-background-color: #4a90e2; "
            "}"
            "QLineEdit:focus { border-color: #6db3ff; }"
        ));

    auto *layout = new QVBoxLayout(&dialog);

    auto *labelWidget = new QLabel(label, &dialog);
    labelWidget->setWordWrap(true);
    layout->addWidget(labelWidget);

    auto *lineEdit = new QLineEdit(&dialog);
    lineEdit->setText(initialText);
    layout->addWidget(lineEdit);

    if (selectionStart < 0) {
        selectionStart = 0;
    }
    if (selectionStart > initialText.size()) {
        selectionStart = initialText.size();
    }
    if (selectionLength < 0) {
        selectionLength = initialText.size() - selectionStart;
    }
    lineEdit->setSelection(selectionStart, selectionLength);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(lineEdit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);
    layout->addWidget(buttonBox);

    if (owner) {
        const QRect parentFrame = owner->window()->frameGeometry();
        const QPoint centeredTopLeft = parentFrame.center()
            - QPoint(dialog.width() / 2, dialog.height() / 2);
        dialog.move(centeredTopLeft);
    }

    QTimer::singleShot(0, &dialog, [lineEdit]() {
        lineEdit->setFocus(Qt::OtherFocusReason);
    });

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    *text = lineEdit->text();
    return true;
}

}
