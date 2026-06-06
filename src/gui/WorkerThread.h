#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QThread>
#include <QString>

class WorkerThread : public QThread {
    Q_OBJECT

public:
    explicit WorkerThread(const QString &isoPath, const QString &devnode,
                         const QString &volumeLabel, const QString &fileSystem,
                         uint32_t clusterSize, bool quickFormat, int imageOption,
                         int wueFlags, const QString &username, QObject *parent = nullptr);
    ~WorkerThread();

signals:
    void progressUpdated(int percent, QString message);
    void finished(bool success, QString errorCode, QString errorMsg);

protected:
    void run() override;

private:
    QString m_isoPath;
    QString m_devnode;
    QString m_volumeLabel;
    QString m_fileSystem;
    uint32_t m_clusterSize;
    bool m_quickFormat;
    int m_imageOption;
    int m_wueFlags;
    QString m_username;
};

#endif // WORKERTHREAD_H
