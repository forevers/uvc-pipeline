//#pragma once
#ifndef CAMERA_H
#define CAMERA_H

//#include <jni.h>
//#include <pthread.h>
#include <future>
#include <vector>

#include "camera-control-ifc.h"
#include "output.h"
#include "rapidjson/document.h"

//#include "webcam_status_callback.h"
//#include "webcam_fifo.h"
//#include "frame_access_registration_ifc.h"
#include "sync-log.h"


struct buffer
{
    unsigned int idx;
    unsigned int padding[VIDEO_MAX_PLANES];
    unsigned int size[VIDEO_MAX_PLANES];
    uint8_t *mem[VIDEO_MAX_PLANES];
};

struct V4l2FormatInfo {
    const char *name;
    unsigned int fourcc;
    unsigned char n_planes;
};

struct Field {
    const char *name;
    enum v4l2_field field;
};

enum BufferFillMode
{
    BUFFER_FILL_NONE = 0,
    BUFFER_FILL_FRAME = 1 << 0,
    BUFFER_FILL_PADDING = 1 << 1,
};

struct CameraConfig {
    int bytes_per_pixel;
    int width_enumerated;
    int width_actual;
    int height;
    int bytes_per_row;
    int fps;
};

// +++++ from android project

typedef enum camera_frame_format {
    CAMERA_FRAME_FORMAT_UNKNOWN = 0,
    // supported webcam format
    CAMERA_FRAME_FORMAT_YUYV = 1,
    CAMERA_FRAME_FORMAT_MJPEG = 2,
    // 32-bit RGBA
    CAMERA_FRAME_FORMAT_RGBX = 3,
    /** Raw grayscale images */
    CAMERA_FRAME_FORMAT_GRAY_8 = 4,
    CAMERA_FRAME_FORMAT_GRAY_16 = 5,

    CAMERA_FRAME_FORMAT_RGB = 6,

    /** Number of formats understood */
    CAMERA_FRAME_FORMAT_COUNT,
} CameraFormat;

struct CameraFrame {

    /** image data */
    uint8_t* data;
    /** expected size of image data buffer */
    size_t data_bytes;
    /** actual received data of image frame */
    size_t actual_bytes;
    /** image width */
    uint32_t width;
    /** image height */
    uint32_t height;
    /** pixel data format */
    CameraFormat frame_format;
    /** bytes per horizontal line */
    size_t step;
    /** frame number */
    uint32_t sequence;
    /** estimate of system time when the device started capturing the image */
    struct timeval capture_time;
} ;

// ----- from android project


class Camera : public ICameraControl {

public:

    Camera();
    ~Camera();

    // CameraControl methods
    //IFrameAccessRegistration* GetFrameAccessIfc(int interface_number) override;
    //int Prepare(int vid, int pid, int fd, int busnum, int devaddr, const char *usbfs) override;
    int Start() override;
    int Stop() override;
    bool IsRunning() override;
    // +++++ TODO move to pipe interface
    virtual int UvcV4l2GetFrame(void) override;
    // ----- TODO move to pipe interface
    //int SetStatusCallback(JNIEnv *env, jobject status_callback_obj) override;
    // void RegisterSupportedCameras() override;
    std::string GetSupportedVideoModes() override;
    //int SetPreviewSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_format) override;
    // TODO ported from linux C gtkmm project ported
    // int QueryCapabilities(Device *dev, unsigned int *capabilities) override;
    int UvcV4l2Init(/*IFrameQueue* frame_queue_ifc, */std::string device_node, int enumerated_width, int enumerated_height/*, int actual_width, int actual_height*/) override;
    int UvcV4l2Exit(void) override;

    /* blocking client frame get ... pulls from end of queue */
    bool GetFrame(uint8_t** frame);

private:

    /* return number of cameras detected */
    bool DetectCameras();
    bool detected_;

    SyncLog* synclog_;

    // create promise ... can be issued anywhere in a context ... not just return value
    std::vector<std::future<std::shared_ptr<Output>>> future_vec_;

    // TODO udevadm monitor to see usb register and unregistered devices
    /* rapidJSON DOM- Default template parameter uses UTF8 and MemoryPoolAllocator */
    rapidjson::Document camera_doc_;

    void VideoInit(Device *device);
    const V4l2FormatInfo* V4l2FormatByName(const char *name);
    bool VideoHasFd(Device* device);
    int VideoOpen(Device *device, const char *devname);
    int VideoQueryCap(Device* device, unsigned int* capabilities);
    int CapGetBufType(unsigned int capabilities);
    bool VideoHasValidBufType(Device* device);
    void VideoSetBufType(Device* device, enum v4l2_buf_type type);
    int VideoSetFormat(Device* device, unsigned int w, unsigned int h,
        unsigned int format, unsigned int stride,
        enum v4l2_field field);
    const struct V4l2FormatInfo* V4l2FormatByFourCC(unsigned int fourcc);
    const char* V4l2FormatName(unsigned int fourcc);
    const char* V4l2FieldName(enum v4l2_field field);
    void VideoClose(Device* device);
    int VideoGetFormat(Device* device);
    int VideoSetFramerate(Device* device, v4l2_fract* time_per_frame);
    int VideoSetQuality(Device* device, unsigned int quality);
    int VideoPrepareCapture(Device*device, 
        int nbufs, 
        unsigned int offset,
        const char *filename, 
        enum BufferFillMode fill);
    int VideoAllocBuffers(Device* dev, 
        int nbufs,
        unsigned int offset, 
        unsigned int padding);
    void GetTsFlags(uint32_t flags, const char **ts_type, const char **ts_source);
    int VideoBufferMmap(Device* dev, struct buffer* buffer, struct v4l2_buffer* v4l2buf);
    int VideoBufferAllocUserptr(Device* device, struct buffer *buffer, struct v4l2_buffer *v4l2buf, unsigned int offset, unsigned int padding);
    bool VideoIsCapture(Device* device);
    bool VideoIsMplane(Device* device);
    bool VideoIsOutput(Device* device);
    int VideoQueueBuffer(Device* device, int index, BufferFillMode fill);
    int VideoEnable(Device* device, int enable);
    int VideoFreeBuffers(Device* device);
    int VideoBufferMunmap(Device* device, buffer* buffer);
    void VideoBufferFreeUserptr(Device* device, buffer* buffer);

    int VideoQueuebuffer(/*Device* device, */int index, BufferFillMode fill);


    //char* usb_fs_;

    //uvc_context_t* camera_context_;

    //int uvc_fd_;

    //uvc_device_t* camera_device_;

    //uvc_device_handle_t* camera_device_handle_;

    //WebcamStatusCallback* status_callback_;

    //WebcamFifo* webcam_fifo_;

    Device device_;

    static const V4l2FormatInfo pixel_formats_[];
    static const Field fields_[];

    // TODO output frame queue
    // cheat with bool for now
    bool frame_avail_;
    std::mutex frame_avail_mtx_;
    std::condition_variable frame_avail_cv_;

    /* V4L2 userspace frame puller thread */ 
    void FramePull(int width, int height);
    // TODO port frame queue
    std::thread* frame_pull_thread_;
    // TODO place under sync control
    bool request_start_;
//    bool request_stop_;
    bool is_running_;
    std::mutex frame_pull_thread_mutex_;
    CameraFrame uvc_frame_;
    CameraFrame rgb_frame_;
    int width_;
    int height_;
};

#endif // CAMERA_H
