#ifndef WEBCAM_UTIL_H
#define WEBCAM_UTIL_H

#include "webcam_include.h"

CameraFrame* allocate_camera_frame(size_t data_bytes);

void free_camera_frame(CameraFrame* frame);

int get_bytes_per_pixel(CameraFormat camera_format);

CameraError ensure_frame_size(CameraFrame* camera_frame, size_t need_bytes);

#endif // WEBCAM_UTIL_H