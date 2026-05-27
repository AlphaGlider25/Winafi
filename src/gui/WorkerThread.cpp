#include "WorkerThread.h"
#include "winafi.h"
#include <QDebug>

static void progress_callback(int percent, const char *message, void *user_data) {
    WorkerThread *worker = static_cast<WorkerThread *>(user_data);
    QMetaObject::invokeMethod(worker, [worker, percent, message]() {
        emit worker->progressUpdated(percent, QString::fromUtf8(message));
    }, Qt::QueuedConnection);
}

WorkerThread::WorkerThread(const QString &isoPath, const QString &devnode,
                          const QString &volumeLabel, const QString &fileSystem,
                          uint32_t clusterSize, bool quickFormat, int imageOption, QObject *parent)
    : QThread(parent), m_isoPath(isoPath), m_devnode(devnode),
      m_volumeLabel(volumeLabel), m_fileSystem(fileSystem),
      m_clusterSize(clusterSize), m_quickFormat(quickFormat), m_imageOption(imageOption) {}

WorkerThread::~WorkerThread() {
    wait();
}

void WorkerThread::run() {
    winafi_session_t *session = winafi_session_create();
    if (!session) {
        emit finished(false, "SESSION_CREATE_FAILED", "Failed to create session");
        return;
    }

    // Load ISO
    if (winafi_session_load_iso(session, m_isoPath.toUtf8().constData()) != WINAFI_OK) {
        const char *error = winafi_get_error_message(session);
        emit finished(false, "ISO_LOAD_FAILED", QString::fromUtf8(error));
        winafi_session_destroy(session);
        return;
    }

    // Enumerate devices
    winafi_device_t *devices = nullptr;
    int device_count = 0;
    if (winafi_enumerate_devices(session, &devices, &device_count) != WINAFI_OK) {
        const char *error = winafi_get_error_message(session);
        emit finished(false, "DEVICE_SELECT_FAILED", QString::fromUtf8(error));
        winafi_session_destroy(session);
        return;
    }

    // Select device
    if (winafi_session_select_device(session, m_devnode.toUtf8().constData()) != WINAFI_OK) {
        const char *error = winafi_get_error_message(session);
        emit finished(false, "DEVICE_SELECT_FAILED", QString::fromUtf8(error));
        winafi_session_destroy(session);
        return;
    }

    // Prepare
    if (winafi_session_prepare(session) != WINAFI_OK) {
        const char *error = winafi_get_error_message(session);
        emit finished(false, "PREPARE_FAILED", QString::fromUtf8(error));
        winafi_session_destroy(session);
        return;
    }

    // Set format options
    winafi_session_set_volume_label(session, m_volumeLabel.toUtf8().constData());
    winafi_session_set_quick_format(session, m_quickFormat ? 1 : 0);
    winafi_session_set_image_option(session, static_cast<winafi_image_option_t>(m_imageOption));

    // Execute with progress callback
    winafi_set_progress_callback(session, progress_callback, this);
    if (winafi_session_execute(session) != WINAFI_OK) {
        const char *error = winafi_get_error_message(session);
        emit finished(false, "EXECUTE_FAILED", QString::fromUtf8(error));
        winafi_session_destroy(session);
        return;
    }

    emit finished(true, "", "");
    winafi_session_destroy(session);
}
