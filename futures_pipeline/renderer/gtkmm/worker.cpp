#include "worker.h"

#include <iostream>
#include <sstream>
#include <chrono>

using namespace std;


Worker::Worker() :
    mutex_stop_{},
    request_stop_{false},
    has_stopped_{false},
    synclog_{SyncLog::GetLog()}
{
    synclog_->LogV(FFL,"entry");
    synclog_->LogV(FFL,"exit");
}


Worker::~Worker()
{
    synclog_->LogV(FFL,"entry");
    synclog_->LogV(FFL,"exit");
}


void Worker::stop_work()
{
    synclog_->LogV(FFL,"entry\n");

    std::lock_guard<std::mutex> lock(mutex_stop_);
    request_stop_ = true;

    synclog_->LogV(FFL,"exit\n");
}


bool Worker::has_stopped() const
{
    std::lock_guard<std::mutex> lock(mutex_stop_);
    return has_stopped_;
}


// TODO std::shared_ptr<Camera> camera to to support access camera-frame_queue-client-ifc
// TODO Renderer renderer to pipe_push_ifc
void Worker::do_work(std::shared_ptr<Camera> camera, ICameraFrameQueueClient* queue_client, ICameraFrameQueueServer* queue_server)
{
    synclog_->LogV(FFL,"entry");

    request_stop_ = false;
    has_stopped_ = false;

    bool run = true;
    while (run) {

        CameraFrame* frame;

        // TODO GetFrame() returns actual frame or nullptr, ... never error
        if (queue_client->GetFrame(&frame)) {
            if (frame == nullptr) {
                synclog_->LogV(FFL," **************************************");
                synclog_->LogV(FFL," ******* received nullptr frame *******");
                synclog_->LogV(FFL," **************************************");
                run = false;
            } else {
                /* synchronous addition of frame to UI owned context */
                queue_server->AddFrameToQueue(frame);
            }
        } else {
            cout << "camera->GetFrame(&frame) failure" << endl;
        }

        /* stream termination signaled from ui context */
        {
            std::lock_guard<std::mutex> lock(mutex_stop_);
            if (request_stop_ == true) {
                run = false;
            }
        }
    }

    has_stopped_ = true;

    synclog_->LogV(FFL,"pre Stop()");
    camera->Stop();
    synclog_->LogV(FFL,"post Stop()");

    request_stop_ = false;
}
