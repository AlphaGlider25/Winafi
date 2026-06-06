#ifndef ADVANCED_FORMAT_DIALOG_H
#define ADVANCED_FORMAT_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include "../platform/linux/session.h"

class AdvancedFormatDialog : public QDialog {
    Q_OBJECT

public:
    explicit AdvancedFormatDialog(winafi_session_t *session, QWidget *parent = nullptr);
    ~AdvancedFormatDialog();

private slots:
    void onAccepted();

private:
    void setupUI();

    winafi_session_t *session_;
    QComboBox *image_option_combo_;
    QLabel *info_label_;
};

#endif // ADVANCED_FORMAT_DIALOG_H
