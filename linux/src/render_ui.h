#ifndef RENDER_UI_H
#define RENDER_UI_H

#include <gtkmm.h>

#include "image_scaled.h"
//#include "image_scaled_eventbox.h"
//#include "image_scaled_clickable.h"
#include "uvc_worker.h"
#include "pipeline.h"

enum ScaleMode {
    SCALE_MODE_NONE,
    SCALE_MODE_NEAREST,
    SCALE_MODE_BILINEAR,
    NUM_SCALE_MODE,
};


// TODO remove cyclic once interfaces are defined
//class ExampleWorker;

class RenderUI : public Gtk::Window
{
public:

    RenderUI();

    // Called from the worker thread.
    void notify();

    void enable_scaling(bool enable);

private:

    bool on_key_press_event(GdkEventKey* event) override;

    ScaleMode m_scaling;

    // screend and main window sizes
    int screen_width_, screen_height_;
    int window_width_, window_height_;
    
    // layout
    int interior_border_;
    int widget_margin_;
    bool full_screen_;

    // camera detection
    UvcCamera m_UvcCamera;
    UvcMode m_UvcMode;
    std::string m_device_node_string;
    int m_vid, m_pid;
    int m_enumerated_width, m_enumerated_height;
    int m_actual_width, m_actual_height;

    // Signal handlers.
    bool on_image_src_button_pressed(GdkEventButton *button_event);
    bool on_image_proc_button_pressed(GdkEventButton *button_event);
    void on_start_button_clicked();
    void on_stop_button_clicked();
    void on_shift_down_button_clicked();
    void on_shift_up_button_clicked();
    void on_scaling_button_clicked();
    void on_quit_button_clicked();

    void update_widgets();

    // Dispatcher handler.
    void on_notification_from_worker_thread();

    // layout
    // top level vertical box
    Gtk::Box m_VBox;
    // banner label
    Gtk::Label m_Banner;
    // auto-scaling image windows
    Gtk::EventBox event_box_src_;
    Gtk::EventBox event_box_proc_;
    Gtk::HBox video_box_;
    ImageScaled image_scaled_src_;
    ImageScaled image_scaled_proc_;
    //ImageScaled2 image_scaled_src_;
    //ImageScaled2 image_scaled_proc_;
    //ImageScaledClickable image_scaled_src_;
    //ImageScaledClickable image_scaled_proc_;
    // buttons and fps label
    Gtk::ButtonBox m_ButtonBox;
    Gtk::Button m_ButtonStartStop;
    Gtk::Button m_ButtonShiftDown;
    Gtk::Button m_ButtonShiftUp;
    Gtk::Button m_ButtonScaling;
    Gtk::Button m_ButtonQuit;
    Gtk::Label m_FpsLabel;
    //Gtk::ProgressBar m_ProgressBar;

    bool m_update_scroll_view;
    Gtk::ScrolledWindow m_ScrolledWindow;

    Gtk::TextView m_TextView;
    Glib::ustring m_message;

    Gtk::Image image_src_;
    Gtk::Image image_proc_;

    Glib::Dispatcher m_Dispatcher;
    ExampleWorker m_Worker;
    std::thread* m_WorkerThread;
};

#endif // RENDER_UI_H
