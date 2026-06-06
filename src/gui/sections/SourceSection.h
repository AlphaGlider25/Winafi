#ifndef WINAFI_SOURCE_SECTION_H
#define WINAFI_SOURCE_SECTION_H
#include <QWidget>
#include <QString>
class QLineEdit; class QPushButton; class QLabel;
class SourceSection : public QWidget {
    Q_OBJECT
public:
    explicit SourceSection(QWidget *parent = nullptr);
    QString isoPath() const;
    void setOsBadge(const QString &osText);
    void setHash(const QString &shortHash);
signals:
    void isoChosen(const QString &path);
    void verifyRequested();
    void hashRequested();
private:
    QLineEdit *m_isoEdit; QPushButton *m_browse, *m_verify, *m_hash;
    QLabel *m_osBadge, *m_hashLabel;
};
#endif
