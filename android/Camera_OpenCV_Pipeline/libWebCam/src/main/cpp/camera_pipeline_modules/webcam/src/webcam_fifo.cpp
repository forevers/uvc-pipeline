#include <linux/resource.h>
#include <sys/resource.h>
#include "webcam_fifo.h"

#include "webcam_frame_access_impl.h"
#include "webcam_include.h"
#include "webcam_util.h"

#define NUM_PREVIEW_FRAMES  1

// RGBA
#define PIXELS_PER_BYTE_RGBA (4*NUM_PREVIEW_FRAMES)


uvc_frame_format requestMode_to_uvc_frame_format(CameraFormat camera_format) {

    switch (camera_format) {
        case CAMERA_FRAME_FORMAT_YUYV:
            return UVC_FRAME_FORMAT_YUYV;
        case CAMERA_FRAME_FORMAT_MJPEG:
            return UVC_FRAME_FORMAT_MJPEG;
        default:
            return UVC_FRAME_FORMAT_YUYV;
    }
}


WebcamFifo::WebcamFifo(uvc_device_handle_t* uvc_device_handle)
        : uvc_device_handle_(uvc_device_handle),
          status_callback_(nullptr),
          is_client_registered_(false),
          input_frame_width_(0),
          input_frame_height_(0),
          output_frame_width_(0),
          output_frame_height_(0),
          frame_bytes_(0)
{
    uvc_device_handle_ = uvc_device_handle;
    request_width_ = 0;
    request_height_ = 0;
    request_min_fps_ = DEFAULT_UVC_PREVIEW_FPS_MIN;
    request_max_fps_ = DEFAULT_UVC_PREVIEW_FPS_MAX;
    request_mode_ = CAMERA_FRAME_FORMAT_UNKNOWN;
    frame_format_configured = false;
    is_running_ = false;

    num_fifo_enqueue_attempts_ = 0;
    num_fifo_enqueue_overflows_ = 0;

    // initialize with single frame dropping state
    frame_drops_ = drop_frame_count_ = 0;

    camera_frame_queue_ = new frame_queue_webcam<CameraFrame, MAX_FRAMES, FRAME_POOL_SZ>;

    pthread_cond_init(&release_sync_, nullptr);
    pthread_mutex_init(&release_mutex_, nullptr);

    pthread_cond_init(&thread_termination_sync_, nullptr);
    pthread_mutex_init(&thread_termination_mutex_, nullptr);
}


WebcamFifo::~WebcamFifo() {
    ENTER_(TAG_WEBCAM_FIFO);

    DeleteFrameQueue();

    pthread_cond_destroy(&release_sync_);
    pthread_mutex_destroy(&release_mutex_);

    pthread_cond_destroy(&thread_termination_sync_);
    pthread_mutex_destroy(&thread_termination_mutex_);

    EXIT_(TAG_WEBCAM_FIFO);
}


inline const bool WebcamFifo::IsRunning() const {
    return is_running_;
}


int WebcamFifo::SetFrameSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_format) {
    ENTER_(TAG_WEBCAM_FIFO);

    int result = 0;
    if ((request_width_ != width) || (request_height_ != height) || (request_mode_ != camera_format)) {

        uvc_stream_ctrl_t ctrl;
        LOGI_(TAG_WEBCAM_FIFO, "request_width_ : %d, request_height_ : %d, request_min_fps_ : %d, request_max_fps_ : %d", width, height, min_fps, max_fps);
        // libuvc uses fps 0 to mean "first avail" and fps to mean exact match
        if ( UVC_SUCCESS == (result = uvc_get_stream_ctrl_format_size_fps(uvc_device_handle_, &ctrl,
                                                     requestMode_to_uvc_frame_format(camera_format),
                                                     width, height, min_fps, max_fps))) {

            frame_format_configured = true;

            input_frame_width_ = request_width_ = width;
            input_frame_height_ = request_height_ = height;
            request_min_fps_ = min_fps;
            request_max_fps_ = max_fps;
            input_frame_mode_ = request_mode_ = camera_format;
            if (request_width_ == 240 && request_height_ == 720) {
                // hijack this YUYV mode as a BM22 mode
                output_frame_mode_ = CAMERA_FRAME_FORMAT_BM22;
                output_frame_width_ = 120;
                output_frame_height_ = 720;
            } else {
                output_frame_mode_ = input_frame_mode_;
                output_frame_width_ = input_frame_width_;
                output_frame_height_ = input_frame_height_;
            }

            LOGI_(TAG_WEBCAM_FIFO, "input_frame_mode_: %d", input_frame_mode_);
        }
    }

    RETURN_(TAG_WEBCAM_FIFO, result, int);
}


