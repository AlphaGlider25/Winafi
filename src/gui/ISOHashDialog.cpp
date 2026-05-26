#include "ISOHashDialog.h"
#include "ISOHashWorkerThread.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>

ISOHashDialog::ISOHashDialog(const QString &isoPath, winafi_session_t *session, QWidget *parent)
    : QDialog(parent), iso_path_(isoPath), session_(session), worker_thread_(nullptr) {
    setWindowTitle("ISO Hash Verification");
    setModal(true);
    setMinimumWidth(500);

    setupUI();
}

ISOHashDialog::~ISOHashDialog() {
    if (worker_thread_) {
        worker_thread_->requestInterruption();
        worker_thread_->wait();
        delete worker_thread_;
    }
}

void ISOHashDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ISO File Display
    QGroupBox *fileGroup = new QGroupBox("ISO File", this);
    QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);
    iso_file_label_ = new QLabel(iso_path_, this);
    iso_file_label_->setObjectName("isoFileLabel");
    iso_file_label_->setWordWrap(true);
    iso_file_label_->setStyleSheet("background-color: #f0f0f0; padding: 8px; border-radius: 3px;");
    fileLayout->addWidget(iso_file_label_);
    mainLayout->addWidget(fileGroup);

    // Hash Type Selection and Input
    QGroupBox *hashGroup = new QGroupBox("Hash Verification", this);
    QVBoxLayout *hashLayout = new QVBoxLayout(hashGroup);

    // Hash type row
    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Hash type:", this));
    hash_type_combo_ = new QComboBox(this);
    hash_type_combo_->setObjectName("hashTypeCombo");
    hash_type_combo_->addItem("MD5", static_cast<int>(HashType::MD5));
    hash_type_combo_->addItem("SHA256", static_cast<int>(HashType::SHA256));
    hash_type_combo_->addItem("SHA512", static_cast<int>(HashType::SHA512));
    hash_type_combo_->setCurrentIndex(1); // SHA256 default
    typeLayout->addWidget(hash_type_combo_);
    typeLayout->addStretch();
    hashLayout->addLayout(typeLayout);

    // Hash input row
    QLabel *hashInputLabel = new QLabel("Expected hash value:", this);
    hashLayout->addWidget(hashInputLabel);
    hash_input_ = new QLineEdit(this);
    hash_input_->setObjectName("hashInput");
    hash_input_->setPlaceholderText("Paste hash value here...");
    hash_input_->setMinimumHeight(32);
    hashLayout->addWidget(hash_input_);

    // Browse button row
    QHBoxLayout *browseLayout = new QHBoxLayout();
    browseLayout->addStretch();
    browse_btn_ = new QPushButton("Browse for .sha256/.md5 file", this);
    browse_btn_->setObjectName("browseBtn");
    browseLayout->addWidget(browse_btn_);
    hashLayout->addLayout(browseLayout);

    mainLayout->addWidget(hashGroup);

    // Progress and Verification
    QGroupBox *verifyGroup = new QGroupBox("Verification", this);
    QVBoxLayout *verifyLayout = new QVBoxLayout(verifyGroup);

    progress_bar_ = new QProgressBar(this);
    progress_bar_->setObjectName("progressBar");
    progress_bar_->setValue(0);
    progress_bar_->setMinimumHeight(24);
    progress_bar_->setVisible(false);
    verifyLayout->addWidget(progress_bar_);

    result_label_ = new QLabel(this);
    result_label_->setObjectName("resultLabel");
    result_label_->setText("Ready to verify");
    result_label_->setWordWrap(true);
    result_label_->setMinimumHeight(40);
    verifyLayout->addWidget(result_label_);

    mainLayout->addWidget(verifyGroup);

    // Dialog buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    verify_btn_ = new QPushButton("Verify Hash", this);
    verify_btn_->setObjectName("verifyBtn");
    verify_btn_->setMinimumHeight(32);
    verify_btn_->setMinimumWidth(100);
    buttonLayout->addStretch();
    buttonLayout->addWidget(verify_btn_);
    buttonLayout->addWidget(new QPushButton("Cancel", this));
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    // Connect signals
    connect(hash_type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ISOHashDialog::onHashTypeChanged);
    connect(browse_btn_, &QPushButton::clicked, this, &ISOHashDialog::onBrowseHashFile);
    connect(verify_btn_, &QPushButton::clicked, this, &ISOHashDialog::onVerify);

    // Find and connect dialog buttons
    QDialogButtonBox *buttonBox = nullptr;
    for (QObject *child : children()) {
        buttonBox = qobject_cast<QDialogButtonBox *>(child);
        if (buttonBox) break;
    }
    if (!buttonBox) {
        // Create one if not found
        buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    }
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ISOHashDialog::onHashTypeChanged(int /* index */) {
    // Clear hash input when hash type changes
    hash_input_->clear();
}

void ISOHashDialog::onBrowseHashFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select hash file", "",
        "Hash Files (*.sha256 *.md5 *.sha512);;All Files (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString line = in.readLine();
            file.close();

            if (!line.isEmpty()) {
                // Extract hash from line (format: "HASH filename" or just "HASH")
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (!parts.isEmpty()) {
                    hash_input_->setText(parts[0]);
                }
            }
        }
    }
}

void ISOHashDialog::onVerify() {
    if (hash_input_->text().isEmpty()) {
        result_label_->setText("Error: Please enter or load a hash value");
        result_label_->setStyleSheet("color: #d32f2f;");
        return;
    }

    enableControls(false);
    progress_bar_->setVisible(true);
    progress_bar_->setValue(0);
    result_label_->setText("Verifying...");
    result_label_->setStyleSheet("color: #1976d2;");

    // Get hash type
    int current_index = hash_type_combo_->currentIndex();
    HashType hash_type = static_cast<HashType>(hash_type_combo_->itemData(current_index).toInt());

    // Create and start worker thread
    if (worker_thread_) {
        worker_thread_->requestInterruption();
        worker_thread_->wait();
        delete worker_thread_;
    }

    worker_thread_ = new ISOHashWorkerThread(iso_path_, hash_input_->text(), hash_type);
    connect(worker_thread_, &ISOHashWorkerThread::progressUpdated, this, &ISOHashDialog::onProgressUpdated);
    connect(worker_thread_, &ISOHashWorkerThread::verifyComplete, this, &ISOHashDialog::onVerifyComplete);
    worker_thread_->start();
}

void ISOHashDialog::onProgressUpdated(int percent) {
    progress_bar_->setValue(percent);
}

void ISOHashDialog::onVerifyComplete(bool match, QString message) {
    if (match) {
        result_label_->setText("✓ " + message);
        result_label_->setStyleSheet("color: #388e3c; font-weight: bold;");
    } else {
        result_label_->setText("✗ " + message);
        result_label_->setStyleSheet("color: #d32f2f; font-weight: bold;");
    }

    progress_bar_->setVisible(false);
    enableControls(true);
}

void ISOHashDialog::enableControls(bool enable) {
    hash_type_combo_->setEnabled(enable);
    hash_input_->setEnabled(enable);
    browse_btn_->setEnabled(enable);
    verify_btn_->setEnabled(enable);
}
