#include "MainWindow.h"
#include "WorkerThread.h"
#include "AdvancedDriveDialog.h"
#include "AdvancedFormatDialog.h"
#include "ISOHashDialog.h"
#include "winafi.h"
#include "platform/linux/settings.h"
#include "platform/linux/update.h"
#include "platform/linux/iso_extract.h"
#include <cstring>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QLocale>
#include <QLabel>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QColor>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_session(nullptr), m_worker(nullptr), m_currentISO(""), m_imageOption(0), m_darkMode(false) {
    // Create session for storing format options
    m_session = winafi_session_create();
    setWindowTitle("Winafi v4.0.0");
    setWindowIcon(QIcon(":/icons/winafi.png"));
    setMinimumWidth(720);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    setCentralWidget(centralWidget);
    setupUI();

    // Size window to fit all controls at natural height (no vertical squeeze).
    const int contentMinH = centralWidget->minimumSizeHint().height();
    const int minH = contentMinH + menuBar()->sizeHint().height();
    setMinimumHeight(minH);
    resize(720, minH);

    // Load dark mode setting
    settings_t *s2 = settings_open();
    m_darkMode = settings_get_bool(s2, "dark_mode", 0) != 0;
    settings_close(s2);
    if (m_darkMode) applyDarkPalette();

    // Restore persisted settings
    settings_t *cfg = settings_open();
    if (cfg) {
        char saved_locale[64] = "en-US";
        settings_get_string(cfg, "locale", saved_locale, sizeof(saved_locale), "en-US");
        setupLanguagePicker(QString::fromUtf8(saved_locale));
        settings_close(cfg);
        cfg = settings_open(); // reopen for remaining settings below
    } else {
        setupLanguagePicker(QString("en-US"));
    }
    if (cfg) {
        char iso[1024] = "";
        settings_get_string(cfg, "last_iso_path", iso, sizeof(iso), "");
        if (iso[0] != '\0') {
            m_isoPathEdit->setText(QString::fromUtf8(iso));
            m_currentISO = QString::fromUtf8(iso);
        }
        int scheme = settings_get_int(cfg, "partition_scheme", 0);
        if (scheme >= 0 && scheme < m_partitionSchemeCombo->count())
            m_partitionSchemeCombo->setCurrentIndex(scheme);
        int fs = settings_get_int(cfg, "filesystem", 0);
        if (fs >= 0 && fs < m_fileSystemCombo->count())
            m_fileSystemCombo->setCurrentIndex(fs);
        settings_close(cfg);
    }

    refreshDeviceList();
    if (!m_currentISO.isEmpty()) {
        detectISOOptions(m_currentISO);
    }

    connect(this, &MainWindow::destroyed, [this]() {
        if (m_worker && m_worker->isRunning()) {
            m_worker->requestInterruption();
            m_worker->wait();
        }
    });
}

MainWindow::~MainWindow() {
    // Persist settings before teardown
    settings_t *cfg = settings_open();
    if (cfg) {
        settings_set_string(cfg, "last_iso_path", m_currentISO.toUtf8().constData());
        settings_set_int(cfg, "partition_scheme", m_partitionSchemeCombo->currentIndex());
        settings_set_int(cfg, "filesystem", m_fileSystemCombo->currentIndex());
        settings_set_bool(cfg, "dark_mode", m_darkMode ? 1 : 0);
        if (settings_close(cfg) != 0) {
            // Settings could not be saved — log but don't crash
            appendLog("Warning: could not save settings to disk.");
        }
    }
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestInterruption();
        m_worker->wait();
    }
    if (m_session) {
        winafi_session_destroy(m_session);
        m_session = nullptr;
    }
}

void MainWindow::setupUI() {
    // Create menu bar with View menu for dark mode toggle
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *checkUpdateAction = helpMenu->addAction("Check for Updates...");
    connect(checkUpdateAction, &QAction::triggered, this, &MainWindow::onCheckForUpdates);

    QMenu *viewMenu = menuBar->addMenu("&View");
    QAction *darkModeAction = viewMenu->addAction("Dark Mode");
    darkModeAction->setCheckable(true);
    darkModeAction->setChecked(m_darkMode);
    connect(darkModeAction, &QAction::triggered, this, &MainWindow::onDarkModeToggled);

    createDrivePropertiesSection();
    createFormatOptionsSection();
    createAdvancedSection();
    createStatusSection();
    createActionButtons();
}

