#pragma once

#include <gtkmm.h>
#include <mutex>
#include "sync-log.h"
// TODO consider thread construct in Worker context
// #include <thread>

#include "camera.h"
#include "camera-frame-queue-server-ifc.h"


class Worker
{
public:

    // TODO qt dynamically constructs with a camera reference
    // Worker(std::shared_ptr<Camera> camera);
    Worker();
    ~Worker();

    /* thread function */
    
    void do_work(std::shared_ptr<Camera> camera, ICameraFrameQueueClient* queue_client, ICameraFrameQueueServer* queue_server);
    // void do_work(ICameraFrameQueueClient* queue_client, ICameraFrameQueueServer* queue_server);
    // void do_work(std::shared_ptr<Camera> camera, ICameraFrameQueueServer* queue_server);

    void stop_work();
    bool has_stopped() const;

private:

    /* synchronize access to member data */
    mutable std::mutex mutex_;

    /* data used by both GUI thread and worker thread */
    bool shall_stop_;
    bool has_stopped_;

    /* console logger */
    std::shared_ptr<SyncLog> synclog_;
};
