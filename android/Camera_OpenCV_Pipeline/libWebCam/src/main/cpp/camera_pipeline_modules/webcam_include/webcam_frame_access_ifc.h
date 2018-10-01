#ifndef CAMERA_FRAME_ACCESS_IFC_H
#define CAMERA_FRAME_ACCESS_IFC_H

#include <memory>

#include "webcam_include.h"


class ICameraFrameAccess {

public:

    virtual ~ICameraFrameAccess() {};

    virtual CameraError GetFrameDescription(CameraFormat* camera_format, int* height, int* width) = 0;

    virtual CameraFrame* GetFrame(void) = 0;

    virtual void ReleaseFrame(CameraFrame* camera_frame) = 0;

    virtual void UnregisterClient(void) = 0;
};

#endif // CAMERA_FRAME_ACCESS_IFC_H