void MainWindow::createDrivePropertiesSection() {
    QGroupBox *group = new QGroupBox("Drive Properties", this);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout *vboxLayout = new QVBoxLayout(group);
    vboxLayout->setSpacing(8);
    vboxLayout->setContentsMargins(10, 16, 10, 10);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    formLayout->setHorizontalSpacing(8);
    formLayout->setVerticalSpacing(12);

    m_deviceCombo = new QComboBox();
    m_deviceCombo->setMinimumHeight(32);
    m_refreshButton = new QPushButton("🔄");
    m_refreshButton->setMaximumWidth(50);
    m_refreshButton->setMinimumHeight(32);
    QHBoxLayout *deviceLayout = new QHBoxLayout();
    deviceLayout->setSpacing(4);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    deviceLayout->addWidget(m_deviceCombo, 1);
    deviceLayout->addWidget(m_refreshButton);
    formLayout->addRow("Device:", deviceLayout);

    m_isoPathEdit = new QLineEdit();
    m_isoPathEdit->setReadOnly(true);
    m_isoPathEdit->setMinimumHeight(32);
    m_verifyButton = new QPushButton("✓");
    m_verifyButton->setMaximumWidth(50);
    m_verifyButton->setMinimumHeight(32);
    m_browseButton = new QPushButton("SELECT");
    m_browseButton->setMinimumHeight(32);
    m_browseButton->setMinimumWidth(80);
    QHBoxLayout *bootLayout = new QHBoxLayout();
    bootLayout->setSpacing(4);
    bootLayout->setContentsMargins(0, 0, 0, 0);
    bootLayout->addWidget(m_isoPathEdit, 1);
    bootLayout->addWidget(m_verifyButton);
    bootLayout->addWidget(m_browseButton);
    formLayout->addRow("Boot selection:", bootLayout);

    m_imageOptionCombo = new QComboBox();
    m_imageOptionCombo->setMinimumHeight(32);
    formLayout->addRow("Image option:", m_imageOptionCombo);

    m_partitionSchemeCombo = new QComboBox();
    m_partitionSchemeCombo->setMinimumHeight(32);
    m_partitionSchemeCombo->addItems({"MBR", "GPT"});
    m_partitionSchemeCombo->setCurrentText("GPT");

    QLabel *targetLabel = new QLabel("Target system:");
    targetLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_targetSystemCombo = new QComboBox();
    m_targetSystemCombo->setMinimumHeight(32);
    m_targetSystemCombo->addItems({"BIOS", "UEFI"});
    m_targetSystemCombo->setCurrentText("UEFI");

    QHBoxLayout *partRow = new QHBoxLayout();
    partRow->setSpacing(16);
    partRow->setContentsMargins(0, 0, 0, 0);
    partRow->addWidget(m_partitionSchemeCombo, 1);
    partRow->addWidget(targetLabel);
    partRow->addWidget(m_targetSystemCombo, 1);
    formLayout->addRow("Partition scheme:", partRow);

    vboxLayout->addLayout(formLayout);

    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->centralWidget()->layout());
    if (mainLayout) {
        mainLayout->addWidget(group, 0);
    }

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshDevices);
    connect(m_browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseISO);
    connect(m_verifyButton, &QPushButton::clicked, this, &MainWindow::onVerifyISO);

    // Connect partition scheme and target system
    connect(m_partitionSchemeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPartitionSchemeChanged);
    connect(m_targetSystemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTargetSystemChanged);
    connect(m_imageOptionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onImageOptionChanged);
}

