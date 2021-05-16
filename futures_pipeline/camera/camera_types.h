#pragma once

#include "videodev2.h"

#include <cstdint>
#include <cstring>

// TODO move with CameraConfig
#include <string>

struct buffer
{
    unsigned int idx;
    unsigned int padding[VIDEO_MAX_PLANES];
    unsigned int size[VIDEO_MAX_PLANES];
    uint8_t *mem[VIDEO_MAX_PLANES];
};

struct V4l2FormatInfo
{
    const char *name;
    unsigned int fourcc;
    unsigned char n_planes;
};

struct Field
{
    const char *name;
    enum v4l2_field field;
};

enum BufferFillMode
{
    BUFFER_FILL_NONE = 0,
    BUFFER_FILL_FRAME = 1 << 0,
    BUFFER_FILL_PADDING = 1 << 1,
};

// TODO move to another header
struct CameraConfig
{
    CameraConfig(
        std::string camera_dev_node,
        std::string format,
        int bytes_per_pixel,
        int width_enumerated,
        int width_actual,
        int height,
        int bytes_per_row,
        int fps,
        v4l2_fract time_per_frame) :
            camera_dev_node_{camera_dev_node},
            bytes_per_pixel_{bytes_per_pixel},
            width_enumerated_{width_enumerated},
            width_actual_{width_actual},
            height_{height},
            bytes_per_row_{bytes_per_row},
            fps_{fps},
            time_per_frame_{time_per_frame} {};

    CameraConfig(const CameraConfig& camera_config) :
        camera_dev_node_{camera_config.camera_dev_node_},
        bytes_per_pixel_{camera_config.bytes_per_pixel_},
        width_enumerated_{camera_config.width_enumerated_},
        width_actual_{camera_config.width_actual_},
        height_{camera_config.height_},
        bytes_per_row_{camera_config.bytes_per_row_},
        fps_{camera_config.fps_},
        time_per_frame_{camera_config.time_per_frame_} {};

    std::string camera_dev_node_;
    std::string format_;
    int bytes_per_pixel_;
    int width_enumerated_;
    int width_actual_;
    int height_;
    int bytes_per_row_;
    int fps_;
    v4l2_fract time_per_frame_;
};

// +++++ from android project

typedef enum camera_frame_format
{
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
struct CameraFrame
{
    // public:
    uint32_t reuse_token;
    /** image data */
    uint8_t *data;
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