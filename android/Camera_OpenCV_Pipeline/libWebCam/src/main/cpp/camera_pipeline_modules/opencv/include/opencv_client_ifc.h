#ifndef OPENCV_CLIENT_IFC_H
#define OPENCV_CLIENT_IFC_H

#include "webcam_frame_access_ifc.h"
#include "webcam_include.h"

class IOpenCVClient : public ICameraFrameAccess {

public:

    virtual ~IOpenCVClient() {};

    // get frame from a channel interface
    virtual CameraFrame* GetFrame(void) = 0;

    // unlock channel buffer and release reference to channel frame
    virtual void ReleaseFrame(CameraFrame* camera_frame) = 0;

    virtual void Unregister(void) = 0;
};

#endif // OPENCV_CLIENT_IFC_H