void MainWindow::createFormatOptionsSection() {
    QGroupBox *group = new QGroupBox("Format Options", this);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout *vboxLayout = new QVBoxLayout(group);
    vboxLayout->setSpacing(8);
    vboxLayout->setContentsMargins(10, 16, 10, 10);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    formLayout->setHorizontalSpacing(8);
    formLayout->setVerticalSpacing(12);

    m_volumeLabelEdit = new QLineEdit();
    m_volumeLabelEdit->setMinimumHeight(32);
    formLayout->addRow("Volume label:", m_volumeLabelEdit);

    m_fileSystemCombo = new QComboBox();
    m_fileSystemCombo->setMinimumHeight(32);
    m_fileSystemCombo->addItems({"FAT32", "NTFS", "exFAT"});
    m_fileSystemCombo->setCurrentText("NTFS");

    QLabel *csLabel = new QLabel("Cluster size:");
    csLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_clusterSizeCombo = new QComboBox();
    m_clusterSizeCombo->setMinimumHeight(32);
    m_clusterSizeCombo->addItems({"512 bytes", "1024 bytes", "2048 bytes", "4096 bytes (Default)"});
    m_clusterSizeCombo->setCurrentText("4096 bytes (Default)");

    QHBoxLayout *fsRow = new QHBoxLayout();
    fsRow->setSpacing(16);
    fsRow->setContentsMargins(0, 0, 0, 0);
    fsRow->addWidget(m_fileSystemCombo, 1);
    fsRow->addWidget(csLabel);
    fsRow->addWidget(m_clusterSizeCombo, 1);
    formLayout->addRow("File system:", fsRow);

    vboxLayout->addLayout(formLayout);

    // Checkboxes below grid
    m_quickFormatCheck = new QCheckBox("Quick format");
    m_quickFormatCheck->setChecked(true);
    m_quickFormatCheck->setMinimumHeight(24);
    vboxLayout->addWidget(m_quickFormatCheck);

    m_extendedLabelCheck = new QCheckBox("Create extended label and icon files");
    m_extendedLabelCheck->setChecked(true);
    m_extendedLabelCheck->setMinimumHeight(24);
    vboxLayout->addWidget(m_extendedLabelCheck);

    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->centralWidget()->layout());
    if (mainLayout) {
        mainLayout->addWidget(group, 0);
    }

    // Connect format options
    connect(m_fileSystemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFileSystemChanged);
    connect(m_clusterSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onClusterSizeChanged);
    connect(m_volumeLabelEdit, &QLineEdit::textChanged,
            this, &MainWindow::onVolumeLabelChanged);
    connect(m_quickFormatCheck, QOverload<int>::of(&QCheckBox::stateChanged),
            this, &MainWindow::onQuickFormatChanged);
}

void MainWindow::createAdvancedSection() {
    QGroupBox *group = new QGroupBox("Advanced Options", this);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setContentsMargins(10, 16, 10, 8);
    layout->setSpacing(6);

    m_advancedDriveButton = new QPushButton("▶ Advanced drive properties");
    m_advancedDriveButton->setFlat(true);
    layout->addWidget(m_advancedDriveButton);

    m_advancedFormatButton = new QPushButton("▶ Advanced format options");
    m_advancedFormatButton->setFlat(true);
    layout->addWidget(m_advancedFormatButton);

    m_verifyHashButton = new QPushButton("▶ Verify ISO hash");
    m_verifyHashButton->setFlat(true);
    layout->addWidget(m_verifyHashButton);

    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->centralWidget()->layout());
    if (mainLayout) {
        mainLayout->addWidget(group, 0);
    }

    // Connect advanced buttons
    connect(m_advancedDriveButton, &QPushButton::clicked, this, &MainWindow::onShowAdvancedDrive);
    connect(m_advancedFormatButton, &QPushButton::clicked, this, &MainWindow::onShowAdvancedFormat);
    connect(m_verifyHashButton, &QPushButton::clicked, this, &MainWindow::onShowHashDialog);
}

void MainWindow::createStatusSection() {
    QGroupBox *group = new QGroupBox("Status", this);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setContentsMargins(10, 16, 10, 10);
    layout->setSpacing(8);

    m_statusLabel = new QLabel("READY");
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_statusLabel->setFixedHeight(28);
    m_statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(m_progressBar);

    m_logArea = new QPlainTextEdit();
    m_logArea->setReadOnly(true);
    m_logArea->setMinimumHeight(72);
    m_logArea->setFont(QFont("Courier", 10));
    m_logArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_logArea, 1);

    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->centralWidget()->layout());
    if (mainLayout) {
        mainLayout->addWidget(group, 1);
    }

    setProgressActive(false);
}

