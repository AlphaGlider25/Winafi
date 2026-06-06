#ifndef WINAFI_ADVANCED_SECTION_H
#define WINAFI_ADVANCED_SECTION_H
#include <QWidget>
#include <QString>
#include <cstdint>
class QComboBox; class QLineEdit; class QCheckBox;
class AdvancedSection : public QWidget {
    Q_OBJECT
public:
    explicit AdvancedSection(QWidget *parent = nullptr);
    QString fileSystem() const;
    QString volumeLabel() const;
    uint32_t clusterSize() const;
    bool quickFormat() const;
signals:
    void changed();
private:
    QComboBox *m_partScheme, *m_targetSys, *m_fs, *m_cluster;
    QLineEdit *m_label; QCheckBox *m_quick;
};
#endif
