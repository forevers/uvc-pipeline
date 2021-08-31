#pragma once

#include "camera_types.h"


class ICameraFrameQueueClient
{
public:
    // virtual CameraFrame* GetFrame() = 0;
    virtual bool GetFrame(CameraFrame** frame) = 0;
    virtual void ReturnFrame(CameraFrame* frame) = 0;
    // TODO have this signal be in the interface destructor client releases this interface ... 
};