void MainWindow::createActionButtons() {
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setEnabled(false);
    m_cancelButton->setMinimumHeight(34);
    m_cancelButton->setMinimumWidth(110);

    m_startButton = new QPushButton("Start", this);
    m_startButton->setMinimumHeight(34);
    m_startButton->setMinimumWidth(110);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 4, 0, 0);
    buttonLayout->setSpacing(8);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_startButton);

    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->centralWidget()->layout());
    if (mainLayout) {
        mainLayout->addLayout(buttonLayout, 0);
    }

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::onCancel);
}

void MainWindow::setProgressActive(bool active) {
    m_progressBar->setVisible(active);
    if (!active) {
        m_progressBar->setValue(0);
        m_statusLabel->setText("READY");
    }
}

void MainWindow::onRefreshDevices() {
    refreshDeviceList();
}

void MainWindow::onBrowseISO() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select ISO", "", "ISO Files (*.iso)");
    if (!filePath.isEmpty()) {
        m_isoPathEdit->setText(filePath);
        m_currentISO = filePath;
        detectISOOptions(filePath);
    }
}

void MainWindow::onVerifyISO() {
    if (m_currentISO.isEmpty()) {
        showErrorDialog("No ISO", "Please select an ISO file first");
        return;
    }

    winafi_session_t *session = winafi_session_create();
    if (!session) {
        showErrorDialog("Error", "Failed to create session for validation");
        return;
    }

    if (winafi_session_load_iso(session, m_currentISO.toUtf8().constData()) != WINAFI_OK) {
        const char *error = winafi_get_error_message(session);
        showErrorDialog("Invalid ISO", QString::fromUtf8(error));
        winafi_session_destroy(session);
        return;
    }

    const char *detected_os = winafi_get_detected_os(session);
    QString osName = detected_os ? QString::fromUtf8(detected_os) : "Unknown OS";

    winafi_session_destroy(session);

    detectISOOptions(m_currentISO);

    QMessageBox::information(this, "ISO Verified",
        QString("OS: %1\nISO is valid").arg(osName));
}

void MainWindow::onStart() {
    if (m_currentISO.isEmpty()) {
        showErrorDialog("Missing ISO", "Please select an ISO file");
        return;
    }
    if (m_deviceCombo->currentIndex() < 0) {
        showErrorDialog("No Device", "Please select a device");
        return;
    }

    QString devnode = m_deviceCombo->currentData().toString();
    QMessageBox::StandardButton reply = QMessageBox::warning(this, "Confirm",
        "This will erase all data on " + devnode + ". Continue?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    // Extract cluster size value
    QString clusterText = m_clusterSizeCombo->currentText();
    uint32_t clusterSize = 4096; // Default
    if (clusterText.contains("512")) clusterSize = 512;
    else if (clusterText.contains("1024")) clusterSize = 1024;
    else if (clusterText.contains("2048")) clusterSize = 2048;

    if (m_imageOptionCombo->currentIndex() < 0 ||
        !m_imageOptionCombo->itemData(m_imageOptionCombo->currentIndex(), Qt::UserRole).isValid()) {
        showErrorDialog("Image option", "Select a valid image option for this ISO");
        return;
    }

    enableControls(false);
    m_logArea->clear();
    setProgressActive(true);
    m_statusLabel->setText("Starting...");
    m_cancelButton->setEnabled(true);

    m_worker = std::make_unique<WorkerThread>(
        m_currentISO, devnode,
        m_volumeLabelEdit->text(),
        m_fileSystemCombo->currentText(),
        clusterSize,
        m_quickFormatCheck->isChecked(),
        m_imageOption
    );
    connect(m_worker.get(), &WorkerThread::progressUpdated, this, &MainWindow::onProgressUpdated);
    connect(m_worker.get(), &WorkerThread::finished, this, &MainWindow::onWriteFinished);
    m_worker->start();
}

void MainWindow::onCancel() {
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestInterruption();
    }
}

void MainWindow::onProgressUpdated(int percent, QString message) {
    m_progressBar->setValue(percent);
    m_statusLabel->setText(message);
    appendLog("[" + QString::number(percent) + "%] " + message);
}

