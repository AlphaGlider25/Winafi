#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "winafi.h"
#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTranslator>
#include <QPalette>
#include <QTimer>
#include <memory>

class WorkerThread;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRefreshDevices();
    void onAutoRefreshDevices();
    void onBrowseISO();
    void onVerifyISO();
    void onStart();
    void onCancel();
    void onProgressUpdated(int percent, QString message);
    void onWriteFinished(bool success, QString errorCode, QString errorMsg);
    void onPartitionSchemeChanged(int index);
    void onTargetSystemChanged(int index);
    void onFileSystemChanged(int index);
    void onClusterSizeChanged(int index);
    void onVolumeLabelChanged(const QString &text);
    void onQuickFormatChanged(int state);
    void onImageOptionChanged(int index);
    void onShowAdvancedDrive();
    void onShowAdvancedFormat();
    void onShowHashDialog();
    void onLanguageChanged(int index);
    void onDarkModeToggled(bool enabled);
    void onCheckForUpdates();
    void onHideHardDrivesToggled(int state);

private:
    void setupUI();
    void setupLanguagePicker(const QString &savedLocale);
    void createDrivePropertiesSection();
    void createFormatOptionsSection();
    void createAdvancedSection();
    void createStatusSection();
    void createActionButtons();
    void refreshDeviceList();
    bool isBootableDevice(const winafi_device_t *device);
    void detectISOOptions(const QString &isoPath);
    void loadIsoIntoSession(const QString &isoPath);
    void addImageOptionItem(const QString &text, int optionValue);
    void setProgressActive(bool active);
    void syncSessionFromUI();
    void enableControls(bool enable);
    void appendLog(const QString &text);
    void showErrorDialog(const QString &title, const QString &message);
    void applyDarkPalette();
    void applyLightPalette();

    // UI Components - Drive Properties
    QComboBox *m_deviceCombo;
    QPushButton *m_refreshButton;
    QLineEdit *m_isoPathEdit;
    QPushButton *m_verifyButton;
    QPushButton *m_browseButton;
    QComboBox *m_imageOptionCombo;
    QComboBox *m_partitionSchemeCombo;
    QComboBox *m_targetSystemCombo;

    // UI Components - Format Options
    QLineEdit *m_volumeLabelEdit;
    QComboBox *m_fileSystemCombo;
    QComboBox *m_clusterSizeCombo;
    QCheckBox *m_quickFormatCheck;
    QCheckBox *m_extendedLabelCheck;

    // UI Components - Advanced Options
    QPushButton *m_advancedDriveButton;
    QPushButton *m_advancedFormatButton;
    QPushButton *m_verifyHashButton;
    QCheckBox *m_hideHardDrivesCheck;

    // UI Components - Status & Progress
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QPlainTextEdit *m_logArea;
    QPushButton *m_startButton;
    QPushButton *m_cancelButton;

    // Language picker
    QComboBox *m_langCombo;
    QTranslator *m_translator;

    // Session
    struct winafi_session *m_session;

    // Worker
    std::unique_ptr<WorkerThread> m_worker;
    QString m_currentISO;
    int m_imageOption;

    // Dark mode
    bool m_darkMode;

    // Auto-refresh
    QTimer *m_autoRefreshTimer;
    bool m_showHardDrives;  // true = show hard drives, false = hide (default)
};

#endif // MAINWINDOW_H
