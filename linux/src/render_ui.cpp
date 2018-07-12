#include "render_ui.h"
#include <iostream>

#include <gtkmm.h>

int width_ = 0;
int height_ = 0;

extern int shift_down();
extern int shift_up();

unsigned char* g_rgb_buffer = nullptr;

CameraConfig camera_config[NUM_UVC_CAMERA] = {
    {2, 640, 640, 480, 30},     // UVC_CAMERA_LOGITECH_WEBCAM
    {4, 240, 120, 720, 60},     // UVC_CAMERA_MVIS_LIDAR
};


RenderUI::RenderUI() :
    m_scaling(SCALE_MODE_BILINEAR),
    m_UvcCamera(UVC_CAMERA_NONE),
    m_UvcMode(UVC_MODE_LIBUSB),
    m_VBox(Gtk::ORIENTATION_VERTICAL, 5),
    m_ButtonBox(Gtk::ORIENTATION_HORIZONTAL),
    m_ButtonStartStop("start stream"),
    m_ButtonShiftDown(">>"),
    m_ButtonShiftUp("<<"),
    m_ButtonScaling("Toggle Scaling"),
    m_ButtonQuit("_Quit", /* mnemonic= */ true),
    m_FpsLabel("fps xy.xx", true),
    m_Banner("UVC Stream Demo"),
    m_update_scroll_view(true),
    m_ScrolledWindow(),
    m_TextView(),
    m_message(),
    m_Dispatcher(),
    m_Worker(),
    m_WorkerThread(nullptr)
{
    set_title("Multi-threaded example");
    set_border_width(5);

    set_default_size(1280 + 10, 720 + 10);

    Gdk::RGBA rgba;
    rgba.set_rgba(0, 0, 0, 1.0);
    override_background_color(rgba);

    add(m_VBox);

    // banner label
    m_VBox.pack_start(m_Banner, Gtk::PACK_SHRINK);

    // image views
    m_VBox.pack_start(m_Image);
    m_VBox.pack_start(m_ImageScaled);

    // Add the TextView, inside a ScrolledWindow.
    m_ScrolledWindow.add(m_TextView);

    // Only show the scrollbars when they are necessary.
    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_VBox.pack_start(m_ScrolledWindow);

    m_TextView.set_editable(false);

    // Add the buttons to the ButtonBox.
    m_VBox.pack_start(m_ButtonBox, Gtk::PACK_SHRINK);

    m_ButtonBox.pack_start(m_ButtonShiftDown, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_ButtonShiftUp, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_ButtonScaling, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_FpsLabel, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_ButtonStartStop, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_ButtonQuit, Gtk::PACK_SHRINK);
    m_ButtonBox.set_border_width(5);
    m_ButtonBox.set_spacing(5);
    m_ButtonBox.set_layout(Gtk::BUTTONBOX_END);

    m_ButtonStartStop.override_color(Gdk::RGBA("green"), Gtk::STATE_FLAG_NORMAL);

    // Connect the signal handlers to the buttons.
    m_ButtonStartStop.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_start_button_clicked));
    m_ButtonShiftDown.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_shift_down_button_clicked));
    m_ButtonShiftUp.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_shift_up_button_clicked));
    m_ButtonScaling.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_scaling_button_clicked));
    m_ButtonQuit.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_quit_button_clicked));

    // Connect the handler to the dispatcher.
    m_Dispatcher.connect(sigc::mem_fun(*this, &RenderUI::on_notification_from_worker_thread));

    // Create a text buffer mark for use in update_widgets().
    auto buffer = m_TextView.get_buffer();
    buffer->create_mark("last_line", buffer->end(), /* left_gravity= */ true);

    show_all_children();

    m_FpsLabel.hide();
    m_ButtonScaling.hide();
    m_ButtonShiftDown.hide();
    m_ButtonShiftUp.hide();
}


std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}