void MainWindow::onWriteFinished(bool success, QString errorCode, QString errorMsg) {
    m_cancelButton->setEnabled(false);
    setProgressActive(false);

    if (success) {
        m_statusLabel->setText("Completed successfully");
        QMessageBox::information(this, "Success", "Bootable USB created successfully");
    } else {
        // Map error codes to user-friendly messages
        QString displayMsg = errorMsg;
        if (errorCode == "ISO_LOAD_FAILED") {
            displayMsg = "Failed to load ISO file. Ensure the file is a valid Windows/Linux ISO.";
        } else if (errorCode == "DEVICE_SELECT_FAILED") {
            displayMsg = "Device not found or inaccessible. Ensure it's connected and not in use.";
        } else if (errorCode == "EXECUTE_FAILED") {
            displayMsg = "Write operation failed. Check device permissions and free space.";
        }

        m_statusLabel->setText("ERROR: " + errorCode);
        showErrorDialog("Error: " + errorCode, displayMsg);
    }

    enableControls(true);
}

void MainWindow::refreshDeviceList() {
    m_deviceCombo->clear();

    winafi_session_t *session = winafi_session_create();
    if (!session) {
        m_deviceCombo->addItem("Failed to enumerate devices");
        return;
    }

    winafi_device_t *devices = nullptr;
    int count = 0;

    if (winafi_enumerate_devices(session, &devices, &count) != WINAFI_OK) {
        m_deviceCombo->addItem("No devices found");
        winafi_session_destroy(session);
        return;
    }

    if (count == 0) {
        m_deviceCombo->addItem("No devices found");
    } else {
        for (int i = 0; i < count; i++) {
            QString displayText = QString("%1: %2 %3 [%4 GB]")
                .arg(QString::fromUtf8(devices[i].devnode))
                .arg(QString::fromUtf8(devices[i].vendor))
                .arg(QString::fromUtf8(devices[i].model))
                .arg(devices[i].capacity_bytes / (1024 * 1024 * 1024));

            m_deviceCombo->addItem(displayText, QString::fromUtf8(devices[i].devnode));
        }
    }

    winafi_session_destroy(session);
}

void MainWindow::enableControls(bool enable) {
    m_deviceCombo->setEnabled(enable);
    m_refreshButton->setEnabled(enable);
    m_browseButton->setEnabled(enable);
    m_verifyButton->setEnabled(enable);
    m_imageOptionCombo->setEnabled(enable);
    m_partitionSchemeCombo->setEnabled(enable);
    m_targetSystemCombo->setEnabled(enable);
    m_volumeLabelEdit->setEnabled(enable);
    m_fileSystemCombo->setEnabled(enable);
    m_clusterSizeCombo->setEnabled(enable);
    m_quickFormatCheck->setEnabled(enable);
    m_extendedLabelCheck->setEnabled(enable);
    m_startButton->setEnabled(enable);

    if (enable && m_imageOptionCombo->currentIndex() >= 0) {
        onImageOptionChanged(m_imageOptionCombo->currentIndex());
    }
}

void MainWindow::appendLog(const QString &text) {
    m_logArea->appendPlainText(text);
}

void MainWindow::showErrorDialog(const QString &title, const QString &message) {
    QMessageBox::critical(this, title, message);
}

void MainWindow::onPartitionSchemeChanged(int index) {
    if (!m_session) return;

    winafi_partition_scheme_t scheme = (index == 0) ? WINAFI_PARTITION_MBR : WINAFI_PARTITION_GPT;
    winafi_session_set_partition_scheme(m_session, scheme);
}

void MainWindow::onTargetSystemChanged(int index) {
    if (!m_session) return;

    winafi_target_system_t target = (index == 0) ? WINAFI_TARGET_BIOS : WINAFI_TARGET_UEFI;
    winafi_session_set_target_system(m_session, target);
}

void MainWindow::onFileSystemChanged(int index) {
    if (!m_session) return;

    winafi_filesystem_t fs;
    switch (index) {
        case 0: fs = WINAFI_FS_FAT32; break;
        case 1: fs = WINAFI_FS_NTFS; break;
        case 2: fs = WINAFI_FS_EXFAT; break;
        default: fs = WINAFI_FS_NTFS; break;
    }
    winafi_session_set_filesystem(m_session, fs);
}

