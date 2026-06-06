#ifndef WINAFI_WINDOWS_CUSTOMIZE_SECTION_H
#define WINAFI_WINDOWS_CUSTOMIZE_SECTION_H
#include <QWidget>
#include <QString>
class QCheckBox; class QLineEdit;

// Maps the Windows-11 bypass / local-account / offline controls to WUE_* flags.
class WindowsCustomizeSection : public QWidget {
    Q_OBJECT
public:
    explicit WindowsCustomizeSection(QWidget *parent = nullptr);
    int wueFlags() const;        // bitwise-OR of WUE_* (see wue.h); 0 if nothing enabled
    QString username() const;    // local account name (empty if "create account" unchecked)
    void setWindowsIso(bool isWindows);  // grey out the whole section for non-Windows ISOs
signals:
    void changed();
private slots:
    void onApplyAllToggled(int state);
private:
    QCheckBox *m_tpm, *m_secureboot, *m_ram, *m_cpu, *m_storage, *m_applyAll;
    QCheckBox *m_createAccount, *m_admin, *m_skipMsAccount, *m_offline;
    QLineEdit *m_username;
};
#endif
