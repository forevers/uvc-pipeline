#include <stdio.h>
//#include <opencv/highgui.h>

#include <unistd.h>

extern "C" {
#include "libuvc/libuvc.h"
}

#include "render_ui.h"

static int unsigned width_, height_;

static int unsigned bytes_per_pixel_;
static int unsigned bytes_per_row_;

extern unsigned char* g_rgb_buffer;

static RenderUI* g_caller = NULL;

static UvcCamera uvc_camera_ = UVC_CAMERA_NONE;

extern CameraConfig camera_config[NUM_UVC_CAMERA];

void cb(uvc_frame_t *frame, void *ptr) {
    uvc_frame_t *rgb;
    uvc_error_t ret;

    if (uvc_camera_ == UVC_CAMERA_MVIS_LIDAR) {
        uint8_t* buffer = (uint8_t*)frame->data;

        for (unsigned int row = 0; row < height_; row++) {
            for (unsigned int col = 0; col < width_; col++) {
                //uint16_t depth_val = buffer[row*bytes_per_row_ + bytes_per_pixel_*col + 0] + (buffer[row*bytes_per_row_ + bytes_per_pixel_*col + 1] << 8);
                uint16_t amplitude_val = buffer[row*bytes_per_row_ + bytes_per_pixel_*col + 2] + (buffer[row*bytes_per_row_ + bytes_per_pixel_*col + 3] << 8);

                // TODO demux module to split depth/amplitude into 2 frame field streams
                //uint8_t shifted_depth_val = (depth_val >> 1) & 0xFF;
                uint8_t shifted_amplitude_val = amplitude_val >> 4;

                g_rgb_buffer[row*3*width_ + 3*col] = shifted_amplitude_val;
                g_rgb_buffer[row*3*width_ + 3*col + 1] = shifted_amplitude_val;
                g_rgb_buffer[row*3*width_ + 3*col + 2] = shifted_amplitude_val;
            }
        }

        g_caller->notify();

    } else if (uvc_camera_ == UVC_CAMERA_LOGITECH_WEBCAM) {

        rgb = uvc_allocate_frame(frame->width * frame->height * 3);
        if (!rgb) {
            LOG("unable to allocate rgb frame!");
            return;
        }

        ret = uvc_any2rgb(frame, rgb);
        if (ret) {
            uvc_perror(ret, "uvc_any2rgb");
            uvc_free_frame(rgb);
            return;
        } else {
            memcpy(g_rgb_buffer, rgb->data, rgb->data_bytes);
            g_caller->notify();
        }

        uvc_free_frame(rgb);
    }

}

static uvc_context_t *ctx;
static uvc_device_t *dev = NULL;
static uvc_device_handle_t *devh = NULL;


int uvc_init(RenderUI* caller, UvcCamera uvc_camera, int vid, int pid, int enumerated_width, int enumerated_height, int actual_width, int actual_height) {

    uvc_error_t res;
    uvc_stream_ctrl_t ctrl;

    width_ = actual_width;
    height_ = actual_height;

    if (uvc_camera == UVC_CAMERA_LOGITECH_WEBCAM) {
        bytes_per_pixel_ = 2;
    } else if (uvc_camera == UVC_CAMERA_MVIS_LIDAR) {
        bytes_per_pixel_ = 4;
    }
    bytes_per_row_ = bytes_per_pixel_ * actual_width;

    LOG("uvc_init() entry\n");
    LOG("uvc_camera : %d, vid : %x, pid : %x, enumerated_width : %d, enumerated_height : %d\n",
        uvc_camera, vid, pid, enumerated_width, enumerated_height);

    uvc_camera_ = uvc_camera;

    g_caller = caller;

    res = uvc_init(&ctx, NULL);

    if (res < 0) {
        uvc_perror(res, "uvc_init");
        return res;
    }

    puts("UVC initialized\n");

    if (0 != (res = uvc_find_device(ctx, &dev, vid, pid, NULL))) {
        uvc_camera_ = UVC_CAMERA_NONE;
    }

    if (res < 0) {
        uvc_perror(res, "uvc_find_device");
    } else {
        puts("Device found");

        res = uvc_open(dev, &devh);

        if (res < 0) {
            uvc_perror(res, "uvc_open");
        } else {
            puts("Device opened");

            uvc_print_diag(devh, stderr);

            LOG("width_enumerated : %d, enumerated_height : %d, fps : %d\n",
                enumerated_width, enumerated_height, camera_config[uvc_camera_].fps);

            res = uvc_get_stream_ctrl_format_size(devh, &ctrl, UVC_FRAME_FORMAT_YUYV, enumerated_width, enumerated_height, camera_config[uvc_camera_].fps);

            uvc_print_stream_ctrl(&ctrl, stderr);

            if (res < 0) {
                LOG("uvc_get_stream_ctrl_format_size() failure\n");
                uvc_perror(res, "get_mode");
            } else {

                // allocate single buffer
                if (g_rgb_buffer != nullptr) delete [] g_rgb_buffer;

                int buffer_size = camera_config[uvc_camera_].width_actual*3*camera_config[uvc_camera_].height;
                g_rgb_buffer = new unsigned char[buffer_size];

                res = uvc_start_streaming(devh, &ctrl, cb, NULL, 0);

                if (res < 0) {
                    delete [] g_rgb_buffer;
                    g_rgb_buffer = nullptr;

                    LOG("uvc_start_streaming() failure\n");
                    uvc_perror(res, "start_streaming");
                } else {
#if 0
          puts("Streaming for 10 seconds...");
          uvc_error_t resAEMODE = uvc_set_ae_mode(devh, 1);
          uvc_perror(resAEMODE, "set_ae_mode");
          int i;
          for (i = 1; i <= 10; i++) {
            /* uvc_error_t resPT = uvc_set_pantilt_abs(devh, i * 20 * 3600, 0); */
            /* uvc_perror(resPT, "set_pt_abs"); */
            uvc_error_t resEXP = uvc_set_exposure_abs(devh, 20 + i * 5);
            uvc_perror(resEXP, "set_exp_abs");
            
            sleep(1);
          }
          sleep(10);
          uvc_stop_streaming(devh);
	  puts("Done streaming.");
#endif
                }
            }
        }
    }

  LOG("uvc_init() exit\n");

  return res;
}

int uvc_exit(void) {
    LOG("uvc_exit() entry\n");

    uvc_stop_streaming(devh);
    uvc_close(devh);
    uvc_unref_device(dev);
    uvc_exit(ctx);

    LOG("uvc_exit() exit\n");

    return 0;
}