void MainWindow::onClusterSizeChanged(int index) {
    if (!m_session) return;

    uint32_t cluster_size;
    switch (index) {
        case 0: cluster_size = 512; break;
        case 1: cluster_size = 1024; break;
        case 2: cluster_size = 2048; break;
        case 3: cluster_size = 4096; break;
        default: cluster_size = 4096; break;
    }
    winafi_session_set_cluster_size(m_session, cluster_size);
}

void MainWindow::onVolumeLabelChanged(const QString &text) {
    if (!m_session) return;

    winafi_session_set_volume_label(m_session, text.toUtf8().constData());
}

void MainWindow::onQuickFormatChanged(int state) {
    if (!m_session) return;

    int quick = (state == Qt::Checked) ? 1 : 0;
    winafi_session_set_quick_format(m_session, quick);
}

void MainWindow::addImageOptionItem(const QString &text, int optionValue) {
    const int idx = m_imageOptionCombo->count();
    m_imageOptionCombo->addItem(text);
    m_imageOptionCombo->setItemData(idx, optionValue, Qt::UserRole);
}

void MainWindow::syncSessionFromUI() {
    if (!m_session) {
        return;
    }
    onPartitionSchemeChanged(m_partitionSchemeCombo->currentIndex());
    onTargetSystemChanged(m_targetSystemCombo->currentIndex());
    onFileSystemChanged(m_fileSystemCombo->currentIndex());
    onClusterSizeChanged(m_clusterSizeCombo->currentIndex());
    onVolumeLabelChanged(m_volumeLabelEdit->text());
    onQuickFormatChanged(m_quickFormatCheck->checkState());
    if (m_imageOptionCombo->currentIndex() >= 0) {
        onImageOptionChanged(m_imageOptionCombo->currentIndex());
    }
}

void MainWindow::loadIsoIntoSession(const QString &isoPath) {
    if (isoPath.isEmpty()) {
        return;
    }

    if (m_session) {
        winafi_session_destroy(m_session);
    }
    m_session = winafi_session_create();
    if (!m_session) {
        return;
    }

    if (winafi_session_load_iso(m_session, isoPath.toUtf8().constData()) == WINAFI_OK) {
        syncSessionFromUI();
    }
}

void MainWindow::detectISOOptions(const QString &isoPath) {
    QSignalBlocker blocker(m_imageOptionCombo);
    m_imageOptionCombo->clear();

    if (isoPath.isEmpty()) {
        addImageOptionItem("Select an ISO file", -1);
        return;
    }

    // Check if file exists first
    QFileInfo fileInfo(isoPath);
    if (!fileInfo.exists()) {
        addImageOptionItem("ISO file not found", -1);
        return;
    }

    if (!fileInfo.isFile() || !fileInfo.isReadable()) {
        addImageOptionItem("Cannot read ISO file", -1);
        return;
    }

    iso_info_t info;
    memset(&info, 0, sizeof(info));
    int detectResult = iso_detect_os(isoPath.toUtf8().constData(), &info);
    bool recognized = (detectResult == ISO_OK) ||
        (detectResult == ISO_ERR_NO_BOOT_INFO && info.os_type != ISO_OS_UNKNOWN);

    if (!recognized) {
        winafi_session_t *probe = winafi_session_create();
        if (probe && winafi_session_load_iso(probe, isoPath.toUtf8().constData()) == WINAFI_OK) {
            const char *os = winafi_get_detected_os(probe);
            if (os) {
                if (strstr(os, "Windows") != nullptr) {
                    info.os_type = ISO_OS_WINDOWS;
                } else if (strstr(os, "Linux") != nullptr) {
                    info.os_type = ISO_OS_LINUX;
                }
                recognized = (info.os_type != ISO_OS_UNKNOWN);
            }
        }
        if (probe) {
            winafi_session_destroy(probe);
        }
    }

    if (!recognized) {
        addImageOptionItem("Unrecognized ISO format", -1);
        return;
    }

    if (info.os_type == ISO_OS_WINDOWS) {
        addImageOptionItem("Standard Windows installation", WINAFI_IMAGE_STANDARD);
        addImageOptionItem("Windows To Go", WINAFI_IMAGE_WINTOGO);
    } else if (info.os_type == ISO_OS_LINUX) {
        addImageOptionItem("Standard Linux installation", WINAFI_IMAGE_STANDARD);
    } else {
        addImageOptionItem("Standard installation", WINAFI_IMAGE_STANDARD);
    }

    m_imageOptionCombo->setCurrentIndex(0);
    loadIsoIntoSession(isoPath);
    onImageOptionChanged(0);
}

