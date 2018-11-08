#ifndef FRAME_QUEUE_IFC_H
#define FRAME_QUEUE_IFC_H

#include <memory>

#include "webcam_include.h"

class IFrameQueue {

public:

    virtual ~IFrameQueue() {};

    virtual void Signal() = 0;

    //virtual CameraError GetFrameDescription(CameraFormat* camera_format, int* height, int* width) = 0;

    // virtual Frame GetFrame(void) = 0;
    virtual CameraFrame GetFrame(void) = 0;

    //virtual void ReleaseFrame(CameraFrame* camera_frame) = 0;

    //virtual void UnregisterClient(void) = 0;
};

#endif // FRAME_ACCESS_IFC_H
