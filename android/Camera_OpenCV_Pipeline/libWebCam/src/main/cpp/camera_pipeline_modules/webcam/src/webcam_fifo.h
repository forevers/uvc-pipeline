#ifndef WEBCAM_FIFO_H
#define WEBCAM_FIFO_H

//#define #define TAG_WEBCAM_FIFO "WEBCAM_FIFO"
#define TAG_WEBCAM_FIFO "test"

#include <boost/circular_buffer.hpp>
#include <jni.h>
#include <pthread.h>
#include <stddef.h>

#include "frame_queue_webcam.h"
#include "libuvc.h"
#include "webcam_control_ifc.h"
#include "webcam_status_callback.h"
#include "webcam_frame_access_ifc.h"
#include "frame_access_registration_ifc.h"
#include "webcam_include.h"

// frame queue and pool sizes
#define MAX_FRAMES 8
#define FRAME_POOL_SZ (MAX_FRAMES + 2)

#define DEFAULT_UVC_PREVIEW_FPS_MIN 1
#define DEFAULT_UVC_PREVIEW_FPS_MAX 15

// handle on an open camera device
struct camera_device_handle {
    // camera device handle
    void* dev;
};
typedef struct camera_device_handle camera_device_handle_t;


class WebcamFifo : public IFrameAccessRegistration, public ICameraFrameAccess {

public:

    WebcamFifo(uvc_device_handle_t* uvc_device_handle);
    ~WebcamFifo();

    // IFrameAccessRegistration methods
    FrameAccessSharedPtr RegisterClient(void) override;

    // ICameraFrameAccess methods
    CameraError GetFrameDescription(CameraFormat* mode, int* height, int* width) override;
    CameraFrame* GetFrame(void) override;
    void ReleaseFrame(CameraFrame* camera_frame) override;
    void UnregisterClient(void) override;

    inline const bool IsRunning() const;

    int SetFrameSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_format);

    // start frame accumulation
    int StartFrameAcquisition();
    // stop frame accumulation
    int StopFrameAcquisition();

    void DeleteFrameQueue(void) {
        if (camera_frame_queue_) delete camera_frame_queue_;
    }

    void LockQueueMutex(void) {
        if (camera_frame_queue_) camera_frame_queue_->MutexLock();
    }

    void UnlockQueueMutex(void) {
        if (camera_frame_queue_) camera_frame_queue_->MutexUnlock();
    }

    void CondSignalQueue() {
        LockQueueMutex();
        if (camera_frame_queue_) camera_frame_queue_->CondSignal();
        UnlockQueueMutex();
    }

    void NullFrameOnEmpty(bool enable) {
        if (camera_frame_queue_) camera_frame_queue_->NullFrameOnEmpty(enable);
    }
    void ClearFrames() {
        if (camera_frame_queue_) {
            camera_frame_queue_->ClearQueueFrames();
        }
    }

    int SetStatusCallback(WebcamStatusCallback* status_callback);

private:

    uvc_device_handle_t* uvc_device_handle_;

    volatile bool is_running_;

    int request_width_, request_height_;
    CameraFormat request_mode_;
    int request_min_fps_, request_max_fps_;

    bool frame_format_configured;
    int input_frame_width_, input_frame_height_;
    CameraFormat input_frame_mode_;

    int output_frame_width_, output_frame_height_;
    CameraFormat output_frame_mode_;

    size_t frame_bytes_;

    size_t num_fifo_enqueue_attempts_;
    uint64_t num_fifo_enqueue_overflows_;

    int drop_frame_count_;
    int frame_drops_;

    WebcamStatusCallback* status_callback_;

    // Frame callbacks accumulated in fixe size Q. Overruns are dropped.
    frame_queue_webcam<CameraFrame, MAX_FRAMES, FRAME_POOL_SZ>* camera_frame_queue_;

    // uvc callback delivers frame
    static void FrameReceivedCallback(uvc_frame_t *frame, void *vptr_args);

    static void* ThreadFunc(void *vptr_args);

    int Prepare(uvc_stream_ctrl_t *ctrl);

    void StartFrameAccumulation(uvc_stream_ctrl_t *ctrl);

    // client synch
    pthread_t preview_thread_;
    void WaitForRelease(void);
    int CondSignalRelease(void);
    pthread_mutex_t release_mutex_;
    pthread_cond_t release_sync_;
    bool is_client_registered_;

    // thread sync
    pthread_mutex_t thread_termination_mutex_;
    pthread_cond_t thread_termination_sync_;
};


class WebcamClientIfc : public IFrameAccessRegistration, public ICameraFrameAccess {

public:

    WebcamClientIfc(ICameraFrameAccess* camera_client);
    ~WebcamClientIfc();

    // ICameraFrameAccess methods
    CameraError GetFrameDescription(CameraFormat* camera_format, int* height, int* width) override;
    CameraFrame* GetFrame() override;
    void ReleaseFrame(CameraFrame* camera_frame) override;
    void UnregisterClient(void) override;

    // IFrameAccessRegistration methods
    FrameAccessSharedPtr RegisterClient(void) override;

private:

    ICameraFrameAccess* camera_frame_access_;
};

#endif // WEBCAM_FIFO_H
