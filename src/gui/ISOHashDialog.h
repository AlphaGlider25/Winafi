#ifndef ISO_HASH_DIALOG_H
#define ISO_HASH_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QThread>
#include "../platform/linux/session.h"

class ISOHashWorkerThread;

class ISOHashDialog : public QDialog {
    Q_OBJECT

public:
    explicit ISOHashDialog(const QString &isoPath, winafi_session_t *session, QWidget *parent = nullptr);
    ~ISOHashDialog();

private slots:
    void onHashTypeChanged(int index);
    void onBrowseHashFile();
    void onVerify();
    void onVerifyComplete(bool match, QString message);
    void onProgressUpdated(int percent);

private:
    void setupUI();
    void enableControls(bool enable);

    QString iso_path_;
    winafi_session_t *session_;

    // UI Components
    QLabel *iso_file_label_;
    QComboBox *hash_type_combo_;
    QLineEdit *hash_input_;
    QPushButton *browse_btn_;
    QPushButton *verify_btn_;
    QProgressBar *progress_bar_;
    QLabel *result_label_;

    // Worker thread
    ISOHashWorkerThread *worker_thread_;
};

#endif // ISO_HASH_DIALOG_H
