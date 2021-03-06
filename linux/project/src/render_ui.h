#ifndef RENDER_UI_H
#define RENDER_UI_H

#include <gtkmm.h>

#include "frame_queue_ifc.h"
#include "image_scaled.h"
#include "uvc_worker.h"
#include "pipeline.h"

// https://github.com/GNOME/gtkmm/blob/master/demos/gtk-demo/example_glarea.cc

// enum {
//   X_AXIS,
//   Y_AXIS,
//   Z_AXIS,

//   N_AXIS
// };

// static const GLfloat vertex_data[] = {
//   0.f,   0.5f,   0.f, 1.f,
//   0.5f, -0.366f, 0.f, 1.f,
//  -0.5f, -0.366f, 0.f, 1.f,
// };

class RenderUI : public Gtk::Window, IFrameQueue
{
public:

    RenderUI();
    virtual ~RenderUI();

    // Called from the worker thread.
    // void notify();

    void enable_scaling(bool enable);

    enum ScaleMode {
        SCALE_MODE_NONE,
        SCALE_MODE_NEAREST,
        SCALE_MODE_BILINEAR,
        NUM_SCALE_MODE,
    };

    // IFrameQueue impl
    void Signal() override;
    CameraFrame GetFrame(void) override;

private:

    CameraFrame uvc_frame_;

    CameraFrame rgb_frame_;
    //unsigned char* g_rgb_buffer = nullptr;

    bool on_key_press_event(GdkEventKey* event) override;

    ScaleMode scaling_;

    // screen and main window sizes
    int screen_width_, screen_height_;
    int window_width_, window_height_;
    int frame_width_, frame_height_;

    // layout
    int interior_border_;
    int widget_margin_;
    bool full_screen_;

    // camera detection
    UvcMode uvc_mode_;
    std::string device_node_string_;
    int vid_, pid_;
    int enumerated_width_, enumerated_height_;
    int actual_width_, actual_height_;

    // Signal handlers.
    bool on_image_src_button_pressed(GdkEventButton* button_event);
    bool on_image_proc_button_pressed(GdkEventButton* button_event);
    void on_start_button_clicked();
    void on_stop_button_clicked();
    void on_shift_down_button_clicked();
    void on_shift_up_button_clicked();
    void on_scaling_button_clicked();
    void on_quit_button_clicked();
    void on_uvc_mode_changed();

    void update_widgets();
    
    void InitializeUI();
    void ShowUI();
    void StartStream();
    void StopStream();

    // Dispatcher handler.
    void on_notification_from_worker_thread();

    // layout
    
    // top level vertical box
    Gtk::Box vertical_box_;
    
    // banner label
    Gtk::Label banner_;
    
    // auto-scaling image windows
    Gtk::EventBox event_box_src_;
    Gtk::EventBox event_box_proc_;
    Gtk::HBox video_box_;
    ImageScaled image_scaled_src_;
    ImageScaled image_scaled_proc_;

    Gtk::GLArea gl_area_;

    // buttons and fps label
    Gtk::ButtonBox button_box_;
    Gtk::Button button_start_stop_;
    Gtk::Button button_scaling_;
    Gtk::Button button_quit_;
    Gtk::Label fps_label_;
    Gtk::VBox uvc_mode_selection_box_;
    Gtk::Frame uvc_mode_selection_frame_;
    Gtk::ComboBoxText uvc_mode_selection_;

    // view for async event information
    bool update_scroll_view_;
    Gtk::ScrolledWindow scrolled_window_;
    Gtk::TextView text_view_;
    Glib::ustring message_;

    Gtk::Image image_src_;
    Gtk::Image image_proc_;

    Glib::Dispatcher dispatcher_;
    ExampleWorker worker_;
    std::thread* worker_thread_;
    mutable std::mutex mutex_;
};

#endif // RENDER_UI_H
