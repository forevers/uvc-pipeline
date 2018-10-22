#ifndef EXAMPLEWORKER_H
#define EXAMPLEWORKER_H

#include <gtkmm.h>
#include <thread>
#include <mutex>

#include "frame_access_ifc.h"
#include "pipeline.h"

class RenderUI;

class ExampleWorker
{
public:
    ExampleWorker();

    // Thread function.
    void do_work(IFrameAccess* frame_access_ifc, UvcMode uvc_mode, std::string device_node_string, int vid, int pid, int enumerated_width, int enumerated_height, int actual_width, int actual_height);

    void get_data(double* fraction_done, Glib::ustring* message) const;
    void stop_work();
    bool has_stopped() const;

private:
    // Synchronizes access to member data.
    mutable std::mutex m_Mutex;
    // signal frame availability
    // std::condition_variable m_ConditionalFrameAvailable;

    // Data used by both GUI thread and worker thread.
    bool m_shall_stop;
    bool m_has_stopped;
    Glib::ustring m_message;
};

#endif // EXAMPLEWORKER_H
