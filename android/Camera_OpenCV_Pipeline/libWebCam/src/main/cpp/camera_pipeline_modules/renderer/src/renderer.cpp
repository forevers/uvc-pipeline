#include <util.h>
#include <cstdlib>
#include <cstring>
#include <linux/resource.h>
#include <sys/resource.h>

#include "webcam.h"
#include "renderer.h"

#include "webcam_util.h"

//#define RENDERER_TAG "RENDERER"
#define RENDERER_TAG "test"

#define PIXELS_PER_BYTE_RGBA 4


Renderer::Renderer(IFrameAccessRegistration* camera_frame_access, int width, int height) :
        window_(nullptr),
        request_width_(width),
        request_height_(height),
        render_format_(WINDOW_FORMAT_RGBA_8888),
        is_running_(false)
{
    ENTER_(RENDERER_TAG);

    // obtain upstream interface
    if (nullptr == (frame_access_shared_ptr_ = camera_frame_access->RegisterClient())) {
        LOGE_(RENDERER_TAG, "invalid sensor_channel request");
    }

    LOGE_(RENDERER_TAG, "Renderer use_count: %ld", frame_access_shared_ptr_.use_count());
    LOGI_(RENDERER_TAG, "width : %d, height : %d", width, height);

    pthread_cond_init(&surface_sync_, nullptr);
    pthread_mutex_init(&surface_mutex_, nullptr);

    EXIT_(RENDERER_TAG);
}


Renderer::~Renderer() {

    ENTER_(RENDERER_TAG);

    if (window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }

    pthread_cond_destroy(&surface_sync_);
    pthread_mutex_destroy(&surface_mutex_);

    EXIT_(RENDERER_TAG);
}


inline const bool Renderer::IsRunning() const {
    return is_running_;
}


int Renderer::SetSurface(ANativeWindow* window) {
    ENTER_(RENDERER_TAG);

    LockSurfaceMutex();
    if (window_) {
        ANativeWindow_release(window_);
    }
    window_ = window;
    if (window_) {
        ANativeWindow_setBuffersGeometry(window_, request_width_, request_height_, WINDOW_FORMAT_RGBA_8888);
    }
    UnlockSurfaceMutex();

    RETURN_(RENDERER_TAG, 0, int);
}


void Renderer::ClearSurface() {
    ENTER_(RENDERER_TAG);

    ANativeWindow_Buffer buffer;

    // clear the surface
    LockSurfaceMutex();
    if (window_) {
        if (ANativeWindow_lock(window_, &buffer, nullptr) == 0) {
            uint8_t *dest = (uint8_t *)buffer.bits;
            const size_t bytes = buffer.width * PIXELS_PER_BYTE_RGBA;
            const int stride = buffer.stride * PIXELS_PER_BYTE_RGBA;
            for (int i = 0; i < buffer.height; i++) {
                memset(dest, 0, bytes);
                dest += stride;
            }
            ANativeWindow_unlockAndPost(window_);
        }
    }
    UnlockSurfaceMutex();

    EXIT_(RENDERER_TAG);
}


int Renderer::Start() {

    ENTER_(RENDERER_TAG);

    int result = EXIT_FAILURE;

    if (!IsRunning()) {

        is_running_ = true;

        LockSurfaceMutex();
        result = pthread_create(&thread_, nullptr, ThreadImpl, (void *) this);
        UnlockSurfaceMutex();

        if (result != EXIT_SUCCESS) {
            LOGE_(RENDERER_TAG, "pthread_create() failure : %d", result);
            is_running_ = false;
        }
    }

    RETURN_(RENDERER_TAG, result, int);
}


int Renderer::Stop() {
    ENTER_(RENDERER_TAG);

    bool b = IsRunning();
    if (b) {
        is_running_ = false;

        pthread_mutex_lock(&surface_mutex_);
        pthread_cond_signal(&surface_sync_);
        pthread_mutex_unlock(&surface_mutex_);

        if (pthread_join(thread_, nullptr) != EXIT_SUCCESS) {
            LOGW_(RENDERER_TAG, "Renderer pthread_join() failure");
        }
    } else {
        LOGI_(RENDERER_TAG, "StopRender() call when not running");
    }

    ClearSurface();

    LockSurfaceMutex();
    if (window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
        LOGI_(RENDERER_TAG, "ANativeWindow_release()");
    }
    UnlockSurfaceMutex();

    RETURN_(RENDERER_TAG, CAMERA_SUCCESS, int);
}


void* Renderer::ThreadImpl(void* vptr_args) {
    ENTER_(RENDERER_TAG);

    int result;

    // increase thread priority
    int prio = getpriority(PRIO_PROCESS, 0);
    nice(10);
    if (getpriority(PRIO_PROCESS, 0) < prio) {
        LOGW_(TAG_WEBCAM_FIFO, "could not change thread priority");
    }

    Renderer* render = reinterpret_cast<Renderer*>(vptr_args);
    if (render) {
        result = render->Prepare();
        if (!result) {
            render->Run();
        }
    }

    PRE_EXIT_(RENDERER_TAG);
    pthread_exit(nullptr);
}


