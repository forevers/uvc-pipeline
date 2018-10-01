#include "webcam.h"

#include "webcam_include.h"
#include "libuvc/libuvc_internal.h"
#include "rapidjson/prettywriter.h"

#include <ctype.h>

using namespace rapidjson;

//#define WEBCAM_TAG "WEBCAM"
#define WEBCAM_TAG "test"


WebCam::WebCam()
        : uvc_fd_(0),
          usb_fs_(nullptr),
          camera_context_(nullptr),
          camera_device_(nullptr),
          camera_device_handle_(nullptr),
          webcam_fifo_(nullptr),
          status_callback_(nullptr)
{
    ENTER_(CAMERA_TAG);

    EXIT_(CAMERA_TAG);
}

WebCam::~WebCam() {
    ENTER_(WEBCAM_TAG);

    Stop();

    if (camera_context_) {
        uvc_exit(camera_context_);
        camera_context_ = nullptr;
    }
    if (usb_fs_) {
        free(usb_fs_);
        usb_fs_ = nullptr;
    }

    if (status_callback_) {
        delete status_callback_;
    }

    EXIT_(WEBCAM_TAG);
}

int WebCam::Prepare(int vid, int pid, int fd, int busnum, int devaddr, const char* usbfs) {
    ENTER_(WEBCAM_TAG);

    uvc_error_t result = UVC_ERROR_BUSY;
    if (!camera_device_handle_ && fd) {
        if (usb_fs_) {
            free(usb_fs_);
        }
        usb_fs_ = strdup(usbfs);
        if (!camera_context_) {
            result = uvc_init2(&camera_context_, nullptr, usb_fs_);
            if (result < 0) {
                LOGE("failed to init libuvc");
                RETURN_(WEBCAM_TAG, result, int);
            }
        }
        fd = dup(fd);
        result = uvc_get_device_with_fd(camera_context_, &camera_device_, vid, pid, nullptr, fd, busnum, devaddr);
        if (!result) {
            LOGE_(WEBCAM_TAG, "uvc_get_device_with_fd() success");
            result = uvc_open(camera_device_, &camera_device_handle_);
            if (!result) {
                LOGE("uvc_open() success");
                uvc_fd_ = fd;
                status_callback_ = new WebcamStatusCallback();
                webcam_fifo_ = new WebcamFifo(camera_device_handle_);
            } else {
                LOGE("uvc_open() failure : err = %d", result);
                uvc_unref_device(camera_device_);
                camera_device_ = nullptr;
                camera_device_handle_ = nullptr;
                close(fd);
            }
        } else {
            LOGE_(WEBCAM_TAG, "uvc_get_device_with_fd() failure : err = %d", result);
            close(fd);
        }
    } else {
        LOGW_(WEBCAM_TAG, "camera is already opened. you should release first");
    }
    RETURN_(CAMERA_TAG, result, int);
}

int WebCam::SetStatusCallback(JNIEnv* env, jobject status_callback_obj) {
    ENTER_(WEBCAM_TAG);

    int result = EXIT_FAILURE;

    if (status_callback_) {
        // allow only one callback client
        if (EXIT_SUCCESS == (result = status_callback_->SetCallback(env, status_callback_obj))) {
            result = webcam_fifo_->SetStatusCallback(status_callback_);
        }
    }

    RETURN_(WEBCAM_TAG, result, int);
}