int WebcamFifo::StartFrameAcquisition() {
    ENTER_(TAG_WEBCAM_FIFO);

    int result = EXIT_FAILURE;
    if (!IsRunning()) {
        is_running_ = true;

        LOGD_(TAG_WEBCAM_FIFO, "start uvc acq thread");

        if (uvc_device_handle_ != nullptr) {
            result = pthread_create(&preview_thread_, nullptr, ThreadFunc, (void *) this);
        } else {
            LOGE_(TAG_WEBCAM_FIFO, "start() invalid device handle");
        }

        if (result != EXIT_SUCCESS) {
            LOGE_(TAG_WEBCAM_FIFO, "camera already running or pthread_create failure");
            is_running_ = false;
            CondSignalQueue();
        }
    }

    RETURN_(TAG_WEBCAM_FIFO, result, int);
}


int WebcamFifo::StopFrameAcquisition() {
    ENTER_(TAG_WEBCAM_FIFO);

    bool b = IsRunning();
    if (b) {

        uvc_stop_streaming(uvc_device_handle_);
        LOGE_(TAG_WEBCAM_FIFO, "uvc_stop_streaming()");

        // handshake with clients for interface termination
        CondSignalRelease();

        // terminate and join thread
        pthread_mutex_lock(&thread_termination_mutex_);
        pthread_cond_signal(&thread_termination_sync_);
        pthread_mutex_unlock(&thread_termination_mutex_);

        // terminate thread
        if (pthread_join(preview_thread_, nullptr) != EXIT_SUCCESS) {
            LOGW_(TAG_WEBCAM_FIFO, "pthread_join() failure");
        }

    } else {
        LOGE_(TAG_WEBCAM_FIFO, "uvc_stop_streaming() called when not running");
    }
    ClearFrames();

    LOGI_(TAG_WEBCAM_FIFO, "thread exit");

    RETURN_(TAG_WEBCAM_FIFO, 0, int);
}


int WebcamFifo::CondSignalRelease() {
    ENTER_(TAG_WEBCAM_FIFO);

    // flush queue
    camera_frame_queue_->ClearQueueFrames();

    LOGI_(TAG_WEBCAM_FIFO, "broadcast null frame availability to webcam clients");

    // allow queue to return null frames to clients
    NullFrameOnEmpty(true);

    // issue null frame to clients
    CondSignalQueue();

    // wait for client unregistration signal
    pthread_mutex_lock(&release_mutex_);
    while (is_client_registered_) {
        pthread_cond_wait(&release_sync_, &release_mutex_);
    }
    pthread_mutex_unlock(&release_mutex_);

    LOGI_(TAG_WEBCAM_FIFO, "client has unregistered");

    LOGI_(TAG_WEBCAM_FIFO, "CondSignalRelease() exit");

    RETURN_(TAG_WEBCAM_FIFO, 0, int);
}


