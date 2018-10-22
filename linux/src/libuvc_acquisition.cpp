#include <stdio.h>
#include <unistd.h>

extern "C" {
#include "libuvc/libuvc.h"
}

// TODO abstract/encapsulate CameraConfig
#include "render_ui.h"
#include "frame_access_ifc.h"

static int unsigned width_, height_;

static int unsigned bytes_per_pixel_;
static int unsigned bytes_per_row_;
static IFrameAccess* frame_access_ifc_;

extern CameraConfig camera_config;

void cb(uvc_frame_t *frame, void *ptr) {

// TODO initally return yuv and decode in opencv module
// allow client to request RGB formats later ...
#if 1
    IFrameAccess::Frame client_frame = frame_access_ifc_->GetFrame();

    memcpy(client_frame.buffer, frame->data, frame->data_bytes);
    frame_access_ifc_->Signal();
#else
    uvc_error_t ret;
    uvc_frame_t *rgb;
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
        frame_access_ifc_->Signal();
    }

    uvc_free_frame(rgb);
#endif
}

static uvc_context_t *ctx;
static uvc_device_t *dev = NULL;
static uvc_device_handle_t *devh = NULL;


int uvc_init(IFrameAccess* frame_access_ifc, int vid, int pid, int enumerated_width, int enumerated_height, int actual_width, int actual_height) {

    uvc_error_t res;
    uvc_stream_ctrl_t ctrl;

    width_ = actual_width;
    height_ = actual_height;

	bytes_per_pixel_ = 2;
    bytes_per_row_ = bytes_per_pixel_ * actual_width;

    LOG("uvc_init() entry\n");
    LOG("vid : %x, pid : %x, enumerated_width : %d, enumerated_height : %d\n",
        vid, pid, enumerated_width, enumerated_height);

    frame_access_ifc_ = frame_access_ifc;

    res = uvc_init(&ctx, NULL);

    if (res < 0) {
        uvc_perror(res, "uvc_init");
        return res;
    }

    puts("UVC initialized\n");

    if (0 != (res = uvc_find_device(ctx, &dev, vid, pid, NULL))) {
        LOG("no UVC camera detected\n");
        return -1;
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
                enumerated_width, enumerated_height, camera_config.fps);

            res = uvc_get_stream_ctrl_format_size(devh, &ctrl, UVC_FRAME_FORMAT_YUYV, enumerated_width, enumerated_height, camera_config.fps);

            uvc_print_stream_ctrl(&ctrl, stderr);

            if (res < 0) {
                LOG("uvc_get_stream_ctrl_format_size() failure\n");
                uvc_perror(res, "get_mode");
            } else {

                res = uvc_start_streaming(devh, &ctrl, cb, NULL, 0);

                if (res < 0) {
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


