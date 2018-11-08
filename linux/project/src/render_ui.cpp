#include "render_ui.h"
#include <iostream>

#include <gtkmm.h>

extern int shift_down();
extern int shift_up();

using namespace std;

CameraConfig camera_config = {2, 640, 640, 480, 30};


void set_widget_margin(Gtk::Widget& widget, int margin) 
{
    widget.set_margin_left(margin);
    widget.set_margin_right(margin);
    widget.set_margin_top(margin);
    widget.set_margin_bottom(margin);
}

RenderUI::RenderUI() :
	frame_width_(0), frame_height_(0),
	enumerated_width_(0), enumerated_height_(0),
	actual_width_(0), actual_height_(0),
	vid_(0), pid_(0),
    interior_border_(5),
    widget_margin_(5),
    full_screen_(false),
    scaling_(SCALE_MODE_NEAREST),
    uvc_mode_(UVC_MODE_LIBUSB),
    vertical_box_(Gtk::ORIENTATION_VERTICAL, 5),
    button_box_(Gtk::ORIENTATION_HORIZONTAL),
    button_start_stop_("start stream"),
    button_scaling_("Toggle Scaling"),
    button_quit_("_Quit", /* mnemonic= */ true),
    fps_label_("fps xy.xx", true),
    uvc_mode_selection_frame_("uvc library"),
    uvc_mode_selection_(),
    banner_("UVC Video / OpenCV Demo"),
    update_scroll_view_(true),
    scrolled_window_(),
    text_view_(),
    message_(),
    dispatcher_(),
    worker_(),
    worker_thread_(nullptr),
    mutex_()
{
    uvc_frame_.data = nullptr;
    uvc_frame_.data_bytes = 0;
    uvc_frame_.actual_bytes = 0;
    uvc_frame_.width = 0;
    uvc_frame_.height = 0;
    uvc_frame_.frame_format = CAMERA_FRAME_FORMAT_YUYV;
    uvc_frame_.step = 0;
    uvc_frame_.sequence = 0;
    uvc_frame_.capture_time.tv_sec = 0;
    uvc_frame_.capture_time.tv_usec = 0;

    rgb_frame_.data = nullptr;
    rgb_frame_.data_bytes = 0;
    rgb_frame_.actual_bytes = 0;
    rgb_frame_.width = 0;
    rgb_frame_.height = 0;
    rgb_frame_.frame_format = CAMERA_FRAME_FORMAT_RGB;
    rgb_frame_.step = 0;
    rgb_frame_.sequence = 0;
    rgb_frame_.capture_time.tv_sec = 0;
    rgb_frame_.capture_time.tv_usec = 0;

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

    add(vertical_box_);

    // banner label
    vertical_box_.pack_start(banner_, Gtk::PACK_SHRINK);

    // auto-scaled image views
    image_scaled_src_.add(image_src_);
    image_scaled_proc_.add(image_proc_);
    event_box_src_.add(image_scaled_src_);
    event_box_proc_.add(image_scaled_proc_);
    video_box_.pack_start(event_box_src_, Gtk::PACK_EXPAND_WIDGET);
    video_box_.pack_start(event_box_proc_, Gtk::PACK_EXPAND_WIDGET);
    vertical_box_.pack_start(video_box_);

    set_widget_margin(image_scaled_src_, widget_margin_);
    set_widget_margin(image_scaled_proc_, widget_margin_);

    // Add the TextView, inside a ScrolledWindow.
    scrolled_window_.add(text_view_);

    // Only show the scrollbars when they are necessary.
    scrolled_window_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    // sde vertical_box_.pack_start(scrolled_window_);
    vertical_box_.pack_start(scrolled_window_, Gtk::PACK_SHRINK);

    text_view_.set_editable(false);

    // Add the buttons to the ButtonBox.
    vertical_box_.pack_start(button_box_, Gtk::PACK_SHRINK);

    uvc_mode_selection_box_.pack_start(uvc_mode_selection_frame_, Gtk::PACK_EXPAND_WIDGET);
    uvc_mode_selection_.append("libusb");
    uvc_mode_selection_.append("v4l2");
    uvc_mode_selection_.set_active(0);
    uvc_mode_selection_frame_.add(uvc_mode_selection_);
    uvc_mode_selection_.signal_changed().connect(sigc::mem_fun(*this,  &RenderUI::on_uvc_mode_changed));

    button_box_.pack_start(button_scaling_, Gtk::PACK_SHRINK);
    button_box_.pack_start(fps_label_, Gtk::PACK_SHRINK);
    button_box_.pack_start(uvc_mode_selection_box_, Gtk::PACK_EXPAND_WIDGET);
    button_box_.pack_start(button_start_stop_, Gtk::PACK_SHRINK);
    button_box_.pack_start(button_quit_, Gtk::PACK_SHRINK);
    button_box_.set_border_width(5);
    button_box_.set_spacing(5);
    button_box_.set_layout(Gtk::BUTTONBOX_END);

    set_widget_margin(button_scaling_, widget_margin_);
    set_widget_margin(fps_label_, widget_margin_);
    set_widget_margin(uvc_mode_selection_box_, widget_margin_);
    set_widget_margin(button_start_stop_, widget_margin_);
    set_widget_margin(button_quit_, widget_margin_);

    button_start_stop_.override_color(Gdk::RGBA("green"), Gtk::STATE_FLAG_NORMAL);

    // image view mouse button click handler
    event_box_src_.add_events(Gdk::BUTTON_PRESS_MASK);
    event_box_src_.signal_button_press_event().connect(sigc::mem_fun(*this, &RenderUI::on_image_src_button_pressed));
    event_box_proc_.add_events(Gdk::BUTTON_PRESS_MASK);
    event_box_proc_.signal_button_press_event().connect(sigc::mem_fun(*this, &RenderUI::on_image_proc_button_pressed));

    // Connect the signal handlers to the buttons.
    button_start_stop_.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_start_button_clicked));
    button_scaling_.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_scaling_button_clicked));
    button_quit_.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_quit_button_clicked));

    // Connect the handler to the dispatcher.
    dispatcher_.connect(sigc::mem_fun(*this, &RenderUI::on_notification_from_worker_thread));

    // Create a text buffer mark for use in update_widgets().
    auto buffer = text_view_.get_buffer();
    buffer->create_mark("last_line", buffer->end(), /* left_gravity= */ true);

    show_all_children();

    fps_label_.hide();
    button_scaling_.hide();
}


