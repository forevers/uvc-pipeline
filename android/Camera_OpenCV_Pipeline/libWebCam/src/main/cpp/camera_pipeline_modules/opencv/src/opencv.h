#ifndef OPENCV_H
#define OPENCV_H

#include <memory>
#include <pthread.h>
#include <stddef.h>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/opencv.hpp>

#include "webcam_frame_access_ifc.h"
#include "frame_access_registration_ifc.h"
#include "opencv_control_ifc.h"

// centroid ball tracking
typedef enum {
    BALL_TRACKER_INIT,
    BALL_TRACKER_TRACK,
} BallTrackerState;


class OpenCV : public IOpenCVControl, public IFrameAccessRegistration, public ICameraFrameAccess {

public:

    OpenCV(IFrameAccessRegistration* frame_access, int width, int height, std::string face_classifier_filename);

    ~OpenCV();

    // IOpenCVControl methods
    virtual CameraError Prepare();
    void Version() override;
    IFrameAccessRegistration* GetFrameAccessIfc(int interface_number) override;
    int Start() override;
    void ConfigProcessingMode(IOpenCVControl::ProcessingMode demo_mode) override;
    int Stop() override;
    int TouchView(float percent_width, float percent_height) override;
    int CycleProcessingMode() override;

    // IFrameAccessRegistration methods
    FrameAccessSharedPtr RegisterClient(void) override;

    // ICameraFrameAccess methods
    CameraError GetFrameDescription(CameraFormat* camera_format, int* height, int* width) override;
    CameraFrame* GetFrame(void) override;
    void ReleaseFrame(CameraFrame* camera_frame) override;
    void UnregisterClient(void) override;

    inline const bool IsRunning() const;

    cv::Mat ProcessDemo(CameraFrame* camera_frame);

private:

    std::shared_ptr<ICameraFrameAccess> frame_access_;

    volatile bool is_running_;

    // render window size
    int request_width_, request_height_;

    pthread_t thread_;

    CameraFrame* frame_input_;
    pthread_cond_t frame_input_sync_;
    pthread_mutex_t frame_input_mutex_;

    CameraFrame* frame_output_;
    pthread_cond_t frame_output_sync_;
    pthread_mutex_t frame_output_mutex_;
    bool client_owns_buffer_;

    // thread release synchronization
    /** signal clients with null frame to terminate their interface */
    pthread_mutex_t release_mutex_;
    pthread_cond_t release_sync_;
    bool is_client_registered_;

    static void* ThreadImpl(void *vptr_args);

    void Run(void);

    void Process(CameraFrame* camera_frame);

    typedef std::weak_ptr<ICameraFrameAccess> camera_frame_access_ifc_weak_ptr_t;
    camera_frame_access_ifc_weak_ptr_t frame_access_ifc_weak_ptr_;

    ProcessingMode processing_mode_;

    // opencv ball tracking demo
    BallTrackerState ball_tracker_state_;
    float touch_x_percent_;
    float touch_y_percent_;
    cv::Scalar hsv_lower_;
    cv::Scalar hsv_upper_;

    // opencv face detection demo
    std::string face_classifier_filename;
};

#endif // OPENCV_H