void RenderUI::on_start_button_clicked()
{
    if (m_WorkerThread)
    {
        // Order the worker thread to stop.
        m_Worker.stop_work();
        m_device_node_string.clear();

        m_ButtonStartStop.set_label("open stream");
        m_ButtonStartStop.override_color(Gdk::RGBA("green"), Gtk::STATE_FLAG_NORMAL);

        m_FpsLabel.hide();
        m_ButtonScaling.hide();
        m_ButtonShiftDown.hide();
        m_ButtonShiftUp.hide();
    
    } else {
          // query for installed cameras
        std::string list_devices = exec("v4l2-ctl --list-devices");
        std::cout << list_devices << "\n";

        m_UvcCamera = UVC_CAMERA_NONE;
        size_t pos = 0;
        std::string list_devices_iter = list_devices;

        // webcamera is highest priority
        std::string delimiter = "UVC Camera (046d:0825)";
        std::cout << "seeking " << delimiter << "\n";
        if (std::string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
            list_devices_iter = list_devices_iter.substr(pos + delimiter.length());
            delimiter = "/dev/video";
            if (std::string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
                m_device_node_string = list_devices_iter.substr(pos, delimiter.length() + 1);
                std::cout << "m_device_node_string : " << m_device_node_string << "\n";

                // extract pid/vid for libusb (lidar camera does not present as part of v4l2-ctl --list-devices response)
                std::string pid_vid = exec("lsusb");
                std::string pid_vid_inter = pid_vid;
                std::string delimiter = "Logitech, Inc. Webcam C270";
                if (std::string::npos != (pos = pid_vid_inter.find(delimiter, 0))) {
                    pid_vid_inter = pid_vid_inter.substr(pos - 10, 9);
                    std::string::size_type sz = 0;
                    m_vid = std::stoll(pid_vid_inter, &sz, 16);
                    pid_vid_inter = pid_vid_inter.substr(sz + 1);
                    m_pid = std::stoll(pid_vid_inter, &sz, 16);
                    std::cout << "pid:vid 0x" << std::hex << m_pid << ":0x" << m_vid << "\n";

                    m_UvcCamera = UVC_CAMERA_LOGITECH_WEBCAM;
                }
            }
        }

        // lidar camera if no webcamera
        if (m_device_node_string.size() == 0) {
            std::string delimiter = "FX3";
            std::cout << "seeking " << delimiter << "\n";
            if (std::string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
                list_devices_iter = list_devices_iter.substr(pos + delimiter.length());
                delimiter = "/dev/video";
                std::cout << "seeking " << delimiter << "\n";
                if (std::string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
                    m_device_node_string = list_devices_iter.substr(pos, delimiter.length() + 1);
                    std::cout << "m_device_node_string : " << m_device_node_string << "\n";

                    // extract pid/vid for libusb (lidar camera does not present as part of v4l2-ctl --list-devices response)
                    std::string pid_vid = exec("lsusb");
                    std::string pid_vid_inter = pid_vid;
                    std::string delimiter = "Cypress Semiconductor Corp.";
                    if (std::string::npos != (pos = pid_vid_inter.find(delimiter, 0))) {
                        pid_vid_inter = pid_vid_inter.substr(pos - 10, 9);

                        std::string::size_type sz = 0;
                        m_vid = std::stoll(pid_vid_inter, &sz, 16);
                        pid_vid_inter = pid_vid_inter.substr(sz + 1);
                        m_pid = std::stoll(pid_vid_inter, &sz, 16);
                        std::cout << "pid:vid 0x" << std::hex << m_pid << ":0x" << m_vid << "\n";

                        m_UvcCamera = UVC_CAMERA_MVIS_LIDAR;
                    }
                }
            }
        }

        if (m_device_node_string.size() == 0) {
            std::cout << "no camera detected" << std::endl;
            return;
        }

        m_ButtonStartStop.set_label("close stream");
        m_ButtonStartStop.override_color(Gdk::RGBA("red"), Gtk::STATE_FLAG_NORMAL);

        m_enumerated_width = camera_config[m_UvcCamera].width_enumerated;
        m_enumerated_height = camera_config[m_UvcCamera].height;

        m_actual_width = camera_config[m_UvcCamera].width_actual;
        m_actual_height = camera_config[m_UvcCamera].height;

        width_ = camera_config[m_UvcCamera].width_actual;
        height_ = camera_config[m_UvcCamera].height;

        // Start a new worker thread.
        m_WorkerThread = new std::thread(
          [this]
          {
            m_Worker.do_work(this, m_UvcMode, m_UvcCamera, m_device_node_string, m_vid, m_pid, m_enumerated_width, m_enumerated_height, m_actual_width, m_actual_height);
          });
    }

    std::ostringstream ostr;
    if (m_UvcMode == UVC_MODE_LIBUSB) {
        m_UvcMode = UVC_MODE_V4L2;
        ostr << "UVC_MODE_V4L2\n";
    } else {
        m_UvcMode = UVC_MODE_LIBUSB;
        ostr << "UVC_MODE_LIBUSB\n";
    }
    m_message += ostr.str();
    m_update_scroll_view = true;
}


void RenderUI::on_shift_down_button_clicked()
{
    shift_down();
}


void RenderUI::on_shift_up_button_clicked()
{
    shift_up();
}


