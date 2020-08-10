#pragma once

//#include <jni.h>

//#include "frame_access_registration_ifc.h"

#include <string>

class ICameraControl {

public:

    virtual ~ICameraControl() {};

    //virtual IFrameAccessRegistration* GetFrameAccessIfc(int interface_number) = 0;

    //virtual int Prepare(int vid, int pid, int fd, int busnum, int devaddr, const char *usbfs) = 0;

    virtual int Start() = 0;
    virtual int Stop() = 0;

    //virtual int SetStatusCallback(JNIEnv *env, jobject status_callback_obj) = 0;
    // virtual std::string GetSupportedVideoModes() = 0;

private:

    //virtual char* GetSupportedVideoModes() = 0;

    //virtual int SetPreviewSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_frame_format) = 0;
};
