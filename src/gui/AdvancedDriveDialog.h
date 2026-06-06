#ifndef ADVANCED_DRIVE_DIALOG_H
#define ADVANCED_DRIVE_DIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include "../platform/linux/session.h"

class AdvancedDriveDialog : public QDialog {
    Q_OBJECT

public:
    explicit AdvancedDriveDialog(winafi_session_t *session, QWidget *parent = nullptr);
    ~AdvancedDriveDialog();

private slots:
    void onBadBlocksToggled(int state);
    void onAccepted();

private:
    void setupUI();

    winafi_session_t *session_;
    QCheckBox *bad_blocks_check_;
    QSpinBox *passes_spin_;
    QCheckBox *list_bad_blocks_;
    QLabel *info_label_;
};

#endif // ADVANCED_DRIVE_DIALOG_H
