#include "sections/SourceSection.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>

SourceSection::SourceSection(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    auto *title = new QLabel(tr("Source image")); title->setStyleSheet("font-weight:600;");
    root->addWidget(title);

    auto *row = new QHBoxLayout();
    m_isoEdit = new QLineEdit(); m_isoEdit->setPlaceholderText(tr("Select a Windows or Linux ISO…"));
    m_browse  = new QPushButton(tr("Browse"));
    m_verify  = new QPushButton(tr("Verify")); m_verify->setObjectName("Ghost");
    m_hash    = new QPushButton(tr("Compute hash")); m_hash->setObjectName("Ghost");
    row->addWidget(m_isoEdit, 1); row->addWidget(m_browse); row->addWidget(m_verify); row->addWidget(m_hash);
    root->addLayout(row);

    m_osBadge = new QLabel(); m_osBadge->setStyleSheet("color:#9d96b8;");
    m_hashLabel = new QLabel(); m_hashLabel->setStyleSheet("color:#9d96b8;font-family:monospace;");
    root->addWidget(m_osBadge);
    root->addWidget(m_hashLabel);
    root->addStretch(1);

    connect(m_browse, &QPushButton::clicked, this, [this]{
        QString f = QFileDialog::getOpenFileName(this, tr("Select ISO"), QString(), tr("ISO images (*.iso)"));
        if (!f.isEmpty()) { m_isoEdit->setText(f); emit isoChosen(f); }
    });
    connect(m_verify, &QPushButton::clicked, this, &SourceSection::verifyRequested);
    connect(m_hash,   &QPushButton::clicked, this, &SourceSection::hashRequested);
}
QString SourceSection::isoPath() const { return m_isoEdit->text(); }
void SourceSection::setOsBadge(const QString &t) { m_osBadge->setText(t.isEmpty()? QString() : tr("Detected: %1").arg(t)); }
void SourceSection::setHash(const QString &h) { m_hashLabel->setText(h.isEmpty()? QString() : "SHA-256: " + h); }
