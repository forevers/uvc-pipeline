#pragma once

//#include <jni.h>

//#include "frame_access_registration_ifc.h"

#include <string>

#include "videodev2.h"
// TODO tailor to android libv4l2and linux v4l2
struct Device
{
    int fd;
    int opened;

    enum v4l2_buf_type type;
    enum v4l2_memory memtype;
    unsigned int nbufs;
    struct buffer *buffers;

    unsigned int width;
    unsigned int height;
    uint32_t buffer_output_flags;
    uint32_t timestamp_type;

    unsigned char num_planes;
    struct v4l2_plane_pix_format plane_fmt[VIDEO_MAX_PLANES];

    void *pattern[VIDEO_MAX_PLANES];
    unsigned int patternsize[VIDEO_MAX_PLANES];
};



class ICameraControl {

public:

    virtual ~ICameraControl() {};

    //virtual IFrameAccessRegistration* GetFrameAccessIfc(int interface_number) = 0;

    //virtual int Prepare(int vid, int pid, int fd, int busnum, int devaddr, const char *usbfs) = 0;

    virtual int Start() = 0;
    virtual int Stop() = 0;

    //virtual int SetStatusCallback(JNIEnv *env, jobject status_callback_obj) = 0;
    virtual std::string GetSupportedVideoModes() = 0;
    virtual int UvcV4l2Init(/*IFrameQueue* frame_queue_ifc, */std::string device_node, int enumerated_width, int enumerated_height/*, int actual_width, int actual_height*/) = 0;
    virtual int UvcV4l2Exit(void) = 0;

    // virtual int QueryCapabilities(Device *dev, unsigned int *capabilities) = 0;

private:

    //virtual char* GetSupportedVideoModes() = 0;

    //virtual int SetPreviewSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_frame_format) = 0;
};
