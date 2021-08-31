#pragma once

#include <gtkmm.h>
#include <list>
#include <memory>

#include "camera.h"
#include "camera-frame-queue-client-ifc.h"
#include "camera-frame-queue-server-ifc.h"
#include "image-scaled.h"
#include "sync-log.h"
#include "worker.h"


class RenderUI : public Gtk::Window, ICameraFrameQueueClient, ICameraFrameQueueServer
{
public:

    RenderUI();
    virtual ~RenderUI();

    // void enable_scaling(bool enable);

    enum ScaleMode {
        SCALE_MODE_NONE,
        SCALE_MODE_NEAREST,
        SCALE_MODE_BILINEAR,
        NUM_SCALE_MODE,
    };

    /* ICameraFrameQueueClient impl */
    virtual bool GetFrame(CameraFrame** frame) override;
    virtual void ReturnFrame(CameraFrame* frame) override;

    /* ICameraFrameQueueServer impl */
    void Init(size_t num_elems, size_t data_bytes) override;
    int AddFrameToQueue(CameraFrame *frame) override;
    CameraFrame* GetPoolFrame() override;
    std::shared_ptr<ICameraFrameQueueClient> ClientCameraQueueIfc() override;

private:

    CameraFrame uvc_frame_;

    CameraFrame* rgb_frame_;

    /* escape key toggle full screen mode */
    bool on_key_press_event(GdkEventKey* event) override;

    /* return vector of camera characteristics for:
        "cameras", "formats", "res rates", "rates"
    */
    std::vector<std::string> CamaraParameterSelectionList(std::string selection_type);

    /* video scaling */
    ScaleMode scaling_;

    /* screen and main window sizes */
    int screen_width_, screen_height_;
    int window_width_, window_height_;
    // int frame_width_, frame_height_;

    /* layout */
    int interior_border_;
    int widget_margin_;
    bool full_screen_;

    /* camera detection */
    std::string device_node_string_;
    int vid_, pid_;
    int enumerated_width_, enumerated_height_;
    int actual_width_, actual_height_;

    /* Signal handlers. */
    bool on_image_src_button_pressed(GdkEventButton* button_event);
    bool on_image_proc_button_pressed(GdkEventButton* button_event);
    void on_open_camera_clicked();
    void on_combo_box_camera_index_changed();
    void on_combo_box_format_index_changed();
    void on_combo_box_resolution_index_changed();
    void on_combo_box_rate_index_changed();
    void on_full_screen_clicked();
    void on_start_button_clicked();
    void on_stop_button_clicked();
    void on_scaling_button_clicked();
    void on_quit_button_clicked();

    void update_widgets();
    
    void InitializeUI();
    void ShowUI();

    /* dispatcher handler */
    void on_notification_from_worker_thread();
    /* render frame */
    void RenderFrame();

    /* layout */
    
    /* top level vertical box */
    Gtk::Box vertical_box_;
    
    /* banner label */
    Gtk::Label banner_;
    
    /* auto-scaling image windows */
    Gtk::EventBox event_box_src_;
    Gtk::EventBox event_box_proc_;
    Gtk::HBox video_box_;
    ImageScaled image_scaled_src_;
    ImageScaled image_scaled_proc_;

    Gtk::GLArea gl_area_;

    /* buttons and fps label */
    Gtk::ButtonBox button_box_;
    Gtk::Button button_open_close_camera_;
    Gtk::ComboBoxText combo_box_camera_;
    Gtk::ComboBoxText combo_box_format_;
    Gtk::ComboBoxText combo_box_resolution_;
    Gtk::ComboBoxText combo_box_rate_;
    Gtk::Button button_full_screen_;
    Gtk::Button button_start_stop_;
    Gtk::Button button_scaling_;
    Gtk::Button button_quit_;
    Gtk::Label fps_label_;

    /* view for async event information */
    bool update_scroll_view_;
    Gtk::ScrolledWindow scrolled_window_;
    Gtk::TextView text_view_;
    Glib::ustring message_;

    Gtk::Image image_src_;
    Gtk::Image image_proc_;

    std::shared_ptr<Camera> camera_;

    rapidjson::Document camera_modes_doc_;

    bool camera_types_validated_;
    /* number of available cameras */
    size_t camera_dev_nodes_size_;
    std::list<std::string> camera_dev_nodes_;
    /* index and name of selected camera */
    int camera_dev_node_index_;
    std::string camera_dev_node_;
    /* index and name of selected format */
    int camera_format_index_;
    // std::string camera_format_;
    /* index and name of selected resolution */
    int camera_resolution_index_;
    std::string camera_resolution_;
     /* index and name of selected resolution */
    int camera_rate_index_;
    std::string camera_rate_;

    /* selected camera configuration */
    CameraConfig camera_config_;

    /* rendering worker thread */
    Glib::Dispatcher dispatcher_;
    Worker worker_;
    std::thread* worker_thread_;
    mutable std::mutex mutex_;

    bool opened_;
    bool running_;

    /* console logger */
    std::shared_ptr<SyncLog> synclog_;

    void Blank();
};