CameraError copy_uvc_to_camera_frame(uvc_frame_t *in, CameraFrame *out) {

    CameraError ret = CAMERA_SUCCESS;

    if (CAMERA_SUCCESS != (ret = ensure_frame_size(out, in->actual_bytes))) {

        LOGE_(TAG_WEBCAM_FIFO, "ensure_frame_size() failure");
        LOGE_(TAG_WEBCAM_FIFO, "out bytes : %zu, in bytes : %zu", out->actual_bytes, in->actual_bytes);

        return CAMERA_ERROR_NO_MEM;
    }

    out->width = in->width;
    out->height = in->height;
    switch (in->frame_format) {
        /** supported webcam formats */
        case UVC_FRAME_FORMAT_YUYV:
            if (in->width == 240 && in->height == 720) {
                // hijack this YUYV mode as a BM22 mode
                out->frame_format = CAMERA_FRAME_FORMAT_BM22;
                out->step = in->step;
                out->sequence = in->sequence;
                out->capture_time = in->capture_time;
                out->width = in->width / 2;
                out->height = in->height;
                out->data_bytes = out->actual_bytes= in->actual_bytes;
                memcpy(out->data, in->data, out->actual_bytes);
                return ret;
            } else {
                out->frame_format = CAMERA_FRAME_FORMAT_YUYV;
            }
            break;
        case UVC_FRAME_FORMAT_GRAY8:
            out->frame_format = CAMERA_FRAME_FORMAT_GRAY_8;
            break;
        case UVC_FRAME_FORMAT_MJPEG:
            out->frame_format = CAMERA_FRAME_FORMAT_RGBX;
            break;
        case UVC_FRAME_FORMAT_UNKNOWN:
            out->frame_format = CAMERA_FRAME_FORMAT_UNKNOWN;
        default:
            LOGE_(TAG_WEBCAM_FIFO, "in->frame_format not supported");
            ret = CAMERA_ERROR_INVALID_MODE;
            break;
    }

    if (ret == CAMERA_SUCCESS) {

        out->step = in->step;
        out->sequence = in->sequence;
        out->capture_time = in->capture_time;
        out->width = in->width;
        out->height = in->height;
        out->data_bytes = out->actual_bytes = in->actual_bytes;
        memcpy(out->data, in->data, in->actual_bytes);

    } else {
        LOGE_(TAG_WEBCAM_FIFO, "invalid in->frame_format");
    }

    return ret;
}


