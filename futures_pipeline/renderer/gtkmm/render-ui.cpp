#include "exec-shell.h"
#include "render-ui.h"

#include <iostream>
#include <gtkmm.h>

using namespace std;


void set_widget_margin(Gtk::Widget& widget, int margin)
{
    widget.set_margin_left(margin);
    widget.set_margin_right(margin);
    widget.set_margin_top(margin);
    widget.set_margin_bottom(margin);
}


RenderUI::RenderUI() :
    rgb_frame_mutex_{},
    rgb_frame_{nullptr},
    scaling_{SCALE_MODE_NEAREST},
    screen_width_{0}, screen_height_{0},
    window_width_{0}, window_height_{0},
    interior_border_{5},
    widget_margin_{5},
    full_screen_{false},
    vid_{0}, pid_{0},
    enumerated_width_{0}, enumerated_height_{0},
    actual_width_{0}, actual_height_{0},
    vertical_box_{Gtk::ORIENTATION_VERTICAL, 5},
    banner_{"UVC Video / OpenCV Demo"},
    button_box_{Gtk::ORIENTATION_HORIZONTAL},
    button_open_close_camera_{"open camera"},
    //combo_box_camera_{"placeholder"},
    //combo_box_format_{"placeholder"},
    button_full_screen_{"full screen"},
    button_start_stop_{"start stream"},
    button_scaling_{"Toggle Scaling"},
    button_quit_{"_Quit", /* mnemonic= */ true},
    fps_label_{"fps xy.xx", true},
    update_scroll_view_{true},
    scrolled_window_{},
    text_view_{},
    message_{},
    camera_{nullptr},
    camera_types_validated_{false},
    camera_dev_nodes_size_{0},
    camera_dev_node_index_{0},
    camera_dev_node_{"none"},
    camera_format_index_{0},
    camera_resolution_index_{0},
    camera_resolution_{"none"},
    camera_rate_index_{0},
    camera_rate_{"none"},
    camera_config_{"none", "none", -1, -1, -1, -1, -1, -1, {0, 0}},
    dispatcher_{},
    worker_thread_{nullptr},
    render_mutex_{},
    opened_{false},
    running_{false},
    synclog_{SyncLog::GetLog()}
{
    set_title("Camera OpenCV Demo");

    set_border_width(interior_border_);

    /* display screen resolution */
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

    /* banner label */
    vertical_box_.pack_start(banner_, Gtk::PACK_SHRINK);

    /* auto-scaled image views */
    image_scaled_src_.add(image_src_);
    image_scaled_proc_.add(image_proc_);
    event_box_src_.add(image_scaled_src_);
    event_box_proc_.add(image_scaled_proc_);
    video_box_.pack_start(event_box_src_, Gtk::PACK_EXPAND_WIDGET);
    video_box_.pack_start(event_box_proc_, Gtk::PACK_EXPAND_WIDGET);
    vertical_box_.pack_start(video_box_);

    set_widget_margin(image_scaled_src_, widget_margin_);
    set_widget_margin(image_scaled_proc_, widget_margin_);

    /* Add the TextView, inside a ScrolledWindow. */
    scrolled_window_.add(text_view_);

    /* Only show the scrollbars when they are necessary. */
    scrolled_window_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    // TODO vertical_box_.pack_start(scrolled_window_);
    vertical_box_.pack_start(scrolled_window_, Gtk::PACK_SHRINK);

    text_view_.set_editable(false);

    /* Add the buttons to the ButtonBox. */
    vertical_box_.pack_start(button_box_, Gtk::PACK_SHRINK);

    button_box_.pack_start(button_scaling_, Gtk::PACK_SHRINK);
    button_box_.pack_start(fps_label_, Gtk::PACK_SHRINK);
    button_box_.pack_start(button_open_close_camera_, Gtk::PACK_SHRINK);
    button_box_.pack_start(button_start_stop_, Gtk::PACK_SHRINK);
    button_box_.pack_start(combo_box_camera_, Gtk::PACK_SHRINK);
    button_box_.pack_start(combo_box_format_, Gtk::PACK_SHRINK);
    button_box_.pack_start(combo_box_resolution_, Gtk::PACK_SHRINK);
    button_box_.pack_start(combo_box_rate_, Gtk::PACK_SHRINK);
    button_box_.pack_start(button_full_screen_, Gtk::PACK_SHRINK);
    button_box_.pack_start(button_quit_, Gtk::PACK_SHRINK);
    button_box_.set_border_width(5);
    button_box_.set_spacing(5);
    button_box_.set_layout(Gtk::BUTTONBOX_END);

    set_widget_margin(button_scaling_, widget_margin_);
    set_widget_margin(fps_label_, widget_margin_);
    set_widget_margin(button_open_close_camera_, widget_margin_);
    set_widget_margin(button_full_screen_, widget_margin_);
    set_widget_margin(button_start_stop_, widget_margin_);
    set_widget_margin(button_quit_, widget_margin_);

    button_start_stop_.override_color(Gdk::RGBA("green"), Gtk::STATE_FLAG_NORMAL);

    /* image view mouse button click handler */
    event_box_src_.add_events(Gdk::BUTTON_PRESS_MASK);
    event_box_src_.signal_button_press_event().connect(sigc::mem_fun(*this, &RenderUI::on_image_src_button_pressed));
    event_box_proc_.add_events(Gdk::BUTTON_PRESS_MASK);
    event_box_proc_.signal_button_press_event().connect(sigc::mem_fun(*this, &RenderUI::on_image_proc_button_pressed));

    /* Connect the signal handlers to the buttons. */
    button_open_close_camera_.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_open_camera_clicked));
    combo_box_camera_.signal_changed().connect(sigc::mem_fun(*this, &RenderUI::on_combo_box_camera_index_changed));
    combo_box_format_.signal_changed().connect(sigc::mem_fun(*this, &RenderUI::on_combo_box_format_index_changed));
    combo_box_resolution_.signal_changed().connect(sigc::mem_fun(*this, &RenderUI::on_combo_box_resolution_index_changed));
    combo_box_rate_.signal_changed().connect(sigc::mem_fun(*this, &RenderUI::on_combo_box_rate_index_changed));
    button_full_screen_.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_full_screen_clicked));
    button_start_stop_.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_start_button_clicked));
    button_scaling_.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_scaling_button_clicked));
    button_quit_.signal_clicked().connect(sigc::mem_fun(*this, &RenderUI::on_quit_button_clicked));

    /* Connect the handler to the dispatcher. */
    dispatcher_.connect(sigc::mem_fun(*this, &RenderUI::on_notification_from_worker_thread));

    /* Create a text buffer mark for use in update_widgets(). */
    auto buffer = text_view_.get_buffer();
    buffer->create_mark("last_line", buffer->end(), /* left_gravity= */ true);

    show_all_children();

    button_start_stop_.hide();
    combo_box_format_.hide();
    combo_box_resolution_.hide();
    combo_box_rate_.hide();
    fps_label_.hide();
    button_scaling_.hide();

    camera_ = make_shared<Camera>();

    /* populate camera capabilities */
    RefreshCameraCapabilities();

    /* present inital camera configuration options */
    auto cameras = CamaraParameterSelectionList("cameras");
    for (auto camera : cameras)
        combo_box_camera_.append(camera);
    combo_box_camera_.set_active_text(cameras[0]);
}


