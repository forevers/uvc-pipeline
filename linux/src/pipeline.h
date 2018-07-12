#ifndef PIPELINE_H
#define PIPELINE_H

enum UvcMode {
    UVC_MODE_LIBUSB,
    UVC_MODE_V4L2,
    NUM_UVC_MODE,
};

enum UvcCamera {
    UVC_CAMERA_LOGITECH_WEBCAM,
    UVC_CAMERA_MVIS_LIDAR,
    NUM_UVC_CAMERA,
    UVC_CAMERA_NONE,
};

struct CameraConfig {
    int bytes_per_pixel;
    int width_enumerated;
    int width_actual;
    int height;
    int bytes_per_row;
    int fps;
};

#define LOG(FMT, ...) printf("[%d:%s]:" FMT, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#endif // PIPELINE_H
