#include "sections/WindowsCustomizeSection.h"
#include "platform/linux/wue.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <unistd.h>
#include <pwd.h>

static QString currentLinuxUser() {
    const char *u = getenv("USER");
    if (u && *u) return QString::fromUtf8(u);
    struct passwd *pw = getpwuid(getuid());
    return pw ? QString::fromUtf8(pw->pw_name) : QString();
}

WindowsCustomizeSection::WindowsCustomizeSection(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);

    auto *bypassLbl = new QLabel(tr("Windows 11 compatibility bypasses"));
    bypassLbl->setStyleSheet("font-weight:600;");
    root->addWidget(bypassLbl);

    m_tpm        = new QCheckBox(tr("Remove TPM requirement"));        m_tpm->setObjectName("bypassTpm");
    m_secureboot = new QCheckBox(tr("Remove Secure Boot requirement")); m_secureboot->setObjectName("bypassSb");
    m_ram        = new QCheckBox(tr("Remove minimum RAM requirement")); m_ram->setObjectName("bypassRam");
    m_cpu        = new QCheckBox(tr("Remove CPU requirement"));         m_cpu->setObjectName("bypassCpu");
    m_storage    = new QCheckBox(tr("Remove storage requirement"));     m_storage->setObjectName("bypassStorage");
    m_applyAll   = new QCheckBox(tr("Apply all compatibility bypasses")); m_applyAll->setObjectName("applyAll");

    m_tpm->setToolTip(tr("Sets LabConfig BypassTPMCheck so Setup ignores the TPM 2.0 requirement."));
    m_secureboot->setToolTip(tr("Sets LabConfig BypassSecureBootCheck."));
    m_ram->setToolTip(tr("Sets LabConfig BypassRAMCheck (4 GB minimum)."));
    m_cpu->setToolTip(tr("Sets LabConfig BypassCPUCheck (unsupported processors)."));
    m_storage->setToolTip(tr("Sets LabConfig BypassStorageCheck (64 GB minimum)."));

    for (QCheckBox *c : {m_tpm, m_secureboot, m_ram, m_cpu, m_storage, m_applyAll})
        root->addWidget(c);

    auto *acctLbl = new QLabel(tr("Account & offline"));
    acctLbl->setStyleSheet("font-weight:600;margin-top:8px;");
    root->addWidget(acctLbl);

    m_createAccount = new QCheckBox(tr("Create local account automatically")); m_createAccount->setObjectName("createAccount");
    m_admin         = new QCheckBox(tr("Administrator account"));               m_admin->setObjectName("admin");
    m_skipMsAccount = new QCheckBox(tr("Skip Microsoft account requirement"));  m_skipMsAccount->setObjectName("skipMs");
    m_offline       = new QCheckBox(tr("Allow offline installation"));          m_offline->setObjectName("offline");

    auto *userRow = new QFormLayout();
    m_username = new QLineEdit(currentLinuxUser()); m_username->setObjectName("username");
    m_username->setToolTip(tr("Windows local account name. Invalid characters are replaced with '_'."));
    userRow->addRow(tr("Username:"), m_username);

    root->addWidget(m_createAccount);
    root->addLayout(userRow);
    root->addWidget(m_admin);
    root->addWidget(m_skipMsAccount);
    root->addWidget(m_offline);
    root->addStretch(1);

    connect(m_applyAll, &QCheckBox::stateChanged, this, &WindowsCustomizeSection::onApplyAllToggled);
    for (QCheckBox *c : {m_tpm,m_secureboot,m_ram,m_cpu,m_storage,m_createAccount,
                         m_admin,m_skipMsAccount,m_offline})
        connect(c, &QCheckBox::stateChanged, this, [this]{ emit changed(); });
    connect(m_username, &QLineEdit::textChanged, this, [this]{ emit changed(); });
}

void WindowsCustomizeSection::onApplyAllToggled(int state) {
    bool on = (state == Qt::Checked);
    for (QCheckBox *c : {m_tpm, m_secureboot, m_ram, m_cpu, m_storage}) {
        c->blockSignals(true); c->setChecked(on); c->blockSignals(false);
    }
    emit changed();
}

int WindowsCustomizeSection::wueFlags() const {
    int f = 0;
    if (m_tpm->isChecked())        f |= WUE_BYPASS_TPM;
    if (m_secureboot->isChecked()) f |= WUE_BYPASS_SECUREBOOT;
    if (m_ram->isChecked())        f |= WUE_BYPASS_RAM;
    if (m_cpu->isChecked())        f |= WUE_BYPASS_CPU;
    if (m_storage->isChecked())    f |= WUE_BYPASS_STORAGE;
    if (m_skipMsAccount->isChecked()) f |= WUE_NO_ONLINE_ACCOUNT;
    if (m_offline->isChecked())       f |= WUE_OFFLINE_DRIVES;
    if (m_createAccount->isChecked() && !m_username->text().trimmed().isEmpty())
        f |= WUE_SET_USER;
    return f;
}

QString WindowsCustomizeSection::username() const {
    return m_createAccount->isChecked() ? m_username->text().trimmed() : QString();
}

void WindowsCustomizeSection::setWindowsIso(bool isWindows) {
    setEnabled(isWindows);
}