RenderUI::~RenderUI()
{
    camera_ = nullptr;
}


void RenderUI::RefreshCameraCapabilities()
{
    /* present camera capabilities */
    /* validate schema */
    {
        rapidjson::Document document;
        document.Parse(g_schema);

        rapidjson::SchemaDocument schemaDocument(document);
        rapidjson::SchemaValidator validator(schemaDocument);

        /* parse JSON string */
        string camera_modes = camera_->GetSupportedVideoModes();

        camera_modes_doc_.Parse(camera_modes.c_str());

        if (camera_modes_doc_.Accept(validator)) {
            camera_types_validated_ = true;
#if 0
            std::vector<std::string> cameraStringList;

            cout<<"schema validated"<<endl;
            size_t align = 0;
            camera_dev_nodes_size_ = camera_modes_doc_["cameras"].Size();
            cout<<string(align, ' ')<<"num cameras: "<<camera_dev_nodes_size_<<endl;
            for (auto& camera : camera_modes_doc_["cameras"].GetArray()) {
                align = 2;
                cout<<string(align, ' ')<<camera["device node"].GetString()<<endl;
                cameraStringList.append(camera["device node"].GetString());
                camera_formats_size_ = camera["formats"].Size();
                cout<<string(align, ' ')<<"num formats: "<<camera_formats_size_<<endl;
                for (auto& format : camera["formats"].GetArray()) {
                    align += 4;
                    cout<<string(align, ' ')<<format["index"].GetInt()<<endl;
                    cout<<string(align, ' ')<<format["type"].GetString()<<endl;
                    cout<<string(align, ' ')<<format["format"].GetString()<<endl;
                    cout<<string(align, ' ')<<format["name"].GetString()<<endl;
                    cout<<string(align, ' ')<<"num res rates: "<<format["res rates"].Size()<<endl;
                    for (auto& res_rate : format["res rates"].GetArray()) {
                        align = 6;
                        cout<<string(align, ' ')<<res_rate["resolution"].GetString()<<endl;
                        cout<<string(align, ' ')<<"num rates: "<<res_rate["rates"].Size()<<endl;
                        for (auto& rate : res_rate["rates"].GetArray()) {
                            align = 8;
                            cout<<string(align, ' ')<<rate.GetString()<<endl;
                        }
                    }
                }
            }
#endif
         } else {
             cout<<"schema invalidated"<<endl;
         }
     }
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


bool RenderUI::on_image_src_button_pressed(__attribute__((unused)) GdkEventButton* button_event)
{
    cout << "on_image_src_button_pressed()" << endl;

    if (event_box_proc_.is_visible()) {
        event_box_proc_.hide();
    } else {
        event_box_proc_.show();
    }

    return false;
}


bool RenderUI::on_image_proc_button_pressed(__attribute__((unused)) GdkEventButton* button_event)
{
    cout << "on_image_proc_button_pressed()" << endl;

    if (event_box_src_.is_visible()) {
        event_box_src_.hide();
    } else {
        event_box_src_.show();
    }

    return false;
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


vector<string> RenderUI::CamaraParameterSelectionList(string selection_type)
{
    vector<string> camera_selection_list;

    if (selection_type == "cameras") {
        for (auto& camera : camera_modes_doc_["cameras"].GetArray()) {
            camera_selection_list.emplace_back(camera["device node"].GetString());
        }
    } else if (selection_type == "formats") {
        for (auto& format : camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"].GetArray()) {
            camera_selection_list.emplace_back(format["name"].GetString());
        }
    } else if (selection_type == "res rates") {
        for (auto& resolution : camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"][camera_format_index_]["res rates"].GetArray()) {
            camera_selection_list.emplace_back(resolution["resolution"].GetString());
        }
    } else if (selection_type == "rates") {
        for (auto& rate : camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"][camera_format_index_]["res rates"][camera_resolution_index_]["rates"].GetArray()) {
            camera_selection_list.emplace_back(rate.GetString());
        }
    }

    return camera_selection_list;
}


void RenderUI::on_open_camera_clicked()
{
    synclog_->LogV(FFL,"entry");

    if (opened_) {

        opened_ = false;

        /* depopulate and hide camera configuration and control */
        button_start_stop_.hide();
        combo_box_camera_.set_sensitive(true);
        combo_box_format_.hide();
        combo_box_resolution_.hide();
        combo_box_rate_.hide();
        button_open_close_camera_.set_label("open camera");

    } else {

        opened_ = true;

        /* render camera configuration and control */
        button_open_close_camera_.set_label("close camera");
        button_start_stop_.show();
        combo_box_camera_.set_sensitive(false);
        combo_box_format_.set_entry_text_column(0);
        combo_box_format_.show();
        combo_box_resolution_.set_entry_text_column(0);
        combo_box_resolution_.show();
        combo_box_rate_.set_entry_text_column(0);
        combo_box_rate_.show();

    }

    synclog_->LogV(FFL,"exit");
}


void RenderUI::on_combo_box_camera_index_changed()
{
    synclog_->LogV(FFL,"entry");

    /* update selected camera */
    if (-1 != (camera_dev_node_index_ = combo_box_camera_.get_active_row_number())) {

        camera_dev_node_ = combo_box_camera_.get_active_text();
        camera_config_.camera_dev_node_ = camera_dev_node_;

        /* reset formats */
        combo_box_format_.remove_all();
        auto format_string_vector = CamaraParameterSelectionList("formats");
        for (auto format : format_string_vector)
            combo_box_format_.append(format);
        combo_box_format_.set_active_text(format_string_vector[0]);

    } else {
        /* clear all sub formats */
        combo_box_format_.remove_all();
    }

    synclog_->LogV(FFL,"camera_dev_node_index_: %d", camera_dev_node_index_);

    synclog_->LogV(FFL,"exit");
}


void RenderUI::on_combo_box_format_index_changed()
{
    synclog_->LogV(FFL,"entry");

    /* update selected format */
    if (-1 != (camera_format_index_ = combo_box_format_.get_active_row_number())) {

        // camera_format_ = combo_box_format_.get_active_text();
        camera_config_.format_ = combo_box_format_.get_active_text();
        synclog_->LogV(FFL,"camera_config_.format_: %s", camera_config_.format_ );

        /* update dependent formats */
        combo_box_resolution_.remove_all();
        auto resolution_string_vector = CamaraParameterSelectionList("res rates");
        cout<<"resolution_string_vector: "<<endl;
        for (std::vector<string>::const_iterator i = resolution_string_vector.begin(); i != resolution_string_vector.end(); ++i)
            std::cout << *i << ' ';
        cout<<endl;

        for (auto resolution : resolution_string_vector)
            combo_box_resolution_.append(resolution);
        combo_box_resolution_.set_active_text(resolution_string_vector[0]);

    } else {
        /* clear all sub formats */
        combo_box_resolution_.remove_all();
    }

    cout<<"camera_format_index_:"<<camera_format_index_<<endl;

    synclog_->LogV(FFL,"exit");
}


void RenderUI::on_combo_box_resolution_index_changed()
{
    synclog_->LogV(FFL,"entry");

    /* update selected resolution */
    if (-1 != (camera_resolution_index_ = combo_box_resolution_.get_active_row_number())) {

        auto resolution_ = combo_box_resolution_.get_active_text();

        /* extract w/h from resolution string */
        int start = 0;
        string delim = "x";
        int end = resolution_.find(delim);
        int width = stoi(resolution_.substr(start, end - start));
        start = end + delim.size();
        end = resolution_.find(delim, start);
        int height = stoi(resolution_.substr(start, end - start));
        camera_config_.width_enumerated_ = width;
        camera_config_.width_actual_ = width;
        camera_config_.height_ = height;

        /* update dependent formats */
        combo_box_rate_.remove_all();
        auto rate_string_vector = CamaraParameterSelectionList("rates");
        // synclog_->Log(rate_string_vector);

        for (auto rate : rate_string_vector)
            combo_box_rate_.append(rate);
        combo_box_rate_.set_active_text(rate_string_vector[0]);

    } else {

        /* clear all sub formats */
        combo_box_rate_.remove_all();
    }

    cout<<"camera_resolution_index_:"<<camera_resolution_index_<<endl;

    synclog_->LogV(FFL,"exit");
}


void RenderUI::on_combo_box_rate_index_changed()
{
    synclog_->LogV(FFL,"entry");

    if (-1 != (camera_rate_index_ = combo_box_rate_.get_active_row_number())) {

        auto rate = combo_box_rate_.get_active_text();

        // TODO format error handling
        // camera_config_.time_per_frame_.denominator = camera_rate_.left(camera_rate_.size() - 4).toFloat();
        // camera_config_.time_per_frame_.numerator = 1;
    }

    cout<<"camera_rate_index_:"<<camera_rate_index_<<endl;

    synclog_->LogV(FFL,"exit");
}


void RenderUI::on_full_screen_clicked()
{
    synclog_->LogV(FFL,"entry");
    synclog_->LogV(FFL,"exit");
}


// TODO follow Worker model used in qt ?
void RenderUI::on_start_button_clicked()
{
    synclog_->LogV(FFL,"entry");

    if (opened_) {

        // TODO replace running_ with if (worker_thread_) {
        if (running_) {

            cout<<"stop streaming"<<endl;
            worker_.stop_work();
            running_ = false;

            // TODO enable this clear memset(rgb_frame_->data, 0, rgb_frame_->data_bytes);

            button_open_close_camera_.set_sensitive(true);
            button_start_stop_.set_label("start stream");

        } else {

            cout<<"start streaming"<<endl;

            /* image view scaling */
            image_scaled_src_.SetAspectRatio(camera_config_.width_actual_, camera_config_.height_);
            image_scaled_proc_.SetAspectRatio(camera_config_.width_actual_, camera_config_.height_);

            camera_->Start(camera_config_);
            /* Start a new worker thread. */
            worker_thread_ = new thread(
                [this]
                {
                    worker_.do_work(camera_, this, this);
                });

            running_ = true;
            button_open_close_camera_.set_sensitive(false);
            button_start_stop_.set_label("stop stream");
        }

    } else {
        cout << "camera not opened" << endl;
    }

    // ostringstream ostr;
    // ostr<<"start/stop";
    // message_ += ostr.str();
    // update_scroll_view_ = true;

    synclog_->LogV(FFL,"exit");
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


void nop(__attribute__((unused)) const guint* data) {

}


void RenderUI::update_widgets()
{
    Glib::ustring message_from_worker_thread;

    /* rough estimate for fps */
    struct timespec start;
    static timeval last;
    double fps;

    synclog_->LogV(FFL,"entry");

    /* cache last render time */
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

    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb_src = Gdk::Pixbuf::create_from_data(rgb_frame_->data, Gdk::COLORSPACE_RGB, FALSE, 8, rgb_frame_->width, rgb_frame_->height, rgb_frame_->width*3);
    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb_proc = Gdk::Pixbuf::create_from_data(rgb_frame_->data, Gdk::COLORSPACE_RGB, FALSE, 8, rgb_frame_->width, rgb_frame_->height, rgb_frame_->width*3);

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

    /* explicit render of image residing in event boxed scrolled window */
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

    synclog_->LogV(FFL,"exit");

    render_mutex_.unlock();
}


void RenderUI::on_quit_button_clicked()
{
    if (worker_thread_)
    {
        // Order the worker thread to stop and wait for it to stop.
        // worker_.stop_work();
        // if (worker_thread_->joinable())
        //     worker_thread_->join();
    }
    hide();
}


void RenderUI::RenderFrame()
{
    // synclog_->LogV(FFL,"entry");

    /* single depth ui frame buffering */
    std::lock_guard<std::mutex> lock(render_mutex_);

    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb_src;
    {
    std::lock_guard<std::mutex> lock(rgb_frame_mutex_);
    pixbuf_rgb_src = Gdk::Pixbuf::create_from_data(rgb_frame_->data, Gdk::COLORSPACE_RGB, FALSE, 8, rgb_frame_->width, rgb_frame_->height, rgb_frame_->width*3);
    }

    int image_src_render_width =  image_scaled_src_.GetImageWidth();
    int image_src_render_height =  image_scaled_src_.GetImageHeight();
    switch (scaling_) {
        case SCALE_MODE_NEAREST:
            image_src_.set(pixbuf_rgb_src->scale_simple(image_src_render_width, image_src_render_height, Gdk::INTERP_NEAREST));
            // image_proc_.set(pixbuf_rgb_proc->scale_simple(image_proc_render_width_, image_proc_render_height_ , Gdk::INTERP_NEAREST));
            break;
        case SCALE_MODE_BILINEAR:
            image_src_.set(pixbuf_rgb_src->scale_simple(image_src_render_width, image_src_render_height, Gdk::INTERP_BILINEAR));
            // image_proc_.set(pixbuf_rgb_proc->scale_simple(image_proc_render_width_, image_proc_render_height_ , Gdk::INTERP_BILINEAR));
            break;
        case SCALE_MODE_NONE:
        default:
            image_src_.set(pixbuf_rgb_src);
            // image_proc_.set(pixbuf_rgb_proc);
            break;
    }

    // explicit render of image residing in event boxed scrolled window
    event_box_src_.queue_draw();
    event_box_proc_.queue_draw();

    /* single depth ui frame buffering */
    render_busy_ = false;
    render_cv_.notify_all();

    // synclog_->LogV(FFL,"exit");
}


void RenderUI::on_notification_from_worker_thread()
{
    // synclog_->LogV(FFL,"entry");

    // TODO check still required?
    //if (worker_thread_) {

        // TODO replace with null frame signal
        if (worker_.has_stopped()) {

            synclog_->LogV(FFL,"worker_.has_stopped()");
            Blank();

            //     update_widgets();
            // Work is done.
            synclog_->LogV(FFL,"pre join()");
            if (worker_thread_->joinable())
                worker_thread_->join();
            synclog_->LogV(FFL,"post join()");
            delete worker_thread_;
            worker_thread_ = nullptr;
            render_busy_ = false;



        } else {

            /* render frame */
            RenderFrame();
            {
                std::lock_guard<std::mutex> lock(rgb_frame_mutex_);
                camera_->ReturnFrame(rgb_frame_);
                rgb_frame_ = nullptr;
            }
           /* TODO Add a fixed frame delay to the queue feeding the renderer (raw v4l2 or openCV processed)
                ... i.e. block the queue pull until N (say 2) frames have been loaded
                ... will add fixed latency to render but will address jitter issues on nominally or async loaded platform
                ... do while this loop if source queue depth > N
            */
        }
    //}

    // update_widgets();
    // synclog_->LogV(FFL,"exit");
}


bool RenderUI::GetFrame(CameraFrame** frame)
{
    return camera_->GetFrame(frame);
}


void RenderUI::ReturnFrame(__attribute__((unused)) CameraFrame* frame)
{

}


/* ICameraFrameQueueServer impl */

void RenderUI::Init(__attribute__((unused)) size_t num_elems, __attribute__((unused)) size_t data_bytes)
{
    // TODO create and initialize queue
};


int RenderUI::AddFrameToQueue(CameraFrame* frame)
{
    // synclog_->LogV(FFL,"entry");

    // TODO impl render queue
    unique_lock<std::mutex> lock(render_mutex_);
    while (render_busy_) render_cv_.wait(lock);
    render_busy_ = true;

    rgb_frame_ = frame;

    /* emit triggers a call to on_notification_from_worker_thread() */

    /* https://developer-old.gnome.org/glibmm/stable/classGlib_1_1Dispatcher.html
    usage rules:
        Only one thread may connect to the signal and receive notification, but multiple senders are allowed even without locking.
        The GLib main loop must run in the receiving thread (this will be the GUI thread usually).
        The Dispatcher object must be instantiated by the receiver thread.
        The Dispatcher object should be instantiated before creating any of the sender threads, if you want to avoid extra locking.
        The Dispatcher object must be deleted by the receiver thread.
        All Dispatcher objects instantiated by the same receiver thread must use the same main context.
    */
    dispatcher_.emit();

    // synclog_->LogV(FFL,"exit");

    // TODO queue overflow indication
    return 0;
}


CameraFrame* RenderUI::GetPoolFrame()
{
    // TODO AddFrameToQueue calls this method

    // TODO if source queue > 2 and null frame not in first 2 queue frame return frame else block() (else requires a signal all)
    return nullptr;
}


std::shared_ptr<ICameraFrameQueueClient> RenderUI::ClientCameraQueueIfc()
{
    // TODO is required still ?
    return nullptr;
}


void RenderUI::Blank()
{
    synclog_->LogV(FFL,"entry");

    int COLOR_COMPONENTS = 3;

    /* create and render pixmap */
    uint8_t blank[COLOR_COMPONENTS*camera_config_.width_actual_*camera_config_.height_];
    memset(blank, 0x00, sizeof(blank));

    Glib::RefPtr<Gdk::Pixbuf> pixbuf_rgb_src = Gdk::Pixbuf::create_from_data(blank, Gdk::COLORSPACE_RGB, FALSE, 8, camera_config_.width_actual_, camera_config_.height_, camera_config_.width_actual_*3);
    int image_src_render_width = image_scaled_src_.GetImageWidth();
    int image_src_render_height = image_scaled_src_.GetImageHeight();

    // TODO combine with RenderFrame()
    switch (scaling_) {
        case SCALE_MODE_NEAREST:
            image_src_.set(pixbuf_rgb_src->scale_simple(image_src_render_width, image_src_render_height, Gdk::INTERP_NEAREST));
            // image_proc_.set(pixbuf_rgb_proc->scale_simple(image_proc_render_width_, image_proc_render_height_ , Gdk::INTERP_NEAREST));
            break;
        case SCALE_MODE_BILINEAR:
            image_src_.set(pixbuf_rgb_src->scale_simple(image_src_render_width, image_src_render_height, Gdk::INTERP_BILINEAR));
            // image_proc_.set(pixbuf_rgb_proc->scale_simple(image_proc_render_width_, image_proc_render_height_ , Gdk::INTERP_BILINEAR));
            break;
        case SCALE_MODE_NONE:
        default:
            image_src_.set(pixbuf_rgb_src);
            // image_proc_.set(pixbuf_rgb_proc);
            break;
    }

    // explicit render of image residing in event boxed scrolled window
    event_box_src_.queue_draw();
    event_box_proc_.queue_draw();

    synclog_->LogV(FFL,"exit");
}
