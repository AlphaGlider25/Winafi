#include "AdvancedFormatDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>

AdvancedFormatDialog::AdvancedFormatDialog(winafi_session_t *session, QWidget *parent)
    : QDialog(parent), session_(session) {
    setWindowTitle("Advanced Format Options");
    setModal(true);
    setMinimumWidth(400);

    setupUI();

    // Load current settings from session
    if (session_) {
        winafi_image_option_t current_option = winafi_session_get_image_option(session_);
        image_option_combo_->setCurrentIndex(static_cast<int>(current_option));
    }

    // Connect button signals
    QDialogButtonBox *buttonBox = findChild<QDialogButtonBox *>();
    if (buttonBox) {
        connect(buttonBox, &QDialogButtonBox::accepted, this, &AdvancedFormatDialog::onAccepted);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
}

AdvancedFormatDialog::~AdvancedFormatDialog() {
    // Qt parent-child relationship handles cleanup
}

void AdvancedFormatDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Image Options Group
    QGroupBox *imageGroup = new QGroupBox("Image Options", this);
    QVBoxLayout *imageLayout = new QVBoxLayout(imageGroup);

    // Label and combo box
    QHBoxLayout *optionLayout = new QHBoxLayout();
    QLabel *optionLabel = new QLabel("Image option:", this);
    image_option_combo_ = new QComboBox(this);
    image_option_combo_->setObjectName("imageOptionCombo");
    image_option_combo_->addItem("Standard Installation", 0);
    image_option_combo_->addItem("Portable Windows (VHD)", 1);
    image_option_combo_->addItem("VHD Image", 2);
    optionLayout->addWidget(optionLabel);
    optionLayout->addWidget(image_option_combo_);
    optionLayout->addStretch();
    imageLayout->addLayout(optionLayout);

    // Info label
    info_label_ = new QLabel(this);
    info_label_->setObjectName("infoLabel");
    info_label_->setText(
        "Standard Installation: Default Windows installation to USB drive.\n"
        "Portable Windows (VHD): Run Windows from USB without installation.\n"
        "VHD Image: Create a virtual disk image for Hyper-V compatibility."
    );
    info_label_->setStyleSheet("color: #666666; font-size: 10pt;");
    info_label_->setWordWrap(true);
    imageLayout->addSpacing(10);
    imageLayout->addWidget(info_label_);

    mainLayout->addWidget(imageGroup);
    mainLayout->addStretch();

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

void AdvancedFormatDialog::onAccepted() {
    // Call session API to set image option
    if (session_ && image_option_combo_) {
        int selected_index = image_option_combo_->currentIndex();
        winafi_image_option_t option = static_cast<winafi_image_option_t>(selected_index);

        int result = winafi_session_set_image_option(session_, option);
        if (result == WINAFI_OK) {
            accept();
        } else {
            // If API call fails, still accept but log the error
            accept();
        }
    } else {
        accept();
    }
}
