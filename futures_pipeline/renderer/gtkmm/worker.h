#pragma once

#include <gtkmm.h>
#include <mutex>
#include "sync-log.h"

#include "camera.h"
#include "camera-frame-queue-server-ifc.h"


class Worker
{
public:

    Worker();
    ~Worker();

    /* thread function */
    void do_work(std::shared_ptr<Camera> camera, ICameraFrameQueueClient* queue_client, ICameraFrameQueueServer* queue_server);

    void stop_work();
    bool has_stopped() const;

private:

    /* synchronize stop variable access to member data */
    mutable std::mutex mutex_stop_;
    /* data used by both GUI thread and worker thread */
    bool request_stop_;
    bool has_stopped_;

    /* console logger */
    std::shared_ptr<SyncLog> synclog_;
};