void WebcamFifo::FrameReceivedCallback(uvc_frame_t *frame, void *vptr_args) {

    WebcamFifo* frame_fifo = reinterpret_cast<WebcamFifo *>(vptr_args);

    if UNLIKELY(!frame_fifo->IsRunning() || !frame || !frame->frame_format || !frame->data || !frame->data_bytes) return;

    if ( (frame->frame_format != UVC_FRAME_FORMAT_YUYV) && (frame->frame_format != UVC_FRAME_FORMAT_MJPEG) ) {
        LOGE_(TAG_WEBCAM_FIFO, "invalid frame_format");
        return;
    } else if (frame->actual_bytes != frame_fifo->frame_bytes_) {
        LOGE_(TAG_WEBCAM_FIFO, "frame->actual_bytes : %zu, frame->data_bytes : %zu", frame->actual_bytes, frame_fifo->frame_bytes_);
        return;
    } else if (frame->width != frame_fifo->input_frame_width_) {
        LOGE_(TAG_WEBCAM_FIFO, "frame->width : %zu, frame_fifo->input_frame_width_ : %zu", frame->width, frame_fifo->input_frame_width_);
        return;
    } else if (frame->height != frame_fifo->input_frame_height_) {
        LOGE_(TAG_WEBCAM_FIFO, "frame->height : %zu, frame_fifo->input_frame_height_ : %zu", frame->height, frame_fifo->input_frame_height_);
        return;
    }

    if (LIKELY(frame_fifo->IsRunning())) {

        // TODO - review frame dropping scheme
        if (frame_fifo->frame_drops_ && frame_fifo->drop_frame_count_-- != 0) {

            LOGE_(TAG_WEBCAM_FIFO, "drop");
            // drop this frame, frame rate halfing up to subsampling 1 of 8
            return;

        } else {

            // reset frame drop count
            frame_fifo->drop_frame_count_ = frame_fifo->frame_drops_;

            // TODO yuv and mjpeg processed here ? into target buffer (rgbx, rgray8, ...)
            // ... does the size dynamic bounce around for yuv or mjpeg still ?
            // TODO note parameter not used in method ?
            auto num_bytes = (frame->width == 240 && frame->height == 720) ? (2*frame->data_bytes) : frame->data_bytes;
            CameraFrame *copy = frame_fifo->camera_frame_queue_->GetFrameFromPool(num_bytes);

            if (copy != nullptr) {

                if (CAMERA_SUCCESS == copy_uvc_to_camera_frame(frame, copy)) {
                    if (0 != frame_fifo->camera_frame_queue_->AddFrameToQueue(copy, frame_fifo->IsRunning())) {
                        LOGE_(TAG_WEBCAM_FIFO, "AddFrameToQueue() failure");
                    } else {
                    }
                } else {
                    LOGE_(TAG_WEBCAM_FIFO, "copy_uvc_to_camera_frame() failure");
                    frame_fifo->camera_frame_queue_->RecycleFrameIntoPool(copy);
                    return;
                }
            }
#if 0
            // TODO review need/benefit for dropping frames in such a manner
            // TODO consider selecting a lower bps video mode automatically
            // quick recycle test
            LOGE_(TAG_WEBCAM_FIFO, "frame->data_bytes  %d", frame->data_bytes);

            frame_fifo->camera_frame_queue_->RecycleFrameIntoPool(copy);
            return;

            static size_t processed_count = 0;
            static size_t dropped_count = 0;

            if (++processed_count == std::numeric_limits<std::size_t>::max()) {
                processed_count = 0;
                dropped_count = 0;
            }

            if (CAMERA_SUCCESS != copy_uvc_to_camera_frame(frame, copy))
            {
                LOGE_(TAG_WEBCAM_FIFO, "copy_uvc_to_camera_frame() failure");
                frame_fifo->camera_frame_queue_->RecycleFrameIntoPool(copy);
                return;
            }

            frame_fifo->num_fifo_enqueue_attempts_++;
            if (0 != frame_fifo->camera_frame_queue_->AddFrameToQueue(copy, frame_fifo->IsRunning())) {
                LOGE_(TAG_WEBCAM_FIFO, "Qd");
                frame_fifo->num_fifo_enqueue_overflows_++;

                dropped_count++;
                float dropped_percentage = 100 * static_cast<float>(dropped_count) / (processed_count + dropped_count);
                if (processed_count > 200) {
                    LOGE_(TAG_WEBCAM_FIFO, "d : %zu, p : %zu, %%%f", dropped_count, processed_count, dropped_percentage);
                    if (frame_fifo->frame_drops_ != 7 && dropped_percentage > 1.) {
                        switch (frame_fifo->frame_drops_) {
                            case 0:
                                frame_fifo->frame_drops_ = frame_fifo->drop_frame_count_ = 1;
                                break;
                            case 1:
                                frame_fifo->frame_drops_ = frame_fifo->drop_frame_count_ = 3;
                                break;
                            case 3:
                                frame_fifo->frame_drops_ = frame_fifo->drop_frame_count_ = 7;
                                break;
                            default:
                                break;
                        }
                        LOGE_(TAG_WEBCAM_FIFO, "drop %d of %d frames", frame_fifo->frame_drops_, frame_fifo->frame_drops_ + 1);
                        processed_count = dropped_count = 0;

                        if (frame_fifo->status_callback_) {
                            frame_fifo->status_callback_->IssueSchemaErrorRxFifoOverflow(frame_fifo->num_fifo_enqueue_overflows_, dropped_percentage);
                            frame_fifo->status_callback_->IssueSchemaDropFrame(frame_fifo->frame_drops_, frame_fifo->frame_drops_ + 1);
                        }

                    } else { // if (!(frame_fifo->num_fifo_enqueue_overflows_ % 2)) {

                        // TODO review this thread sync
//                        if (frame_fifo->status_callback_) {
//                            frame_fifo->status_callback_->IssueSchemaErrorRxFifoOverflow(
//                                    frame_fifo->num_fifo_enqueue_overflows_,
//                                    double(frame_fifo->num_fifo_enqueue_overflows_) /
//                                    frame_fifo->num_fifo_enqueue_attempts_);
//                        }
                    }
                }

            } else if (!(processed_count % 0x3F)) {

//                // current stats
//                if (frame_fifo->status_callback_) {
//                    frame_fifo->status_callback_->IssueSchemaDropFrame(
//                            frame_fifo->num_fifo_enqueue_overflows_, frame_fifo->num_fifo_enqueue_attempts_);
//                }
            }
#endif
        }
    } else {
        LOGE_(TAG_WEBCAM_FIFO, "!frame_fifo->IsRunning()");
    }
}


