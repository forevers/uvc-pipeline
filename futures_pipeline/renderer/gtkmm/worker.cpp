#include "worker.h"

#include <iostream>
#include <sstream>
#include <chrono>

using namespace std;


// Worker::Worker(std::shared_ptr<Camera> camera) :
Worker::Worker() :
    // TODO consider camera param during construction
    // camera_{camera},
    mutex_{},
    shall_stop_{false},
    has_stopped_{false},
    synclog_{SyncLog::GetLog()}
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
}


Worker::~Worker()
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
}


void Worker::stop_work()
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry\n");

    std::lock_guard<std::mutex> lock(mutex_);
    // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","lock obtained\n");

    shall_stop_ = true;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit\n");
}


bool Worker::has_stopped() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return has_stopped_;
}


// TODO std::shared_ptr<Camera> camera to to support access camera-frame_queue-client-ifc
// TODO Renderer renderer to pipe_push_ifc
void Worker::do_work(std::shared_ptr<Camera> camera, ICameraFrameQueueClient* queue_client, ICameraFrameQueueServer* queue_server)
// void Worker::do_work(ICameraFrameQueueClient* queue_client, ICameraFrameQueueServer* queue_server)
// void Worker::do_work(std::shared_ptr<Camera> camera, ICameraFrameQueueServer* queue_server)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    // TODO get rid of
    shall_stop_ = false;
    has_stopped_ = false;

    bool run = true;
    while (run) {
        CameraFrame* frame;
        // TODO change camera to camera-frame-queue-client-ifc
        // TODO GetFrame() returns actual frame or nullptr never error
        if (queue_client->GetFrame(&frame)) {
        // if (camera->GetFrame(&frame)) {
            if (frame == nullptr) {
                synclog_->LogV("[",__func__,": ",__LINE__,"]: "," **************************************");
                synclog_->LogV("[",__func__,": ",__LINE__,"]: "," ******* received nullptr frame *******");
                synclog_->LogV("[",__func__,": ",__LINE__,"]: "," **************************************");
                run = false;
            } else {
                /* synchronous addition of frame to UI owned context */
                queue_server->AddFrameToQueue(frame);
            }
        } else {
            cout << "camera->GetFrame(&frame) failure" << endl;
        }
    }

    /* signal frame access ifc we no longer require it */
    camera->Release();

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit do_work() loop");

    // TODO find different signalling means ... shared_ptr nulling ...
    shall_stop_ = false;
    has_stopped_ = true;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","pre AddFrameToQueue(nullptr)");

    /* indicate worker thread exit */
    queue_server->AddFrameToQueue(nullptr);

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","post AddFrameToQueue(nullptr)");
}
