#include "sections/TargetSection.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

TargetSection::TargetSection(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    auto *title = new QLabel(tr("Target device")); title->setStyleSheet("font-weight:600;");
    root->addWidget(title);

    auto *row = new QHBoxLayout();
    m_combo = new QComboBox(); m_refresh = new QPushButton(tr("Refresh")); m_refresh->setObjectName("Ghost");
    row->addWidget(m_combo, 1); row->addWidget(m_refresh);
    root->addLayout(row);

    m_showHdd = new QCheckBox(tr("Show hard drives (dangerous)"));
    m_showHdd->setToolTip(tr("By default only removable USB drives are listed to prevent wiping a disk."));
    root->addWidget(m_showHdd);

    m_info = new QLabel(); m_info->setStyleSheet("color:#9d96b8;");
    root->addWidget(m_info);
    root->addStretch(1);

    connect(m_refresh, &QPushButton::clicked, this, &TargetSection::refreshRequested);
    connect(m_showHdd, &QCheckBox::toggled, this, &TargetSection::showHardDrivesToggled);
    connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ emit deviceChanged(); });
}
QString TargetSection::selectedDevnode() const { return m_combo->currentData().toString(); }
bool TargetSection::showHardDrives() const { return m_showHdd->isChecked(); }
void TargetSection::clearDevices() { m_combo->clear(); }
void TargetSection::addDevice(const QString &label, const QString &devnode) { m_combo->addItem(label, devnode); }
void TargetSection::setInfo(const QString &t) { m_info->setText(t); }