void MainWindow::onImageOptionChanged(int index) {
    if (!m_session || index < 0) return;

    const QVariant optionData = m_imageOptionCombo->itemData(index, Qt::UserRole);
    if (!optionData.isValid() || !optionData.canConvert<int>()) {
        return;
    }

    const int optionValue = optionData.toInt();
    if (optionValue < 0) {
        return;
    }

    const winafi_image_option_t option =
        static_cast<winafi_image_option_t>(optionValue);
    const bool isWinToGo = (option == WINAFI_IMAGE_WINTOGO);

    m_imageOption = static_cast<int>(option);
    winafi_session_set_image_option(m_session, option);

    // Windows To Go special handling
    if (isWinToGo) {
        // Lock format options for Windows To Go
        m_fileSystemCombo->setCurrentIndex(1);  // NTFS
        m_fileSystemCombo->setEnabled(false);

        m_partitionSchemeCombo->setCurrentIndex(1);  // GPT
        m_partitionSchemeCombo->setEnabled(false);

        m_targetSystemCombo->setCurrentIndex(1);  // UEFI
        m_targetSystemCombo->setEnabled(false);

        appendLog("Windows To Go selected: NTFS, GPT, UEFI locked");
    } else {
        // Re-enable options
        m_fileSystemCombo->setEnabled(true);
        m_partitionSchemeCombo->setEnabled(true);
        m_targetSystemCombo->setEnabled(true);
    }
}

void MainWindow::onShowAdvancedDrive() {
    if (!m_session) return;

    AdvancedDriveDialog dialog(m_session, this);
    dialog.exec();
}

void MainWindow::onShowAdvancedFormat() {
    if (!m_session) return;

    AdvancedFormatDialog dialog(m_session, this);
    dialog.exec();
}

void MainWindow::setupLanguagePicker(const QString &savedLocale) {
    m_translator = new QTranslator(this);

    m_langCombo = new QComboBox(this);
    m_langCombo->setMinimumHeight(28);
    m_langCombo->setToolTip("Interface language");

    struct { const char *display; const char *tag; } langs[] = {
        {"English (US)",   "en-US"},
        {"Français",       "fr-FR"},
        {"Deutsch",        "de-DE"},
        {"Español",        "es-ES"},
        {"Italiano",       "it-IT"},
        {"日本語",          "ja-JP"},
        {"中文 (简体)",     "zh-CN"},
        {nullptr, nullptr}
    };

    for (int i = 0; langs[i].display; i++) {
        m_langCombo->addItem(QString::fromUtf8(langs[i].display),
                             QString::fromUtf8(langs[i].tag));
    }

    int idx = m_langCombo->findData(savedLocale);
    if (idx >= 0)
        m_langCombo->setCurrentIndex(idx);

    // Apply the saved locale immediately (without triggering persist)
    if (m_translator->load("winafi_" + savedLocale, ":/translations/")) {
        qApp->installTranslator(m_translator);
    }
    QLocale::setDefault(QLocale(savedLocale));

    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageChanged);

    // Add a toolbar with the language picker
    QToolBar *langToolBar = addToolBar("Language");
    langToolBar->setMovable(false);
    langToolBar->setFloatable(false);
    // Push combo to the right side via a spacer
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    langToolBar->addWidget(spacer);
    langToolBar->addWidget(new QLabel("  Language:  "));
    langToolBar->addWidget(m_langCombo);
}

void MainWindow::onLanguageChanged(int index) {
    QString tag = m_langCombo->itemData(index).toString();

    qApp->removeTranslator(m_translator);
    if (m_translator->load("winafi_" + tag, ":/translations/")) {
        qApp->installTranslator(m_translator);
    }
    QLocale::setDefault(QLocale(tag));

    settings_t *s = settings_open();
    if (s) {
        settings_set_string(s, "locale", tag.toUtf8().constData());
        settings_close(s);
    }
}

