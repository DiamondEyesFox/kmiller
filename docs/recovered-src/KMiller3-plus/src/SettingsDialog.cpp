#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Preferences");
    auto *lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel("Preferences stub — add options here."));
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    lay->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
}
