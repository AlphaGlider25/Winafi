#ifndef WINAFI_TARGET_SECTION_H
#define WINAFI_TARGET_SECTION_H
#include <QWidget>
#include <QString>
class QComboBox; class QPushButton; class QCheckBox; class QLabel;
class TargetSection : public QWidget {
    Q_OBJECT
public:
    explicit TargetSection(QWidget *parent = nullptr);
    QString selectedDevnode() const;
    bool showHardDrives() const;
    void clearDevices();
    void addDevice(const QString &label, const QString &devnode);
    void setInfo(const QString &text);
signals:
    void refreshRequested();
    void showHardDrivesToggled(bool on);
    void deviceChanged();
private:
    QComboBox *m_combo; QPushButton *m_refresh; QCheckBox *m_showHdd; QLabel *m_info;
};
#endif
