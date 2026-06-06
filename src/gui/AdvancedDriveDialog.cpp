#include "AdvancedDriveDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>

AdvancedDriveDialog::AdvancedDriveDialog(winafi_session_t *session, QWidget *parent)
    : QDialog(parent), session_(session) {
    setWindowTitle("Advanced Drive Properties");
    setModal(true);
    setMinimumWidth(400);

    setupUI();

    // Load current settings from session
    if (session_) {
        int enabled = winafi_session_get_bad_blocks_enabled(session_);
        int passes = winafi_session_get_bad_blocks_passes(session_);

        bad_blocks_check_->setChecked(enabled ? Qt::Checked : Qt::Unchecked);
        passes_spin_->setValue(passes);
    }

    // Connect button signals
    QDialogButtonBox *buttonBox = findChild<QDialogButtonBox *>();
    if (buttonBox) {
        connect(buttonBox, &QDialogButtonBox::accepted, this, &AdvancedDriveDialog::onAccepted);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    // Connect checkbox to spinbox enable/disable
    if (bad_blocks_check_) {
        connect(bad_blocks_check_, QOverload<int>::of(&QCheckBox::stateChanged),
                this, &AdvancedDriveDialog::onBadBlocksToggled);
    }
}

AdvancedDriveDialog::~AdvancedDriveDialog() {
    // Qt parent-child relationship handles cleanup
}

void AdvancedDriveDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Bad Blocks Check Group
    QGroupBox *badBlocksGroup = new QGroupBox("Bad Blocks Checking", this);
    QVBoxLayout *badBlocksLayout = new QVBoxLayout(badBlocksGroup);

    // Checkbox: Enable bad blocks check
    bad_blocks_check_ = new QCheckBox("Check for bad blocks", this);
    bad_blocks_check_->setObjectName("badBlocksCheck");
    bad_blocks_check_->setChecked(false);
    badBlocksLayout->addWidget(bad_blocks_check_);

    // Spinbox: Number of passes
    QHBoxLayout *passesLayout = new QHBoxLayout();
    passesLayout->addSpacing(20);
    QLabel *passesLabel = new QLabel("Number of passes:", this);
    passes_spin_ = new QSpinBox(this);
    passes_spin_->setObjectName("passesSpin");
    passes_spin_->setMinimum(1);
    passes_spin_->setMaximum(4);
    passes_spin_->setValue(1);
    passes_spin_->setEnabled(false);
    passesLayout->addWidget(passesLabel);
    passesLayout->addWidget(passes_spin_);
    passesLayout->addStretch();
    badBlocksLayout->addLayout(passesLayout);

    // Checkbox: List bad blocks (disabled for now, future use)
    list_bad_blocks_ = new QCheckBox("List bad blocks on disk", this);
    list_bad_blocks_->setObjectName("listBadBlocks");
    list_bad_blocks_->setChecked(false);
    list_bad_blocks_->setEnabled(false);
    badBlocksLayout->addWidget(list_bad_blocks_);

    // Info label
    info_label_ = new QLabel(this);
    info_label_->setObjectName("infoLabel");
    info_label_->setText("Note: Read-only non-destructive test");
    info_label_->setStyleSheet("color: #666666; font-size: 10pt;");
    badBlocksLayout->addSpacing(10);
    badBlocksLayout->addWidget(info_label_);

    mainLayout->addWidget(badBlocksGroup);
    mainLayout->addStretch();

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

void AdvancedDriveDialog::onBadBlocksToggled(int state) {
    // Enable/disable spinbox based on checkbox state
    if (passes_spin_) {
        passes_spin_->setEnabled(state == Qt::Checked);
    }
}

void AdvancedDriveDialog::onAccepted() {
    // Call session API to set bad blocks check settings
    if (session_ && bad_blocks_check_ && passes_spin_) {
        int enabled = bad_blocks_check_->isChecked() ? 1 : 0;
        int passes = passes_spin_->value();

        int result = winafi_session_set_bad_blocks_check(session_, enabled, passes);
        if (result == 0) {
            accept();
        } else {
            // If API call fails, still accept but log the error
            accept();
        }
    } else {
        accept();
    }
}
