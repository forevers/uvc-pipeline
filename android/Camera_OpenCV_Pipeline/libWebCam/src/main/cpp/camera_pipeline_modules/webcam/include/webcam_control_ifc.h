#ifndef WEBCAM_CAMERA_CONTROL_IFC_H
#define WEBCAM_CAMERA_CONTROL_IFC_H

#include <jni.h>

#include "frame_access_registration_ifc.h"


class IWebcamControl {

public:

    virtual ~IWebcamControl() {};

    virtual IFrameAccessRegistration* GetFrameAccessIfc(int interface_number) = 0;

    virtual int Prepare(int vid, int pid, int fd, int busnum, int devaddr, const char *usbfs) = 0;

    virtual int Start() = 0;
    virtual int Stop() = 0;

    virtual int SetStatusCallback(JNIEnv *env, jobject status_callback_obj) = 0;

    virtual char* GetSupportedVideoModes() = 0;

    virtual int SetPreviewSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_frame_format) = 0;
};

#endif // WEBCAM_CAMERA_CONTROL_IFC_H