CameraError Renderer::Prepare() {
    ENTER_(RENDERER_TAG);

    auto result = CAMERA_SUCCESS;

    RETURN_(RENDERER_TAG, result, CameraError);
}


void Renderer::Run(void) {

    ENTER_(RENDERER_TAG);

    CameraFrame* frame = nullptr;

    for ( ; IsRunning() ; ) {

        if (nullptr != (frame = frame_access_shared_ptr_->GetFrame())) {
            Render(frame);
            frame_access_shared_ptr_->ReleaseFrame(frame);
        } else {
            LOGI_(RENDERER_TAG, "null frame received");
            // exit thread
            is_running_ = false;
        }
    }

    ClearSurface();

    LOGI_(RENDERER_TAG, "Renderer use_count: %ld", frame_access_shared_ptr_.use_count());

    // signal release of interface to upstream element
    frame_access_shared_ptr_ = nullptr;

    LOGI_(RENDERER_TAG, "thread exit");

    EXIT_(RENDERER_TAG);
}


void Renderer::Render(CameraFrame* camera_frame) {

    LockSurfaceMutex();

    ANativeWindow_Buffer buffer;
    if (window_ && (ANativeWindow_lock(window_, &buffer, nullptr) == 0)) {

        if (camera_frame && camera_frame->height == buffer.height && camera_frame->width == buffer.width) {

            auto bytes_per_pixel = 0;

            switch (camera_frame->frame_format) {

                case CAMERA_FRAME_FORMAT_RGBX:
                case CAMERA_FRAME_FORMAT_GRAY_8:
                case CAMERA_FRAME_FORMAT_GRAY_16:
                    bytes_per_pixel = get_bytes_per_pixel(camera_frame->frame_format);
                    break;

                default:
                    break;
            }

            if (bytes_per_pixel) {

                switch (camera_frame->frame_format) {

                    case CAMERA_FRAME_FORMAT_RGBX:
                        // direct copy of rgba into window surface
                        if (buffer.stride == buffer.width) {
                            memcpy(buffer.bits, camera_frame->data, camera_frame->data_bytes);
                        } else {
                            uint8_t* raw = (uint8_t*) (camera_frame->data);
                            uint8_t* rgba = (uint8_t*) buffer.bits;
                            int bytes_per_row = camera_frame->width*bytes_per_pixel;
                            for (int row = 0; row < camera_frame->height; row++) {
                                memcpy(rgba, &raw[row * bytes_per_row], bytes_per_row);
                                rgba +=  (buffer.stride * bytes_per_pixel);
                            }
                        }
                        break;

                    case CAMERA_FRAME_FORMAT_GRAY_8:
                    case CAMERA_FRAME_FORMAT_GRAY_16: {
                        uint8_t* raw = (uint8_t*) (camera_frame->data);

                        uint8_t* rgb = (uint8_t*) buffer.bits;
                        uint8_t* rgb_end = rgb + (buffer.height * buffer.width - 1) * PIXELS_PER_BYTE_RGBA;

                        // render smaller width/height region
                        auto width = camera_frame->width < buffer.width ? camera_frame->width : buffer.width;
                        // use lower height
                        auto height = camera_frame->height < buffer.height ? camera_frame->height : buffer.height;

                        uint8_t* surface_line_start = (uint8_t*) buffer.bits;
                        auto surface_line_stride = buffer.stride * PIXELS_PER_BYTE_RGBA;

                        for (int row = 0; row < height; row++) {

                            uint8_t* surface_fill_addr = surface_line_start;

                            for (int col = 0; col < width; col++) {

                                uint16_t shifted_word;
                                // TODO implement range select control in render interface
                                uint8_t shifted_val;

                                if (bytes_per_pixel == 2) {

                                    shifted_word = raw[0] | (raw[1] << 8);
                                    shifted_val = shifted_word >> 0;
                                } else {
                                    shifted_val = raw[0];
                                }

                                surface_fill_addr[0] = shifted_val;
                                surface_fill_addr[1] = shifted_val;
                                surface_fill_addr[2] = shifted_val;
                                surface_fill_addr[3] = 0xff;

                                surface_fill_addr += 4;
                                raw += bytes_per_pixel;
                            }

                            // advance render address using stride
                            surface_line_start += surface_line_stride;
                        }
                    }
                        break;

                    default:
                        break;
                }
            }

        } else {
            LOGE_(RENDERER_TAG, "render surface size mismatch");
        }

        ANativeWindow_unlockAndPost(window_);
    }

    UnlockSurfaceMutex();
}
