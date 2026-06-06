#ifndef ISO_HASH_WORKER_THREAD_H
#define ISO_HASH_WORKER_THREAD_H

#include <QThread>
#include <QString>

enum class HashType {
    MD5 = 0,
    SHA256 = 1,
    SHA512 = 2
};

class ISOHashWorkerThread : public QThread {
    Q_OBJECT

public:
    explicit ISOHashWorkerThread(const QString &isoPath, const QString &expectedHash, HashType hashType);
    ~ISOHashWorkerThread();

protected:
    void run() override;

signals:
    void progressUpdated(int percent);
    void verifyComplete(bool match, QString message);

private:
    QString computeHash(HashType type);

    QString iso_path_;
    QString expected_hash_;
    HashType hash_type_;
};

#endif // ISO_HASH_WORKER_THREAD_H
