#pragma once

#include "camera-frame-queue-client-ifc.h"
#include "camera_types.h"

#include <memory.h>

// class ICameraFrameQueueClient;

class ICameraFrameQueueServer
{
public:
    virtual void Init(size_t num_elems, size_t data_bytes) = 0;
    // virtual int AddFrameToQueue(CameraFrame *frame, bool is_running) = 0;
    virtual int AddFrameToQueue(CameraFrame *frame) = 0;
    virtual CameraFrame* GetPoolFrame() = 0;
    virtual std::shared_ptr<ICameraFrameQueueClient> ClientCameraQueueIfc() = 0;
};