RenderUI::~RenderUI()
{
	if (uvc_frame_.data != nullptr) {
        delete [] uvc_frame_.data;
        uvc_frame_.data != nullptr;
    }
    uvc_frame_.data_bytes = 0;
    uvc_frame_.actual_bytes = 0;
    uvc_frame_.width = 0;
    uvc_frame_.height = 0;
    uvc_frame_.frame_format = CAMERA_FRAME_FORMAT_YUYV;
    uvc_frame_.step = 0;
    uvc_frame_.sequence = 0;
    uvc_frame_.capture_time.tv_sec = 0;
    uvc_frame_.capture_time.tv_usec = 0;

    if (rgb_frame_.data != nullptr) {
        delete [] rgb_frame_.data;
        rgb_frame_.data == nullptr;
    }
    rgb_frame_.data = nullptr;
    rgb_frame_.data_bytes = 0;
    rgb_frame_.actual_bytes = 0;
    rgb_frame_.width = 0;
    rgb_frame_.height = 0;
    rgb_frame_.frame_format = CAMERA_FRAME_FORMAT_RGB;
    rgb_frame_.step = 0;
    rgb_frame_.sequence = 0;
    rgb_frame_.capture_time.tv_sec = 0;
    rgb_frame_.capture_time.tv_usec = 0;
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
            break;
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


void RenderUI::on_uvc_mode_changed()
{
    bool worker_was_active = false;

    if (worker_thread_)
    {
        // Order the worker thread to stop and wait for it to stop.
        worker_.stop_work();
        if (worker_thread_->joinable()) {
            worker_thread_->join();
        }

        worker_was_active = true;
    }

    switch (static_cast<UvcMode>(uvc_mode_selection_.get_active_row_number())) {
        case UVC_MODE_LIBUSB:
            uvc_mode_ = UVC_MODE_LIBUSB;
            break;
        case UVC_MODE_V4L2:
            uvc_mode_ = UVC_MODE_V4L2;
            break;
        case NUM_UVC_MODE:
            break;
        default:
            break;
    }

    if (worker_was_active) {
        StartStream();
    }

    // uvc_mode_selection_box_.pack_start(uvc_mode_selection_frame_, Gtk::PACK_EXPAND_WIDGET);
    // uvc_mode_selection_.append("libusb");
    // uvc_mode_selection_.append("v4l2");
    // uvc_mode_selection_.set_active(0);
    // uvc_mode_selection_frame_.add(uvc_mode_selection_);
    // uvc_mode_selection_.signal_changed().connect(sigc::mem_fun(*this,  &RenderUI::on_uvc_mode_changed));
    // button_box_.pack_start(uvc_mode_selection_box_, Gtk::PACK_EXPAND_WIDGET);
}


void RenderUI::InitializeUI()
{
    button_start_stop_.set_label("open stream");
    button_start_stop_.override_color(Gdk::RGBA("green"), Gtk::STATE_FLAG_NORMAL);

    char buffer[11];
    snprintf(buffer, 11, "fps %5.2f", 0.);
    fps_label_.set_text(buffer);
	
    scaling_ = SCALE_MODE_NEAREST;

    fps_label_.hide();
    button_scaling_.hide();
    event_box_src_.hide();
    event_box_proc_.hide();
}


void RenderUI::ShowUI()
{
    char buffer[11];
    snprintf(buffer, 11, "fps %5.2f", 0.);
    fps_label_.set_text(buffer);

    fps_label_.show();
    button_scaling_.show();
    event_box_src_.show();
    event_box_proc_.show();
}


void RenderUI::StartStream()
{
    // query for installed cameras
    string list_devices = exec("v4l2-ctl --list-devices");
    cout << list_devices << "\n";

    size_t pos = 0;
    string list_devices_iter = list_devices;

    // webcamera is highest priority
    string delimiter = "UVC Camera (046d:0825)";
    cout << "seeking " << delimiter << "\n";
    if (string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
        list_devices_iter = list_devices_iter.substr(pos + delimiter.length());
        delimiter = "/dev/video";
        if (string::npos != (pos = list_devices_iter.find(delimiter, 0))) {
            device_node_string_ = list_devices_iter.substr(pos, delimiter.length() + 1);
            cout << "device_node_string_ : " << device_node_string_ << "\n";

            // extract pid/vid for libusb (lidar camera does not present as part of v4l2-ctl --list-devices response)
            string pid_vid = exec("lsusb");
            string pid_vid_inter = pid_vid;
            string delimiter = "Logitech, Inc. Webcam C270";
            if (string::npos != (pos = pid_vid_inter.find(delimiter, 0))) {
                pid_vid_inter = pid_vid_inter.substr(pos - 10, 9);
                string::size_type sz = 0;
                vid_ = stoll(pid_vid_inter, &sz, 16);
                pid_vid_inter = pid_vid_inter.substr(sz + 1);
                pid_ = stoll(pid_vid_inter, &sz, 16);
                cout << "pid:vid 0x" << hex << pid_ << ":0x" << vid_ << dec << endl;
            }
        }
    }

    if (device_node_string_.size() == 0) {
        cout << "no camera detected" << endl;
        return;
    }

    button_start_stop_.set_label("close stream");
    button_start_stop_.override_color(Gdk::RGBA("red"), Gtk::STATE_FLAG_NORMAL);

    enumerated_width_ = camera_config.width_enumerated;
    enumerated_height_ = camera_config.height;

    actual_width_ = camera_config.width_actual;
    actual_height_ = camera_config.height;

    frame_width_ = camera_config.width_actual;
    frame_height_ = camera_config.height;

    image_scaled_src_.SetAspectRatio(frame_width_, frame_height_);
    image_scaled_proc_.SetAspectRatio(frame_width_, frame_height_);

    ShowUI();

    uvc_frame_.data_bytes = 2 * frame_width_ * frame_height_;
    uvc_frame_.actual_bytes = 2 * frame_width_ * frame_height_;
    uvc_frame_.width = frame_width_;
    uvc_frame_.height = frame_height_;
    uvc_frame_.frame_format = CAMERA_FRAME_FORMAT_YUYV;
    uvc_frame_.step = 0;
    uvc_frame_.sequence = 0;
    uvc_frame_.capture_time.tv_sec = 0;
    uvc_frame_.capture_time.tv_usec = 0;
    uvc_frame_.data = new uint8_t[uvc_frame_.data_bytes];

    rgb_frame_.data_bytes = 3 * frame_width_ * frame_height_;
    rgb_frame_.actual_bytes = 3 * frame_width_ * frame_height_;
    rgb_frame_.width = frame_width_;
    rgb_frame_.height = frame_height_;
    rgb_frame_.frame_format = CAMERA_FRAME_FORMAT_RGB;
    rgb_frame_.step = 0;
    rgb_frame_.sequence = 0;
    rgb_frame_.capture_time.tv_sec = 0;
    rgb_frame_.capture_time.tv_usec = 0;
    rgb_frame_.data = new uint8_t[rgb_frame_.data_bytes];

    // Start a new worker thread.
    worker_thread_ = new thread(
      [this]
      {
        worker_.do_work(this, uvc_mode_, device_node_string_, vid_, pid_, enumerated_width_, enumerated_height_, actual_width_, actual_height_);
      });
}


void RenderUI::StopStream()
{
    // Order the worker thread to stop.
    worker_.stop_work();
    device_node_string_.clear();

    InitializeUI();
}

void RenderUI::on_start_button_clicked()
{
    if (worker_thread_) {
        StopStream();
    } else {
        StartStream();
    }

    // ostringstream ostr;
    // if (uvc_mode_ == UVC_MODE_LIBUSB) {
    //     uvc_mode_ = UVC_MODE_V4L2;
    //     ostr << "UVC_MODE_V4L2\n";
    // } else {
    //     uvc_mode_ = UVC_MODE_LIBUSB;
    //     ostr << "UVC_MODE_LIBUSB\n";
    // }
    // message_ += ostr.str();
    // update_scroll_view_ = true;
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

    switch (scaling_) {
    case SCALE_MODE_NONE:
        scaling_ = SCALE_MODE_NEAREST;
        ostr << "SCALE_MODE_NEAREST\n";
        break;
    case SCALE_MODE_NEAREST:
        scaling_ = SCALE_MODE_BILINEAR;
        ostr << "SCALE_MODE_BILINEAR\n";
        break;
    case SCALE_MODE_BILINEAR:
        scaling_ = SCALE_MODE_NONE;
        ostr << "SCALE_MODE_NONE\n";
        break;
    default:
        break;
    }

    message_ += ostr.str();
    update_scroll_view_ = true;
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
            fps_label_.set_text(buffer);
            fps_label_.show();
            button_scaling_.show();
            accum_index = 0;
            accum = 0;
        }
    }

    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb_src = Gdk::Pixbuf::create_from_data(rgb_frame_.data, Gdk::COLORSPACE_RGB, FALSE, 8, rgb_frame_.width, rgb_frame_.height, rgb_frame_.width*3);
    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb_proc = Gdk::Pixbuf::create_from_data(rgb_frame_.data, Gdk::COLORSPACE_RGB, FALSE, 8, rgb_frame_.width, rgb_frame_.height, rgb_frame_.width*3);

    int image_src_render_width_ =  image_scaled_src_.GetImageWidth();
    int image_src_render_height_ =  image_scaled_src_.GetImageHeight();

    int image_proc_render_width_ =  image_scaled_proc_.GetImageWidth();
    int image_proc_render_height_ =  image_scaled_proc_.GetImageHeight();

    switch (scaling_) {
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

  // TODO mutex
  if (update_scroll_view_) {

    static int num_renders = 0;
    num_renders++;
    auto buffer = text_view_.get_buffer();
    buffer->set_text(message_);

    // Scroll the last inserted line into view. That's somewhat complicated.
    Gtk::TextIter iter = buffer->end();
    iter.set_line_offset(0); // Beginning of last line
    auto mark = buffer->get_mark("last_line");
    buffer->move_mark(mark, iter);
    text_view_.scroll_to(mark);
    // TextView::scroll_to(iter) is not perfect.
    // We do need a TextMark to always get the last line into view.

    if (num_renders > 10) {
      message_.clear();
      num_renders = 0;
    }
    update_scroll_view_ = false;
  }

    mutex_.unlock();

}