void RenderUI::on_scaling_button_clicked()
{
  std::ostringstream ostr;

    switch (m_scaling) {
    case SCALE_MODE_NONE:
        m_scaling = SCALE_MODE_NEAREST;
        ostr << "SCALE_MODE_NEAREST\n";
        break;
    case SCALE_MODE_NEAREST:
        m_scaling = SCALE_MODE_BILINEAR;
        ostr << "SCALE_MODE_BILINEAR\n";
        break;
    case SCALE_MODE_BILINEAR:
        m_scaling = SCALE_MODE_NONE;
        ostr << "SCALE_MODE_NONE\n";
    default:
        break;
    }

    m_message += ostr.str();
    m_update_scroll_view = true;
}


void nop(const guint* data) {

}


void RenderUI::update_widgets()
{
    Glib::ustring message_from_worker_thread;

    // rough estimate for fps
    struct timespec start;
    static timeval last;
    double fps;

    // cache last render time
    clock_gettime(CLOCK_MONOTONIC, &start);

    fps = ((start.tv_sec - last.tv_sec) * 1000000) + ((start.tv_nsec / 1000) - last.tv_usec);
    fps = fps ? 1000000.0 / fps : 0.0;

    last.tv_sec = start.tv_sec;
    last.tv_usec = start.tv_nsec / 1000;

    {
        static int accum_index = 0;
        static double accum = 0;
        if (accum_index < 60) {
            accum += fps;
            accum_index++;
        } else {
            char buffer[11];
            snprintf(buffer, 11, "fps %5.2f", (accum / 60));
            m_FpsLabel.set_text(buffer);
            m_FpsLabel.show();
            if (m_UvcCamera == UVC_CAMERA_MVIS_LIDAR) {
                m_ButtonScaling.show();
                m_ButtonShiftDown.show();
                m_ButtonShiftUp.show();
            }
            accum_index = 0;
            accum = 0;
        }
    }

    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb = Gdk::Pixbuf::create_from_data(g_rgb_buffer, Gdk::COLORSPACE_RGB, FALSE, 8, width_, height_, width_*3);

    if (m_UvcCamera == UVC_CAMERA_MVIS_LIDAR) {
        switch (m_scaling) {
            case SCALE_MODE_NEAREST:
                m_ImageScaled.set(pixbuf_rgb->scale_simple(1200, 720, Gdk::INTERP_NEAREST));
                break;
            case SCALE_MODE_BILINEAR:
                m_ImageScaled.set(pixbuf_rgb->scale_simple(1200, 720, Gdk::INTERP_BILINEAR));
                break;    
            case SCALE_MODE_NONE:
                default:
                m_ImageScaled.set(pixbuf_rgb);
                break;
            }
        } else if (m_UvcCamera == UVC_CAMERA_LOGITECH_WEBCAM) {
        // from yuyv webcam
        m_ImageScaled.set(pixbuf_rgb);
    }

  // TODO mutex
  if (m_update_scroll_view) {

    static int num_renders = 0;
    num_renders++;
    auto buffer = m_TextView.get_buffer();
    buffer->set_text(m_message);

    // Scroll the last inserted line into view. That's somewhat complicated.
    Gtk::TextIter iter = buffer->end();
    iter.set_line_offset(0); // Beginning of last line
    auto mark = buffer->get_mark("last_line");
    buffer->move_mark(mark, iter);
    m_TextView.scroll_to(mark);
    // TextView::scroll_to(iter) is not perfect.
    // We do need a TextMark to always get the last line into view.

    if (num_renders > 10) {
      m_message.clear();
      num_renders = 0;
    }
    m_update_scroll_view = false;
  }

}


void RenderUI::on_quit_button_clicked()
{
    if (m_WorkerThread)
    {
        // Order the worker thread to stop and wait for it to stop.
        m_Worker.stop_work();
        if (m_WorkerThread->joinable())
            m_WorkerThread->join();
    }
    hide();
}


// notify() is called from ExampleWorker::do_work(). It is executed in the worker
// thread. It triggers a call to on_notification_from_worker_thread(), which is
// executed in the GUI thread.
void RenderUI::notify()
{
    m_Dispatcher.emit();
}


void RenderUI::on_notification_from_worker_thread()
{
    if (m_WorkerThread && m_Worker.has_stopped())
    {
        LOG("(m_WorkerThread && m_Worker.has_stopped()) == true\n");
            // fill frame with null
        memset(g_rgb_buffer, 0, width_*3*height_);
        update_widgets();
        // Work is done.
        if (m_WorkerThread->joinable())
            m_WorkerThread->join();
        delete m_WorkerThread;
        m_WorkerThread = nullptr;
    }

  update_widgets();
}
