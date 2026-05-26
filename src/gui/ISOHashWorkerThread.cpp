#include "ISOHashWorkerThread.h"
#include <QFile>
#include <QCryptographicHash>
#include <QDebug>

ISOHashWorkerThread::ISOHashWorkerThread(const QString &isoPath, const QString &expectedHash, HashType hashType)
    : iso_path_(isoPath), expected_hash_(expectedHash), hash_type_(hashType) {
}

ISOHashWorkerThread::~ISOHashWorkerThread() {
    wait();
}

void ISOHashWorkerThread::run() {
    if (isInterruptionRequested()) {
        return;
    }

    emit progressUpdated(0);

    QString computed_hash = computeHash(hash_type_);

    if (isInterruptionRequested()) {
        return;
    }

    emit progressUpdated(100);

    // Normalize hashes for comparison (case-insensitive)
    QString normalized_expected = expected_hash_.toLower().trimmed();
    QString normalized_computed = computed_hash.toLower().trimmed();

    bool match = (normalized_expected == normalized_computed);

    if (match) {
        emit verifyComplete(true, "Hash verification successful! Hashes match.");
    } else {
        QString message = QString(
            "Hash verification failed!\n"
            "Expected: %1\n"
            "Computed: %2"
        ).arg(normalized_expected, normalized_computed);
        emit verifyComplete(false, message);
    }
}

QString ISOHashWorkerThread::computeHash(HashType type) {
    QFile file(iso_path_);
    if (!file.open(QIODevice::ReadOnly)) {
        emit verifyComplete(false, "Failed to open ISO file for reading.");
        return "";
    }

    QCryptographicHash::Algorithm algorithm;
    switch (type) {
        case HashType::MD5:
            algorithm = QCryptographicHash::Md5;
            break;
        case HashType::SHA256:
            algorithm = QCryptographicHash::Sha256;
            break;
        case HashType::SHA512:
            algorithm = QCryptographicHash::Sha512;
            break;
        default:
            algorithm = QCryptographicHash::Sha256;
    }

    QCryptographicHash hash(algorithm);
    const int CHUNK_SIZE = 1024 * 1024; // 1MB chunks
    qint64 file_size = file.size();
    qint64 bytes_read = 0;

    while (!file.atEnd()) {
        if (isInterruptionRequested()) {
            file.close();
            return "";
        }

        QByteArray chunk = file.read(CHUNK_SIZE);
        if (chunk.isEmpty()) {
            break;
        }

        hash.addData(chunk);
        bytes_read += chunk.size();

        int progress = static_cast<int>((bytes_read * 100) / file_size);
        emit progressUpdated(progress);
    }

    file.close();
    return QString::fromLatin1(hash.result().toHex());
}
