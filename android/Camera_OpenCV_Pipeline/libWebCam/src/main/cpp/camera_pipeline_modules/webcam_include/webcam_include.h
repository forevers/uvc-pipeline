#ifndef WEBCAM_INCLUDE_H
#define WEBCAM_INCLUDE_H

#include <cstdlib>
#include <sys/time.h>
#include <stdint.h>
#include <utility>

#include "stddef.h"

typedef enum camera_error {
    CAMERA_SUCCESS,
    CAMERA_ERROR_TIMEOUT,
    CAMERA_ERROR_INACTIVE,
    CAMERA_ERROR_NO_MEM,
    CAMERA_ERROR_INVALID_PARAMS,
    CAMERA_ERROR_INVALID_MODE,
    CAMERA_ERROR_NOT_SUPPORTED,
    CAMERA_ERROR_OTHER,
} CameraError;

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
    CAMERA_FRAME_FORMAT_BM22 = 6,

    /** Number of formats understood */
    CAMERA_FRAME_FORMAT_COUNT,
} CameraFormat;

typedef struct camera_frame {

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
} CameraFrame;

#endif // WEBCAM_INCLUDE_H