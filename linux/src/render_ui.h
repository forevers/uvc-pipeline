#ifndef RENDER_UI_H
#define RENDER_UI_H

#include <gtkmm.h>

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
    ScaleMode m_scaling;

    // camera detection
    UvcCamera m_UvcCamera;
    UvcMode m_UvcMode;
    std::string m_device_node_string;
    int m_vid, m_pid;
    int m_enumerated_width, m_enumerated_height;
    int m_actual_width, m_actual_height;

    // Signal handlers.
    void on_start_button_clicked();
    void on_stop_button_clicked();
    void on_shift_down_button_clicked();
    void on_shift_up_button_clicked();
    void on_scaling_button_clicked();
    void on_quit_button_clicked();

    void update_widgets();

    // Dispatcher handler.
    void on_notification_from_worker_thread();

    // Member data.
    Gtk::Box m_VBox;
    Gtk::ButtonBox m_ButtonBox;
    Gtk::Button m_ButtonStartStop;
    Gtk::Button m_ButtonShiftDown;
    Gtk::Button m_ButtonShiftUp;
    Gtk::Button m_ButtonScaling;
    Gtk::Button m_ButtonQuit;
    Gtk::Label m_FpsLabel;
    Gtk::ProgressBar m_ProgressBar;
    Gtk::Label m_Banner;

    bool m_update_scroll_view;
    Gtk::ScrolledWindow m_ScrolledWindow;
    Gtk::TextView m_TextView;
    Glib::ustring m_message;

    Gtk::Image m_Image;
    Gtk::Image m_ImageScaled;

    Glib::Dispatcher m_Dispatcher;
    ExampleWorker m_Worker;
    std::thread* m_WorkerThread;
};

#endif // RENDER_UI_H