void RenderUI::on_quit_button_clicked()
{
    if (worker_thread_)
    {
        // Order the worker thread to stop and wait for it to stop.
        worker_.stop_work();
        if (worker_thread_->joinable())
            worker_thread_->join();
    }
    hide();
}


// notify() is called from ExampleWorker::do_work(). It is executed in the worker
// thread. It triggers a call to on_notification_from_worker_thread(), which is
// executed in the GUI thread.
void RenderUI::Signal()
{
    // drop frame if busy processing prior
    // add queue of at least depth 2 if pipeline desing required
    dispatcher_.emit();
}


// TODO temp locatio for decoding
static inline unsigned char sat(int i) {
  return (unsigned char)( i >= 255 ? 255 : (i < 0 ? 0 : i));
}
#define YUYV2RGB_2(pyuv, prgb) { \
    float r = 1.402f * ((pyuv)[3]-128); \
    float g = -0.34414f * ((pyuv)[1]-128) - 0.71414f * ((pyuv)[3]-128); \
    float b = 1.772f * ((pyuv)[1]-128); \
    (prgb)[0] = sat(pyuv[0] + r); \
    (prgb)[1] = sat(pyuv[0] + g); \
    (prgb)[2] = sat(pyuv[0] + b); \
    (prgb)[3] = sat(pyuv[2] + r); \
    (prgb)[4] = sat(pyuv[2] + g); \
    (prgb)[5] = sat(pyuv[2] + b); \
    }
