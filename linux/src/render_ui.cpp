#include "render_ui.h"
#include <iostream>

#include <gtkmm.h>

int width_ = 0;
int height_ = 0;

extern int shift_down();
extern int shift_up();

unsigned char* g_rgb_buffer = nullptr;

using namespace std;

CameraConfig camera_config[NUM_UVC_CAMERA] = {
    {2, 640, 640, 480, 30},     // UVC_CAMERA_LOGITECH_WEBCAM
    {4, 240, 120, 720, 60},     // UVC_CAMERA_MVIS_LIDAR
};


void set_widget_margin(Gtk::Widget& widget, int margin) 
{
    widget.set_margin_left(margin);
    widget.set_margin_right(margin);
    widget.set_margin_top(margin);
    widget.set_margin_bottom(margin);
}

RenderUI::RenderUI() :
    interior_border_(5),
    widget_margin_(5),
    full_screen_(false),
    m_scaling(SCALE_MODE_NEAREST),
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
    m_Banner("UVC Video / OpenCV Demo"),
    m_update_scroll_view(true),
    m_ScrolledWindow(),
    m_TextView(),
    m_message(),
    m_Dispatcher(),
    m_Worker(),
    m_WorkerThread(nullptr)
{
    set_title("UVC Camera OpenCV Demo");

    set_border_width(interior_border_);

    // display screen resolution
    Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default(); 
    int primary_monitor = screen->get_primary_monitor();
    Gdk::Rectangle rect;
    screen->get_monitor_geometry(primary_monitor, rect);
    screen_width_ = rect.get_width();
    screen_height_ = rect.get_height();
    cout << "screen dimensions : " << screen_width_ << " x " << screen_height_ << endl;

    window_width_ = screen_width_ / 2;
    window_height_ = screen_height_ / 2;

    set_default_size(window_width_, window_height_);
    set_position(Gtk::WIN_POS_CENTER);

    Gdk::RGBA rgba;
    rgba.set_rgba(0, 0, 0, 1.0);
    override_background_color(rgba);

    add(m_VBox);

    // banner label
    m_VBox.pack_start(m_Banner, Gtk::PACK_SHRINK);

    // auto-scaled image views
    image_scaled_src_.add(image_src_);
    image_scaled_proc_.add(image_proc_);
    event_box_src_.add(image_scaled_src_);
    event_box_proc_.add(image_scaled_proc_);
    //video_box_.pack_start(image_scaled_src_, Gtk::PACK_EXPAND_WIDGET);
    //video_box_.pack_start(image_scaled_proc_, Gtk::PACK_EXPAND_WIDGET);
    video_box_.pack_start(event_box_src_, Gtk::PACK_EXPAND_WIDGET);
    video_box_.pack_start(event_box_proc_, Gtk::PACK_EXPAND_WIDGET);
    m_VBox.pack_start(video_box_);
    //m_VBox.pack_start(image_scaled_src_);
    //m_VBox.pack_start(image_scaled_proc_);
    set_widget_margin(image_scaled_src_, widget_margin_);
    set_widget_margin(image_scaled_proc_, widget_margin_);

    // Add the TextView, inside a ScrolledWindow.
    m_ScrolledWindow.add(m_TextView);

    // Only show the scrollbars when they are necessary.
    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    // sde m_VBox.pack_start(m_ScrolledWindow);
    m_VBox.pack_start(m_ScrolledWindow, Gtk::PACK_SHRINK);

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

    // image view mouse button click handler
    event_box_src_.add_events(Gdk::BUTTON_PRESS_MASK);
    event_box_src_.signal_button_press_event().connect(sigc::mem_fun(*this, &RenderUI::on_image_src_button_pressed));
    event_box_proc_.add_events(Gdk::BUTTON_PRESS_MASK);
    event_box_proc_.signal_button_press_event().connect(sigc::mem_fun(*this, &RenderUI::on_image_proc_button_pressed));

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


string exec(const char* cmd) {
    array<char, 128> buffer;
    string result;
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}


bool RenderUI::on_key_press_event(GdkEventKey* key_event)
{
    cout << "on_key_press_event() entry" << endl;

    switch (key_event->keyval)
    {
        case GDK_KEY_Escape:
            if (full_screen_) {
                full_screen_ = false;
                cout << "GDK_KEY_Escape unfullscreen()" << endl;
                unfullscreen();
            } else {
                full_screen_ = true;
                cout << "GDK_KEY_Escape fullscreen()" << endl;
                fullscreen();
            }

        default:
            break;
    }

    return Gtk::Window::on_key_press_event(key_event);
}


bool RenderUI::on_image_src_button_pressed(GdkEventButton* button_event)
{
    cout << "on_image_src_button_pressed()" << endl;
    
    if (event_box_proc_.is_visible()) {
        event_box_proc_.hide();
    } else {
        event_box_proc_.show();
    }

    return false;
}


bool RenderUI::on_image_proc_button_pressed(GdkEventButton* button_event)
{
    cout << "on_image_proc_button_pressed()" << endl;

    if (event_box_src_.is_visible()) {
        event_box_src_.hide();
    } else {
        event_box_src_.show();
    }

    return false;
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
        string list_devices = exec("v4l2-ctl --list-devices");
        cout << list_devices << "\n";

        m_UvcCamera = UVC_CAMERA_NONE;
        size_t pos = 0;
        string list_devices_iter = list_devices;

        // webcamera is highest priority
        string delimiter = "UVC Camera (046d:0825)";
        cout << "seeking " << delimiter << "\n";
        if (string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
            list_devices_iter = list_devices_iter.substr(pos + delimiter.length());
            delimiter = "/dev/video";
            if (string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
                m_device_node_string = list_devices_iter.substr(pos, delimiter.length() + 1);
                cout << "m_device_node_string : " << m_device_node_string << "\n";

                // extract pid/vid for libusb (lidar camera does not present as part of v4l2-ctl --list-devices response)
                string pid_vid = exec("lsusb");
                string pid_vid_inter = pid_vid;
                string delimiter = "Logitech, Inc. Webcam C270";
                if (string::npos != (pos = pid_vid_inter.find(delimiter, 0))) {
                    pid_vid_inter = pid_vid_inter.substr(pos - 10, 9);
                    string::size_type sz = 0;
                    m_vid = stoll(pid_vid_inter, &sz, 16);
                    pid_vid_inter = pid_vid_inter.substr(sz + 1);
                    m_pid = stoll(pid_vid_inter, &sz, 16);
                    cout << "pid:vid 0x" << hex << m_pid << ":0x" << m_vid << dec << endl;

                    m_UvcCamera = UVC_CAMERA_LOGITECH_WEBCAM;
                }
            }
        }

        // lidar camera if no webcamera
        if (m_device_node_string.size() == 0) {
            string delimiter = "FX3";
            cout << "seeking " << delimiter << "\n";
            if (string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
                list_devices_iter = list_devices_iter.substr(pos + delimiter.length());
                delimiter = "/dev/video";
                cout << "seeking " << delimiter << "\n";
                if (string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
                    m_device_node_string = list_devices_iter.substr(pos, delimiter.length() + 1);
                    cout << "m_device_node_string : " << m_device_node_string << "\n";

                    // extract pid/vid for libusb (lidar camera does not present as part of v4l2-ctl --list-devices response)
                    string pid_vid = exec("lsusb");
                    string pid_vid_inter = pid_vid;
                    string delimiter = "Cypress Semiconductor Corp.";
                    if (string::npos != (pos = pid_vid_inter.find(delimiter, 0))) {
                        pid_vid_inter = pid_vid_inter.substr(pos - 10, 9);

                        string::size_type sz = 0;
                        m_vid = stoll(pid_vid_inter, &sz, 16);
                        pid_vid_inter = pid_vid_inter.substr(sz + 1);
                        m_pid = stoll(pid_vid_inter, &sz, 16);
                        cout << "pid:vid 0x" << hex << m_pid << ":0x" << m_vid << dec << endl;

                        m_UvcCamera = UVC_CAMERA_MVIS_LIDAR;
                    }
                }
            }
        }

        if (m_device_node_string.size() == 0) {
            cout << "no camera detected" << endl;
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

        image_scaled_src_.SetAspectRatio(width_, height_);
        image_scaled_proc_.SetAspectRatio(width_, height_);

        // Start a new worker thread.
        m_WorkerThread = new thread(
          [this]
          {
            m_Worker.do_work(this, m_UvcMode, m_UvcCamera, m_device_node_string, m_vid, m_pid, m_enumerated_width, m_enumerated_height, m_actual_width, m_actual_height);
          });
    }

    ostringstream ostr;
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
  ostringstream ostr;

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
            } else if (m_UvcCamera ==UVC_CAMERA_LOGITECH_WEBCAM) {
                m_ButtonScaling.show();
            }
            accum_index = 0;
            accum = 0;
        }
    }

    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb_src = Gdk::Pixbuf::create_from_data(g_rgb_buffer, Gdk::COLORSPACE_RGB, FALSE, 8, width_, height_, width_*3);
    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb_proc = Gdk::Pixbuf::create_from_data(g_rgb_buffer, Gdk::COLORSPACE_RGB, FALSE, 8, width_, height_, width_*3);

    int image_src_render_width_ =  image_scaled_src_.GetImageWidth();
    int image_src_render_height_ =  image_scaled_src_.GetImageHeight();

    int image_proc_render_width_ =  image_scaled_proc_.GetImageWidth();
    int image_proc_render_height_ =  image_scaled_proc_.GetImageHeight();

    if (m_UvcCamera == UVC_CAMERA_MVIS_LIDAR) {

        switch (m_scaling) {
        case SCALE_MODE_NEAREST:
            image_src_.set(pixbuf_rgb_src->scale_simple(image_src_render_width_, image_src_render_height_, Gdk::INTERP_NEAREST));
            image_proc_.set(pixbuf_rgb_proc->scale_simple(image_proc_render_width_, image_proc_render_height_, Gdk::INTERP_NEAREST));
            break;
        case SCALE_MODE_BILINEAR:
            image_src_.set(pixbuf_rgb_src->scale_simple(image_src_render_width_, image_src_render_height_, Gdk::INTERP_BILINEAR));
            image_proc_.set(pixbuf_rgb_proc->scale_simple(image_proc_render_width_, image_proc_render_height_, Gdk::INTERP_BILINEAR));
            break;    
        case SCALE_MODE_NONE:
        default:
            image_src_.set(pixbuf_rgb_src);
            image_proc_.set(pixbuf_rgb_proc);
            break;
        }

    } else if (m_UvcCamera == UVC_CAMERA_LOGITECH_WEBCAM) {

        switch (m_scaling) {
        case SCALE_MODE_NEAREST:
            image_src_.set(pixbuf_rgb_src->scale_simple(image_src_render_width_, image_src_render_height_ , Gdk::INTERP_NEAREST));
            image_proc_.set(pixbuf_rgb_proc->scale_simple(image_proc_render_width_, image_proc_render_height_ , Gdk::INTERP_NEAREST));
            break;
        case SCALE_MODE_BILINEAR:
            image_src_.set(pixbuf_rgb_src->scale_simple(image_src_render_width_, image_src_render_height_ , Gdk::INTERP_BILINEAR));
            image_proc_.set(pixbuf_rgb_proc->scale_simple(image_proc_render_width_, image_proc_render_height_ , Gdk::INTERP_BILINEAR));
            break;    
        case SCALE_MODE_NONE:
        default:
            image_src_.set(pixbuf_rgb_src);
            image_proc_.set(pixbuf_rgb_proc);
            break;
        }

        // explicit render of image residing in event boxed scrolled window
        event_box_src_.queue_draw();
        event_box_proc_.queue_draw();

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
