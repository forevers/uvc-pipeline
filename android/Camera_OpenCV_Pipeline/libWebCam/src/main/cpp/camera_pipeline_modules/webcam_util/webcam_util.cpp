#include "webcam_include.h"

#include <cstdlib>
#include <cstring>
#include <util.h>

//#define #define TAG_WEBCAM_UTIL "WEBCAM_UTIL"
#define TAG_WEBCAM_UTIL "test"

CameraFrame* allocate_camera_frame(size_t data_bytes) {

    CameraFrame* camera_frame = static_cast<CameraFrame*>(new CameraFrame);

    memset(camera_frame, 0, sizeof(CameraFrame));

    if (data_bytes > 0) {
        camera_frame->actual_bytes = camera_frame->data_bytes = data_bytes;
        camera_frame->data = new uint8_t[data_bytes];

        if (!camera_frame->data) {
            free(camera_frame->data);
            return NULL ;
        }
    }

    return camera_frame;
}

void free_camera_frame(CameraFrame* camera_frame) {

    if (camera_frame->data_bytes > 0)
        free(camera_frame->data);

    free(camera_frame);
}

/**
 * returns bytes per pixel for a given frame format
 */
int get_bytes_per_pixel(CameraFormat camera_format) {

    switch (camera_format) {
        case CAMERA_FRAME_FORMAT_YUYV:
            return 2;
        case CAMERA_FRAME_FORMAT_BM22:
        case CAMERA_FRAME_FORMAT_RGBX:
            return 4;
        case CAMERA_FRAME_FORMAT_GRAY_8:
            return 1;
        case CAMERA_FRAME_FORMAT_GRAY_16:
            return 2;
        case CAMERA_FRAME_FORMAT_UNKNOWN:
        // TODO - CAMERA_FRAME_FORMAT_MJPEG ?
        case CAMERA_FRAME_FORMAT_MJPEG:
        default:
            return 4;
    }
}


// TODO the uvc library may have an increasing data_bytes field over time ... why is this?

/**
 * increases size of camera_frame data field if uvc library return increasing sizes after initial enumeration
 */
// TODO review this resizing
CameraError ensure_frame_size(CameraFrame* camera_frame, size_t need_bytes) {

    if (!camera_frame->data || camera_frame->data_bytes != need_bytes) {
        LOGE_(TAG_WEBCAM_UTIL, "change camera_frame->data bytes from %zu to %zu", camera_frame->data_bytes, need_bytes);
        camera_frame->actual_bytes = camera_frame->data_bytes = need_bytes;
        camera_frame->data = static_cast<uint8_t*>(realloc(camera_frame->data, camera_frame->data_bytes));
    }

    if (!camera_frame->data || !need_bytes)
        return CAMERA_ERROR_NO_MEM;

    return CAMERA_SUCCESS;
}