#define IYUYV2RGB_2(pyuv, prgb) { \
    int r = (22987 * ((pyuv)[3] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[1] - 128) - 11698 * ((pyuv)[3] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[1] - 128)) >> 14; \
    (prgb)[0] = sat(*(pyuv) + r); \
    (prgb)[1] = sat(*(pyuv) + g); \
    (prgb)[2] = sat(*(pyuv) + b); \
    (prgb)[3] = sat((pyuv)[2] + r); \
    (prgb)[4] = sat((pyuv)[2] + g); \
    (prgb)[5] = sat((pyuv)[2] + b); \
    }
#define IYUYV2RGB_8(pyuv, prgb) IYUYV2RGB_4(pyuv, prgb); IYUYV2RGB_4(pyuv + 8, prgb + 12);
#define IYUYV2RGB_4(pyuv, prgb) IYUYV2RGB_2(pyuv, prgb); IYUYV2RGB_2(pyuv + 4, prgb + 6);


void RenderUI::on_notification_from_worker_thread()
{
    if (worker_thread_) {

        if (worker_.has_stopped()) {
            LOG("(worker_thread_ && worker_.has_stopped()) == true\n");

            // fill frames with null
            memset(rgb_frame_.data, 0, rgb_frame_.data_bytes);

            update_widgets();
            // Work is done.
            if (worker_thread_->joinable())
                worker_thread_->join();
            delete worker_thread_;
            worker_thread_ = nullptr;

        } else {

            // extract image planes

            // utilize libuvc yuv->rgb library

            uint8_t *pyuv = uvc_frame_.data;

            int width = rgb_frame_.width;
            int height = rgb_frame_.height;
            uint8_t* prgb = rgb_frame_.data;

            // TODO allocation units for stride
            uint8_t *prgb_end = prgb + (width*3*height);

            while (prgb < prgb_end) {
                IYUYV2RGB_8(pyuv, prgb);

                prgb += 3 * 8;
                pyuv += 2 * 8;
            }
        }
    }

    update_widgets();
}


CameraFrame RenderUI::GetFrame(void)
{
    mutex_.lock();

    return uvc_frame_;
}
