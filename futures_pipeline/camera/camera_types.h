#pragma once

#include "videodev2.h"

#include <cstdint>
#include <cstring>

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

// class CameraFrame {
struct CameraFrame {
// public:
    uint32_t reuse_token;
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
};

// ----- from android project