void MainWindow::onShowHashDialog() {
    if (m_currentISO.isEmpty()) {
        showErrorDialog("No ISO", "Please select an ISO file first");
        return;
    }

    if (!m_session) return;

    ISOHashDialog dialog(m_currentISO, m_session, this);
    dialog.exec();
}

void MainWindow::applyDarkPalette() {
    QPalette dark;
    dark.setColor(QPalette::Window,          QColor(0x2b, 0x2b, 0x2b));
    dark.setColor(QPalette::WindowText,      QColor(0xff, 0xff, 0xff));
    dark.setColor(QPalette::Base,            QColor(0x1e, 0x1e, 0x1e));
    dark.setColor(QPalette::AlternateBase,   QColor(0x35, 0x35, 0x35));
    dark.setColor(QPalette::ToolTipBase,     QColor(0x2b, 0x2b, 0x2b));
    dark.setColor(QPalette::ToolTipText,     QColor(0xff, 0xff, 0xff));
    dark.setColor(QPalette::Text,            QColor(0xff, 0xff, 0xff));
    dark.setColor(QPalette::Button,          QColor(0x3c, 0x3c, 0x3c));
    dark.setColor(QPalette::ButtonText,      QColor(0xff, 0xff, 0xff));
    dark.setColor(QPalette::BrightText,      QColor(0xff, 0x00, 0x00));
    dark.setColor(QPalette::Link,            QColor(0x42, 0xa5, 0xf5));
    dark.setColor(QPalette::Highlight,       QColor(0x42, 0x85, 0xf4));
    dark.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    dark.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x80, 0x80, 0x80));
    dark.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x80, 0x80, 0x80));
    qApp->setPalette(dark);
    m_darkMode = true;
}

void MainWindow::applyLightPalette() {
    // Create light palette with Fusion style defaults
    QPalette light;
    light.setColor(QPalette::Window,          QColor(0xf5, 0xf5, 0xf5));
    light.setColor(QPalette::WindowText,      QColor(0x00, 0x00, 0x00));
    light.setColor(QPalette::Base,            QColor(0xff, 0xff, 0xff));
    light.setColor(QPalette::AlternateBase,   QColor(0xf0, 0xf0, 0xf0));
    light.setColor(QPalette::ToolTipBase,     QColor(0xff, 0xff, 0xdc));
    light.setColor(QPalette::ToolTipText,     QColor(0x00, 0x00, 0x00));
    light.setColor(QPalette::Text,            QColor(0x00, 0x00, 0x00));
    light.setColor(QPalette::Button,          QColor(0xf0, 0xf0, 0xf0));
    light.setColor(QPalette::ButtonText,      QColor(0x00, 0x00, 0x00));
    light.setColor(QPalette::Link,            QColor(0x00, 0x78, 0xd4));
    light.setColor(QPalette::Highlight,       QColor(0x00, 0x78, 0xd4));
    light.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    light.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x80, 0x80, 0x80));
    light.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x80, 0x80, 0x80));
    qApp->setPalette(light);
    m_darkMode = false;
}

void MainWindow::onCheckForUpdates() {
    char latest[64] = "";
    int r = update_check(latest, sizeof(latest));
    if (r < 0) {
        QMessageBox::warning(this, "Update Check Failed",
            "Could not connect to GitHub to check for updates.");
    } else if (r == 0) {
        QMessageBox::information(this, "Up to Date",
            QString("Winafi %1 is the latest version.").arg(WINAFI_VERSION));
    } else {
        char url[256] = "";
        update_build_url(latest, url, sizeof(url));
        QMessageBox::information(this, "Update Available",
            QString("A new version (%1) is available.\n\nDownload at:\n%2")
                .arg(latest).arg(url));
    }
}

void MainWindow::onDarkModeToggled(bool enabled) {
    if (enabled) applyDarkPalette(); else applyLightPalette();
    settings_t *s = settings_open();
    if (s) {
        settings_set_bool(s, "dark_mode", enabled ? 1 : 0);
        settings_close(s);
    }
}