char* WebCam::GetSupportedVideoModes() {
    ENTER_(WEBCAM_TAG);

    char buf[1024];

    if (camera_device_handle_) {

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);

        writer.StartObject();
        {
            if (camera_device_handle_->info->stream_ifs) {
                uvc_streaming_interface_t* stream_if;
                int stream_idx = 0;

                writer.String("formats");
                writer.StartArray();
                DL_FOREACH(camera_device_handle_->info->stream_ifs, stream_if)
                {
                    ++stream_idx;
                    uvc_format_desc_t* fmt_desc;
                    uvc_frame_desc_t* frame_desc;
                    DL_FOREACH(stream_if->format_descs, fmt_desc)
                    {
                        writer.StartObject();
                        {
                            switch (fmt_desc->bDescriptorSubtype) {
                                case UVC_VS_FORMAT_UNCOMPRESSED:
                                case UVC_VS_FORMAT_MJPEG:
                                    // only ascii fourcc codes currently accepted
                                    if ( isprint(fmt_desc->fourccFormat[0]) && isprint(fmt_desc->fourccFormat[1]) &&
                                         isprint(fmt_desc->fourccFormat[2]) && isprint(fmt_desc->fourccFormat[3]) ) {

                                        writer.String("index");
                                        writer.Uint(fmt_desc->bFormatIndex);
                                        writer.String("type");
                                        writer.Int(fmt_desc->bDescriptorSubtype);
                                        writer.String("default");
                                        writer.Uint(fmt_desc->bDefaultFrameIndex);
                                        writer.String("fourcc");
                                        buf[0] = fmt_desc->fourccFormat[0];
                                        buf[1] = fmt_desc->fourccFormat[1];
                                        buf[2] = fmt_desc->fourccFormat[2];
                                        buf[3] = fmt_desc->fourccFormat[3];
                                        buf[4] = '\0';
                                        writer.String(buf);
                                        writer.String("sizes");
                                        writer.StartArray();
                                        DL_FOREACH(fmt_desc->frame_descs, frame_desc) {
                                            writer.StartObject();
                                            writer.Key("resolution");
                                            snprintf(buf, sizeof(buf), "%dx%d",
                                                     frame_desc->wWidth, frame_desc->wHeight);
                                            buf[sizeof(buf) - 1] = '\0';
                                            writer.String(buf);
                                            writer.Key("default_frame_interval");
                                            writer.Uint(frame_desc->dwDefaultFrameInterval);
                                            writer.Key("interval_type");
                                            writer.Uint(frame_desc->bFrameIntervalType);
                                            writer.Key("interval_type_array");
                                            writer.StartArray();
                                            if (frame_desc->bFrameIntervalType) {
                                                for (auto i = 0; i < frame_desc->bFrameIntervalType; i++) {
                                                    writer.Uint((frame_desc->intervals)[i]);
                                                }
                                            }
                                            writer.EndArray();
                                            writer.EndObject();
                                        }
                                        writer.EndArray();
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }
                        writer.EndObject();
                    }
                }
                writer.EndArray();
            }
        }
        writer.EndObject();
        return strdup(buffer.GetString());

    } else {
        return nullptr;
    }

}

int WebCam::SetPreviewSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_format) {
    ENTER_(WEBCAM_TAG);

    int result = EXIT_FAILURE;
    if (webcam_fifo_) {
        result = webcam_fifo_->SetFrameSize(width, height, min_fps, max_fps, camera_format);
    }

    RETURN_(WEBCAM_TAG, result, int);
}

IFrameAccessRegistration* WebCam::GetFrameAccessIfc(int interface_number) {
    ENTER_(WEBCAM_TAG);

    IFrameAccessRegistration* camera_frame_access_registration = webcam_fifo_;

    PRE_EXIT_(WEBCAM_TAG);
    return camera_frame_access_registration;
}

int WebCam::Start() {
    ENTER_(WEBCAM_TAG);

    int result = EXIT_FAILURE;
    if (camera_device_handle_) {
        return webcam_fifo_->StartFrameAcquisition();
    }
    RETURN_(WEBCAM_TAG, result, int);
}

int WebCam::Stop() {
    ENTER_(WEBCAM_TAG);

    if (camera_device_handle_) {
        webcam_fifo_->StopFrameAcquisition();

        SAFE_DELETE(webcam_fifo_);
        uvc_close(camera_device_handle_);
        camera_device_handle_ = nullptr;
    }

    LOGI_(WEBCAM_TAG, "camera_device_");

    if (camera_device_) {
        uvc_unref_device(camera_device_);
        camera_device_ = nullptr;
    }

    LOGI_(WEBCAM_TAG, "usb_fs_");

    if (usb_fs_) {
        close(uvc_fd_);
        uvc_fd_ = 0;
        free(usb_fs_);
        usb_fs_ = nullptr;
    }

    RETURN_(WEBCAM_TAG, 0, int);
}
