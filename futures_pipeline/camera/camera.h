#pragma once

//#include <jni.h>
//#include <pthread.h>
#include <future>
#include <vector>

#include "camera-control-ifc.h"
#include "output.h"
//#include "webcam_status_callback.h"
//#include "webcam_fifo.h"
//#include "frame_access_registration_ifc.h"
#include "sync-log.h"






class Camera : public ICameraControl {

public:

    Camera();
    ~Camera();

    // CameraControl methods
    //IFrameAccessRegistration* GetFrameAccessIfc(int interface_number) override;
    //int Prepare(int vid, int pid, int fd, int busnum, int devaddr, const char *usbfs) override;
    int Start() override;
    int Stop() override;
    //int SetStatusCallback(JNIEnv *env, jobject status_callback_obj) override;
    //char* 
    void GetSupportedCameras() override;
    //char* GetSupportedVideoModes() override;
    //int SetPreviewSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_format) override;

private:

    SyncLog* synclog_;

    // create promise ... can be issued anywhere in a context ... not just return value
    std::vector<std::future<std::shared_ptr<Output>>> future_vec_;


    //char* usb_fs_;

    //uvc_context_t* camera_context_;

    //int uvc_fd_;

    //uvc_device_t* camera_device_;

    //uvc_device_handle_t* camera_device_handle_;

    //WebcamStatusCallback* status_callback_;

    //WebcamFifo* webcam_fifo_;
};
