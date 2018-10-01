#ifndef WEBCAM_FRAME_ACCESS_IMPL_H
#define WEBCAM_FRAME_ACCESS_IMPL_H

#include <memory>

#include "webcam_frame_access_ifc.h"
#include "util.h"

//#define WEBCAM_FRAME_ACCESS_IMPL_TAG "WEBCAM_FRAME_ACCESS_IMPL"
#define WEBCAM_FRAME_ACCESS_IMPL_TAG "test"

class WebcamFrameAccessIfc : public ICameraFrameAccess {

public:

    WebcamFrameAccessIfc(ICameraFrameAccess* frame_access) :
            camera_frame_access_(frame_access) {
        ENTER_(WEBCAM_FRAME_ACCESS_IMPL_TAG);
        EXIT_(WEBCAM_FRAME_ACCESS_IMPL_TAG);
    }

    ~WebcamFrameAccessIfc() {
        ENTER_(WEBCAM_FRAME_ACCESS_IMPL_TAG);

        // signal indicating client unregistration
        UnregisterClient();

        EXIT_(WEBCAM_FRAME_ACCESS_IMPL_TAG);
    }

    // ICameraFrameAccess methods
    virtual CameraError GetFrameDescription(CameraFormat* camera_format, int* height, int* width) {

        CameraError camera_error = CAMERA_ERROR_OTHER;

        if (camera_frame_access_) {
            camera_error = camera_frame_access_->GetFrameDescription(camera_format, height, width);
        }

        return camera_error;
    }

    virtual CameraFrame* GetFrame(void) {

        CameraFrame* camera_frame = nullptr;
        if (camera_frame_access_) {
            camera_frame = camera_frame_access_->GetFrame();
        }

        return camera_frame;
    }

    virtual void ReleaseFrame(CameraFrame* camera_frame) {

        if (camera_frame_access_) {
            camera_frame_access_->ReleaseFrame(camera_frame);
        }
    }

    virtual void UnregisterClient(void) {
        ENTER_(WEBCAM_FRAME_ACCESS_IMPL_TAG);

        if (camera_frame_access_) {
            camera_frame_access_->UnregisterClient();
        }

        EXIT_(WEBCAM_FRAME_ACCESS_IMPL_TAG);
    }

private:

    ICameraFrameAccess* camera_frame_access_;
};

#endif // WEBCAM_FRAME_ACCESS_IMPL_H
