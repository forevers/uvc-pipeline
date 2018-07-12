#include "uvc_worker.h"
// TODO remove cyclic once interfaces are defined
#include "render_ui.h"

#include <iostream>
#include <sstream>
#include <chrono>

extern int uvc_init(RenderUI* caller, UvcCamera uvc_camera, int vid, int pid, int enumerated_width, int enumerated_height, int actual_width, int actual_height);
extern int uvc_exit();

extern int uvc_v4l2_init(RenderUI* caller, UvcCamera uvc_camera, std::string device_node, int enumerated_width, int enumerated_height, int actual_width, int actual_height);
extern int uvc_v4l2_get_frame(void);
extern int uvc_v4l2_exit();

// extern unsigned char g_rgb_buffer[WIDTH*3*HEIGHT];
extern unsigned char* g_rgb_buffer;

ExampleWorker::ExampleWorker() :
    m_Mutex(),
    m_shall_stop(false),
    m_has_stopped(false),
    m_message()
{
}

// Accesses to these data are synchronized by a mutex.
// Some microseconds can be saved by getting all data at once, instead of having
// separate get_fraction_done() and get_message() methods.
void ExampleWorker::get_data(double* fraction_done, Glib::ustring* message) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (message)
        *message = m_message;
}


void ExampleWorker::stop_work()
{
    LOG("stop_work() entry\n");

    std::lock_guard<std::mutex> lock(m_Mutex);
    LOG("stop_work() lock obtained\n");

    m_shall_stop = true;

    LOG("stop_work() exit\n");
}

bool ExampleWorker::has_stopped() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_has_stopped;
}


void ExampleWorker::do_work(RenderUI* caller, UvcMode uvc_mode, UvcCamera uvc_camera, std::string device_node_string, int vid, int pid, int enumerated_width, int enumerated_height, int actual_width, int actual_height)
{
    LOG("do_work() entry\n");

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_has_stopped = false;
        m_message = "";
    }

    if (uvc_mode == UVC_MODE_LIBUSB) {
        uvc_init(caller, uvc_camera, vid, pid, enumerated_width, enumerated_height, actual_width, actual_height);
    } else {
        if (0 != uvc_v4l2_init(caller, uvc_camera, device_node_string, enumerated_width, enumerated_height, actual_width, actual_height)) {
            LOG("%d > uvc_v4l2_init() failure\n", __LINE__);
            return;
        }
    }

    while (!m_shall_stop) {
        
        if (uvc_mode == UVC_MODE_LIBUSB) {

          //std::this_thread::sleep_for(std::chrono::milliseconds(250));
          // 60fps simulation
          std::this_thread::sleep_for(std::chrono::microseconds(16667));
          {
              std::lock_guard<std::mutex> lock(m_Mutex);

              if (m_shall_stop)
              {
                  m_message += "Stopped";
                  break;
              }
          }
        } else {
            {
              //std::lock_guard<std::mutex> lock(m_Mutex);
              // 4l2 will block, issue signal to release mutex until frame arrives
              uvc_v4l2_get_frame();
            }

            if (m_shall_stop)
            {
              m_message += "Stopped";
              break;
            }

          caller->notify();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (uvc_mode == UVC_MODE_LIBUSB) {
            uvc_exit();
        } else {
            uvc_v4l2_exit();
        }
        m_shall_stop = false;
        m_has_stopped = true;
    }

    caller->notify();
}
