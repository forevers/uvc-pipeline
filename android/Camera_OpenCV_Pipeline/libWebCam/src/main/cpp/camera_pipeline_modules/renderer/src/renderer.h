#ifndef RENDERER_H
#define RENDERER_H

#include <android/native_window.h>
#include <jni.h>
#include <pthread.h>
#include <stddef.h>

#include "webcam_include.h"
#include "renderer_control_ifc.h"


class Renderer : public IRendererControl {

public:

    Renderer(IFrameAccessRegistration* camera_frame_access, int width, int height);

    ~Renderer();

    // IRendererControl methods
    int SetSurface(ANativeWindow *preview_window) override;
    int Start() override;
    int Stop() override;

    inline const bool IsRunning() const;

    int SetPreviewDisplay(ANativeWindow *preview_window, int window_number);

    void LockSurfaceMutex(void) {
        pthread_mutex_lock(&surface_mutex_);
    }

    void UnlockSurfaceMutex(void) {
        pthread_mutex_unlock(&surface_mutex_);
    }

private:

    FrameAccessSharedPtr frame_access_shared_ptr_;

    ANativeWindow* window_;

    volatile bool is_running_;

    // render window size
    int request_width_, request_height_;

    pthread_t thread_;

    void ClearSurface();

    int render_format_;

    pthread_cond_t surface_sync_;
    pthread_mutex_t surface_mutex_;

    static void* ThreadImpl(void *vptr_args);

    CameraError Prepare(void);

    void Run(void);

    void Render(CameraFrame* camera_frame);
};

#endif // RENDERER_H
