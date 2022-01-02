#pragma once

#include <future>
#include <vector>

#include "camera-capabilities.h"
#include "camera-control-ifc.h"
#include "output.h"
#include "frame-queue.h"
#include "rapidjson/document.h"
#include "sync-log.h"
#include "camera_types.h"

class Camera : public ICameraControl
{

public:
    Camera();
    ~Camera();

    // CameraControl methods
    //IFrameAccessRegistration* GetFrameAccessIfc(int interface_number) override;
    // TODO move these into a pipeline interface
    int Start(const CameraConfig& camera_config) override;

    int Stop() override;

    bool IsRunning() override;
    // +++++ TODO move to pipe interface
    virtual int UvcV4l2GetFrame(void) override;
    // ----- TODO move to pipe interface
    // void RegisterSupportedCameras() override;
    std::string GetSupportedVideoModes() override;
    //int SetPreviewSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_format) override;
    // TODO ported from linux C gtkmm project ported
    // int QueryCapabilities(Device *dev, unsigned int *capabilities) override;
    virtual int UvcV4l2Init(const CameraConfig& camera_config) override;
    int UvcV4l2Exit(void) override;

    /* blocking client frame get ... pulls from end of queue */
    // TODO temp is straight buffer pull
    bool GetFrame(uint8_t **frame);
    bool GetFrame(CameraFrame **frame);
    bool ReturnFrame(CameraFrame *frame);

private:
    /* platforma cameras capabilities: dev node, mode, resolution, and rate */
    std::shared_ptr<CameraCapabilities> camera_capabilities_;

    /* selected camera configuration */
    CameraConfig camera_config_;

    /* console logger */
    std::shared_ptr<SyncLog> synclog_;

    // create promise ... can be issued anywhere in a context ... not just return value
    std::vector<std::future<std::shared_ptr<Output>>> future_vec_;

    // // TODO udevadm monitor to see usb register and unregistered devices

    void VideoInit(Device *device);
    const V4l2FormatInfo *V4l2FormatByName(const char *name);
    bool VideoHasFd(Device *device);
    int VideoOpen(Device *device, const char *devname);
    int VideoQueryCap(Device *device, unsigned int *capabilities);
    int CapGetBufType(unsigned int capabilities);
    bool VideoHasValidBufType(Device *device);
    void VideoSetBufType(Device *device, enum v4l2_buf_type type);
    int VideoSetFormat(Device *device, unsigned int w, unsigned int h,
                       unsigned int format, unsigned int stride,
                       enum v4l2_field field);
    const struct V4l2FormatInfo *V4l2FormatByFourCC(unsigned int fourcc);
    const char *V4l2FormatName(unsigned int fourcc);
    const char *V4l2FieldName(enum v4l2_field field);
    void VideoClose(Device *device);
    int VideoGetFormat(Device *device);
    int VideoSetFramerate(Device *device, v4l2_fract *time_per_frame);
    int VideoSetQuality(Device *device, unsigned int quality);
    int VideoPrepareCapture(Device *device,
                            int nbufs,
                            unsigned int offset,
                            const char *filename,
                            enum BufferFillMode fill);
    int VideoAllocBuffers(Device *dev,
                          int nbufs,
                          unsigned int offset,
                          unsigned int padding);
    void GetTsFlags(uint32_t flags, const char **ts_type, const char **ts_source);
    int VideoBufferMmap(Device *dev, struct buffer *buffer, struct v4l2_buffer *v4l2buf);
    int VideoBufferAllocUserptr(Device *device, struct buffer *buffer, struct v4l2_buffer *v4l2buf, unsigned int offset, unsigned int padding);
    bool VideoIsCapture(Device *device);
    bool VideoIsMplane(Device *device);
    bool VideoIsOutput(Device *device);
    int VideoQueueBuffer(Device *device, int index, BufferFillMode fill);
    int VideoEnable(Device *device, int enable);
    int VideoFreeBuffers(Device *device);
    int VideoBufferMunmap(Device *device, buffer *buffer);
    void VideoBufferFreeUserptr(Device *device, buffer *buffer);

    int VideoQueuebuffer(/*Device* device, */ int index, BufferFillMode fill);

    Device device_;

    static const V4l2FormatInfo pixel_formats_[];
    static const Field fields_[];

    /* client frame queue */
    std::shared_ptr<FrameQueue<CameraFrame>> camera_frame_queue_;
    /* queue frame available conditional signal */
    std::mutex release_frame_queue_mtx_;
    std::condition_variable release_frame_queue_cv;

    // TODO output frame queue
    // cheat with bool for now
    bool frame_avail_;
    std::mutex frame_avail_mtx_;
    std::condition_variable frame_avail_cv_;

    /* V4L2 userspace frame puller thread */
    void FramePull(int width, int height);

    std::thread frame_pull_thread_;

    // TODO place under sync control
    bool request_start_;
    bool is_running_;
    std::mutex frame_pull_thread_mutex_;
    CameraFrame uvc_frame_;
    CameraFrame rgb_frame_;
};