void *WebcamFifo::ThreadFunc(void *vptr_args) {
    ENTER_(TAG_WEBCAM_FIFO);

    int result;

    // try to increase thread priority
    int prio = getpriority(PRIO_PROCESS, 0);
    nice(-18);
    if (getpriority(PRIO_PROCESS, 0) < prio) {
        LOGW_(TAG_WEBCAM_FIFO, "could not change thread priority");
    }

    WebcamFifo* camera_fifo = reinterpret_cast<WebcamFifo *>(vptr_args);
    if (camera_fifo) {
        uvc_stream_ctrl_t ctrl;
        result = camera_fifo->Prepare(&ctrl);
        if (!result) {
            camera_fifo->StartFrameAccumulation(&ctrl);
        }
    }

    PRE_EXIT_(TAG_WEBCAM_FIFO);
    pthread_exit(nullptr);
}


int WebcamFifo::Prepare(uvc_stream_ctrl_t* ctrl) {
    ENTER_(TAG_WEBCAM_FIFO);

    uvc_error_t result;

    // see other comment regarding libuvc api
    result = uvc_get_stream_ctrl_format_size_fps(uvc_device_handle_, ctrl,
                                                 requestMode_to_uvc_frame_format(request_mode_),
                                                 request_width_, request_height_, request_min_fps_, request_max_fps_);

    if (LIKELY(!result)) {

        LOGE_(TAG_WEBCAM_FIFO, "uvc_get_stream_ctrl_format_size_fps() success");

        uvc_frame_desc_t *frame_desc;
        result = uvc_get_frame_desc(uvc_device_handle_, ctrl, &frame_desc);
        if (LIKELY(!result)) {
            input_frame_width_ = frame_desc->wWidth;
            input_frame_height_ = frame_desc->wHeight;
            input_frame_mode_ = request_mode_;
            LOGI_(TAG_WEBCAM_FIFO, "frameSize=(%d,%d)@%d", input_frame_width_, input_frame_height_, input_frame_mode_);
        } else {
            input_frame_width_ = request_width_;
            input_frame_height_ = request_height_;
        }

        frame_bytes_ = input_frame_width_ * input_frame_height_ * get_bytes_per_pixel(input_frame_mode_);

        camera_frame_queue_->InitPool(frame_bytes_);

    } else {
        LOGE_(TAG_WEBCAM_FIFO, "uvc_get_stream_ctrl_format_size_fps() failure, err = %d", result);
    }

    RETURN_(TAG_WEBCAM_FIFO, result, int);
}


