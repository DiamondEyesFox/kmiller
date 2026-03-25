#pragma once
#include <QDialog>

class QCheckBox;
class QSpinBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent=nullptr);
};
