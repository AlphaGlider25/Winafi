#include "sections/AdvancedSection.h"
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>

AdvancedSection::AdvancedSection(QWidget *parent) : QWidget(parent) {
    auto *form = new QFormLayout(this);
    m_partScheme = new QComboBox(); m_partScheme->addItems({"GPT","MBR"});
    m_targetSys  = new QComboBox(); m_targetSys->addItems({"UEFI","BIOS"});
    m_fs         = new QComboBox(); m_fs->addItems({"NTFS","FAT32","exFAT"});
    m_cluster    = new QComboBox(); m_cluster->addItem(tr("Auto"), 0u);
    m_label      = new QLineEdit("WINAFI");
    m_quick      = new QCheckBox(tr("Quick format")); m_quick->setChecked(true);

    form->addRow(tr("Partition scheme:"), m_partScheme);
    form->addRow(tr("Target system:"), m_targetSys);
    form->addRow(tr("File system:"), m_fs);
    form->addRow(tr("Cluster size:"), m_cluster);
    form->addRow(tr("Volume label:"), m_label);
    form->addRow(QString(), m_quick);

    auto fire = [this]{ emit changed(); };
    connect(m_fs, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [fire](int){ fire(); });
    connect(m_label, &QLineEdit::textChanged, this, [fire]{ fire(); });
    connect(m_quick, &QCheckBox::toggled, this, [fire]{ fire(); });
}
QString AdvancedSection::fileSystem() const { return m_fs->currentText(); }
QString AdvancedSection::volumeLabel() const { return m_label->text(); }
uint32_t AdvancedSection::clusterSize() const { return m_cluster->currentData().toUInt(); }
bool AdvancedSection::quickFormat() const { return m_quick->isChecked(); }