void WebcamFifo::StartFrameAccumulation(uvc_stream_ctrl_t *ctrl) {
    ENTER_(TAG_WEBCAM_FIFO);

    CameraFrame *frame = nullptr;

    uvc_error_t result = uvc_start_streaming_bandwidth(uvc_device_handle_, ctrl, FrameReceivedCallback, (void *) this, 1.0, 0);

    if (LIKELY(!result)) {

        LOGE_(result, "uvc_start_streaming_bandwidth() success");

        camera_frame_queue_->ClearQueueFrames();

        for ( ; LIKELY(IsRunning()) ; ) {

            WaitForRelease();

            if (frame) {

            } else {
                LOGE_(TAG_WEBCAM_FIFO, "release signal");
            }
        }

        LOGI_(TAG_WEBCAM_FIFO, "Streaming finished");
    } else {
        LOGE_(result, "uvc_start_streaming_bandwidth() failure : %d", result);
    }

    EXIT_(TAG_WEBCAM_FIFO);
}


void WebcamFifo::WaitForRelease(void) {
    ENTER_(TAG_WEBCAM_FIFO);

    pthread_mutex_lock(&thread_termination_mutex_);
    pthread_cond_wait(&thread_termination_sync_, &thread_termination_mutex_);
    pthread_mutex_unlock(&thread_termination_mutex_);

    EXIT_(TAG_WEBCAM_FIFO);
}


static void copyFrame(const uint8_t *src, uint8_t *dest, const int width, int height, const int stride_src, const int stride_dest) {
    const int h8 = height % 8;
    for (int i = 0; i < h8; i++) {
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
    }
    for (int i = 0; i < height; i += 8) {
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
    }
}


int WebcamFifo::SetStatusCallback(WebcamStatusCallback* status_callback) {
    ENTER_(TAG_WEBCAM_FIFO);

    int result = EXIT_FAILURE;
    if (status_callback) {
        status_callback_ = status_callback;
        result = EXIT_SUCCESS;
    } else {
        LOGE_(TAG_WEBCAM_FIFO, "status_callback == null");
    }

    RETURN_(TAG_WEBCAM_FIFO, result, int);
}


CameraError WebcamFifo::GetFrameDescription(CameraFormat* mode, int* height, int* width) {

    CameraError camera_error = CAMERA_ERROR_OTHER;

    if (frame_format_configured) {

        *mode = output_frame_mode_;
        *height = output_frame_height_;
        *width = output_frame_width_;
        camera_error = CAMERA_SUCCESS;
    } else {
        camera_error = CAMERA_ERROR_INACTIVE;
    }

    return camera_error;
}


CameraFrame* WebcamFifo::GetFrame(void) {

    CameraFrame* camera_frame = nullptr;

    if (LIKELY(IsRunning())) {
        camera_frame = camera_frame_queue_->WaitForQueueFrame(IsRunning());
    } else {
        LOGE_(TAG,"not running");
    }

    return camera_frame;
}


void WebcamFifo::ReleaseFrame(CameraFrame* camera_frame) {

    camera_frame_queue_->RecycleFrameIntoPool(camera_frame);
}


FrameAccessSharedPtr WebcamFifo::RegisterClient(void) {
    ENTER_(TAG_WEBCAM_FIFO);

    FrameAccessSharedPtr frame_access_shared_ptr(new WebcamFrameAccessIfc(this));

    pthread_mutex_lock(&release_mutex_);
    is_client_registered_ = true;
    pthread_mutex_unlock(&release_mutex_);

    PRE_EXIT_(TAG_WEBCAM_FIFO);
    return frame_access_shared_ptr;
}


void WebcamFifo::UnregisterClient(void) {
    ENTER_(TAG_WEBCAM_FIFO);

    pthread_mutex_lock(&release_mutex_);

    // signal thread of client release
    is_client_registered_ = false;
    is_running_ = false;

    // signal thread for release
    pthread_cond_signal(&release_sync_);
    pthread_mutex_unlock(&release_mutex_);

    EXIT_(TAG_WEBCAM_FIFO);
}
