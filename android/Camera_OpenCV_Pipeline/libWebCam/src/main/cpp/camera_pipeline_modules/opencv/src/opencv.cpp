#include <deque>
#include <linux/resource.h>
#include <sys/resource.h>
#include <vector>
#include "opencv.h"

#include "webcam_frame_access_impl.h"
#include "webcam_include.h"
#include "webcam_util.h"
#include "util.h"

//#define OPENCV_TAG "OPENCV"
#define OPENCV_TAG "test"

// RGBA
#define PIXELS_PER_BYTE_RGBA 4

using namespace cv;
using namespace std;

// fixed size deque
int fixed_size_deque_index = 0;
#define DEQUE_SIZE 8
static int fixed_size_deque_x[DEQUE_SIZE];
static int fixed_size_deque_y[DEQUE_SIZE];


OpenCV::OpenCV(IFrameAccessRegistration* frame_access, int width, int height, uint8_t channel_shift) :
        request_width_(width),
        request_height_(height),
        is_running_(false),
        frame_input_(nullptr),
        frame_output_(nullptr),
        client_owns_buffer_(false),
        processing_mode_(ProcessingMode::PROCESSING_MODE_NONE),
        ball_tracker_state_(BALL_TRACKER_INIT),
        channel_shift_(channel_shift)
{
    ENTER_(OPENCV_TAG);

    // obtain demux interface for specified channel
    if (nullptr == (frame_access_ = frame_access->RegisterClient())) {
        LOGE_(OPENCV_TAG, "invalid sensor_channel request");
    }
    CameraFormat mode;
    int upstream_height, upstream_width;
    if (CAMERA_SUCCESS != frame_access_->GetFrameDescription(&mode, &upstream_height, &upstream_width)) {
        LOGE_(OPENCV_TAG, "GetFrameDescription() failure");
    }

    // input frame format
    frame_input_ = new (CameraFrame);
    camera_frame_format frame_format = mode;
    int num_bytes = width * height * get_bytes_per_pixel(frame_format);
    frame_input_->data = new uint8_t[num_bytes];
    frame_input_->data_bytes = frame_input_->actual_bytes = num_bytes;
    frame_input_->frame_format = frame_format;
    frame_input_->step = 0;
    frame_input_->width = width;
    frame_input_->height = height;

    // output frame format
    frame_output_ = new (CameraFrame);
    frame_format = CAMERA_FRAME_FORMAT_RGBX;
    num_bytes = width * height * get_bytes_per_pixel(frame_format);
    frame_output_->data = new uint8_t[num_bytes];
    frame_output_->data_bytes = frame_output_->actual_bytes = num_bytes;
    frame_output_->frame_format = frame_format;

    frame_output_->step = 0;
    frame_output_->width = width;
    frame_output_->height = height;

    LOGI_(OPENCV_TAG, "OpenCV use_count: %ld", frame_access_.use_count());

    LOGI_(OPENCV_TAG, "width : %d, height : %d", width, height);

    pthread_cond_init(&frame_input_sync_, nullptr);
    pthread_mutex_init(&frame_input_mutex_, nullptr);

    pthread_cond_init(&frame_output_sync_, nullptr);
    pthread_mutex_init(&frame_output_mutex_, nullptr);

    pthread_cond_init(&release_sync_, nullptr);
    pthread_mutex_init(&release_mutex_, nullptr);

    EXIT_(OPENCV_TAG);
}


OpenCV::~OpenCV() {
    ENTER_(OPENCV_TAG);

    pthread_cond_destroy(&frame_output_sync_);
    pthread_mutex_destroy(&frame_output_mutex_);

    pthread_cond_destroy(&frame_input_sync_);
    pthread_mutex_destroy(&frame_input_mutex_);

    pthread_cond_destroy(&release_sync_);
    pthread_mutex_destroy(&release_mutex_);

    if (frame_input_) {
        if (frame_input_->data) {
            delete [] frame_input_->data;
        }
        delete frame_input_;
    }

    EXIT_(OPENCV_TAG);
}


inline const bool OpenCV::IsRunning() const {
    return is_running_;
}


IFrameAccessRegistration* OpenCV::GetFrameAccessIfc(int interface_number) {
    ENTER_(OPENCV_TAG);

    IFrameAccessRegistration* frame_access_registration = this;

    return frame_access_registration;
}


int OpenCV::Start() {
    ENTER_(OPENCV_TAG);

    int result = EXIT_FAILURE;

    if (!IsRunning()) {

        is_running_ = true;

        pthread_mutex_lock(&frame_input_mutex_);
        result = pthread_create(&thread_, nullptr, ThreadImpl, (void*) this);
        pthread_mutex_unlock(&frame_input_mutex_);

        if (result != EXIT_SUCCESS) {
            LOGE_(OPENCV_TAG, "pthread_create() failure : %d", result);
            is_running_ = false;
        }
    }

    RETURN_(OPENCV_TAG, result, int);
}


void OpenCV::ConfigProcessingMode(IOpenCVControl::ProcessingMode processing_mode) {
    ENTER_(OPENCV_TAG);

    if (processing_mode != processing_mode) {

        processing_mode = processing_mode;

        switch (processing_mode) {

            case ProcessingMode::PROCESSING_MODE_NONE:
                break;

            case ProcessingMode::PROCESSING_MODE_BALL_TRACKER:
                ball_tracker_state_ = BALL_TRACKER_INIT;
                break;

            default:
                break;
        }
    }

    EXIT_(OPENCV_TAG);
}


cv::Mat OpenCV::ProcessDemo(CameraFrame* camera_frame) {

    cv::Mat frame_out;

    switch (processing_mode_) {

        case ProcessingMode::PROCESSING_MODE_NONE: {

            if (camera_frame->frame_format == CAMERA_FRAME_FORMAT_YUYV) {

                // form frame from raw data buffer and color space xform from yuv to rgba
                Mat frame(camera_frame->height, camera_frame->width, CV_8UC2, camera_frame->data);
                cvtColor(frame, frame_out, COLOR_YUV2RGBA_YUY2);

                // perform video processing in huv color space
                Mat centroids;
                cvtColor(frame, centroids, COLOR_YUV2RGB_YUY2);
                cvtColor(centroids, centroids, COLOR_RGB2HSV);

                // spatial filter
                GaussianBlur(centroids, centroids, Size(11, 11), 0, 0);

                // see pyimagesearch range-detector.py for tuning color range for object of interest
                Scalar greenLower = Scalar(27, 40, 70);
                Scalar greenUpper = Scalar(70, 245, 255);

                // qualify ball object
                Mat mask;
                inRange(centroids, greenLower, greenUpper, mask);

                // clean up the mask

                int erosion_type = MORPH_RECT;
                int erosion_size = 4;
                Mat erosion_element = getStructuringElement(erosion_type,
                                                            Size(2 * erosion_size + 1, 2 * erosion_size+1),
                                                            Point(erosion_size, erosion_size) );
                erode(mask, mask, erosion_element, Point(-1,-1), 2);

                int dilation_type = MORPH_RECT;
                int dilation_size = 4;
                Mat dilation_element = getStructuringElement(dilation_type,
                                                             Size(2 * dilation_size + 1, 2 * dilation_size+1),
                                                             Point(dilation_size, dilation_size));
                dilate(mask, mask, dilation_element, Point(-1,-1), 2);

                // find largest contour the ball bask
                vector<vector<Point>> contours;
                vector<Vec4i> hierarchy;

                findContours(mask.clone(), contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

                vector<Point> max_contour_ref;
                int largest_area = 0;
                int i = 0;
                for (vector<vector<Point>>::iterator contour_iter = contours.begin() ; contour_iter != contours.end(); ++contour_iter, i++) {
                    double area = contourArea(*contour_iter, false);
                    if(area > largest_area) {
                        largest_area = area;
                        max_contour_ref = *contour_iter;
                    }
                }

                // center of mass and radius for largest area contour
                Point2f center;
                float radius;
                if (max_contour_ref.size()) {
                    minEnclosingCircle(max_contour_ref, center, radius);
                    Moments mu = moments(max_contour_ref, false);
                    Point center = Point(int(mu.m10/mu.m00) , int(mu.m01/mu.m00));
                    if (radius > 10) {
                        // renderball outline
                        circle(frame_out, center, radius, Scalar(255, 0, 0), 2);

                        // cm tracking tail
                        fixed_size_deque_x[fixed_size_deque_index] = center.x;
                        fixed_size_deque_y[fixed_size_deque_index++] = center.y;
                        if (fixed_size_deque_index == DEQUE_SIZE) fixed_size_deque_index = 0;
                    }

                    // render tracking tail
                    for (int i = 1; i < DEQUE_SIZE; i++) {
                        line(frame_out,
                             Point(fixed_size_deque_x[i-1], fixed_size_deque_y[i-1]),
                             Point(fixed_size_deque_x[i], fixed_size_deque_y[i]),
                             Scalar(0, 0, 255),
                             4);
                    }

                    if (fixed_size_deque_index == DEQUE_SIZE) fixed_size_deque_index = 0;
                }

            } else if (camera_frame->frame_format == CAMERA_FRAME_FORMAT_BM22) {

                /* TODO
                 * This is a method to extract lower 8 bits of grayscale from one of the fields
                 * I doubt this is accelerated in any fashion
                 */
                cv::Mat_<uint16_t> frame_depth = cv::Mat(camera_frame->height, 2 * camera_frame->width, CV_16UC1, camera_frame->data);
                cv::Mat_<uint16_t> frame_amplitude = cv::Mat(camera_frame->height, 2 * camera_frame->width, CV_16UC1, camera_frame->data+2);

                cv::resize(frame_depth, frame_out, cv::Size(camera_frame->width, camera_frame->height), 0, 0, cv::INTER_NEAREST);
                cv::resize(frame_amplitude, frame_out, cv::Size(camera_frame->width, camera_frame->height), 0, 0, cv::INTER_NEAREST);

                // prior to output, select byte range of interest for downstream client
                int shift = channel_shift_;
                frame_out.forEach<uint16_t>([shift](uint16_t &pixel, const int position[]) -> void {
                    pixel = pixel >> shift;
                });
                frame_out.convertTo(frame_out, CV_8UC1);

                cv::cvtColor(frame_out, frame_out, cv::COLOR_GRAY2RGBA);
            }

            // return frame
            frame_access_->ReleaseFrame(camera_frame);
            pthread_mutex_unlock(&frame_input_mutex_);
        }
            break;

        case ProcessingMode::PROCESSING_MODE_NUM:
            break;
    }

    return frame_out;
}


int OpenCV::Stop() {
    ENTER_(OPENCV_TAG);

    bool b = IsRunning();
    if (b) {
        is_running_ = false;

        pthread_mutex_lock(&frame_input_mutex_);
        pthread_cond_signal(&frame_input_sync_);
        pthread_mutex_unlock(&frame_input_mutex_);

        if (pthread_join(thread_, nullptr) != EXIT_SUCCESS) {
            LOGW_(OPENCV_TAG, "pthread_join() failure");
        }
    } else {
        LOGI_(OPENCV_TAG, "Stop() call when not running");
    }

    RETURN_(OPENCV_TAG, CAMERA_SUCCESS, int);
}


int OpenCV::CycleProcessingMode() {
    ENTER_(OPENCV_TAG);

    // TODO mutex access to mode switch
    switch (processing_mode_) {

        case ProcessingMode::PROCESSING_MODE_NONE:
            processing_mode_ = ProcessingMode::PROCESSING_MODE_BALL_TRACKER;
            ball_tracker_state_ = BALL_TRACKER_INIT;
            break;

        case ProcessingMode::PROCESSING_MODE_BALL_TRACKER:
            processing_mode_ = ProcessingMode::PROCESSING_MODE_NONE;
            break;

        default:
            break;
    }

    RETURN_(OPENCV_TAG, CAMERA_SUCCESS, int);
}


int OpenCV::ScaleIfGray16(bool downscale) {
    ENTER_(RENDERER_TAG);

    if (downscale) {
        if (channel_shift_ > 0) {
            channel_shift_--;
        }
    } else {
        if (channel_shift_ < 8) {
            channel_shift_++;
        }
    }

    LOGI_(RENDERER_TAG, "current shift scaling : %d", channel_shift_);

    RETURN_(RENDERER_TAG, channel_shift_, int);
}


void* OpenCV::ThreadImpl(void* vptr_args) {
    ENTER_(OPENCV_TAG);

    int result;

    // try to increase thread priority
    int prio = getpriority(PRIO_PROCESS, 0);
    nice(10);
    if (getpriority(PRIO_PROCESS, 0) < prio) {
        LOGW_(OPENCV_TAG, "could not change thread priority");
    }

    OpenCV* open_cv = reinterpret_cast<OpenCV*>(vptr_args);
    if (open_cv) {
        result = open_cv->Prepare();
        if (!result) {
            open_cv->Run();
        }
    }

    PRE_EXIT_(OPENCV_TAG);
    pthread_exit(nullptr);
}


CameraError OpenCV::Prepare() {
    ENTER_(OPENCV_TAG);

    auto result = CAMERA_SUCCESS;

    RETURN_(OPENCV_TAG, result, CameraError);
}


//std::string OpenCV::Version() {
void OpenCV::Version() {
    LOGD_(OPENCV_TAG, "%s", cv::getBuildInformation().c_str());
//    return cv::getBuildInformation().c_str();
}


void OpenCV::Run(void) {
    ENTER_(OPENCV_TAG);

    CameraFrame* frame = nullptr;

    for ( ; IsRunning() ; ) {

        if (nullptr != (frame = frame_access_->GetFrame())) {

            // early release performed in process
            Process(frame);

        } else {

            LOGI_(OPENCV_TAG, "null frame received");

            // treat null frame as a pipeline shutdown request
            pthread_mutex_lock(&frame_output_mutex_);
            is_running_ = false;
            if (frame_output_) {
                if (frame_output_->data) {
                    delete [] frame_output_->data;
                }
                delete frame_output_;
            }
            frame_output_ = nullptr;

            LOGI_(OPENCV_TAG, "broadcast null frame availability to opencv clients");

            // broadcast null frame availability to clients
            pthread_cond_broadcast(&frame_output_sync_);
            pthread_mutex_unlock(&frame_output_mutex_);

            // wait for client unregistration signal
            pthread_mutex_lock(&release_mutex_);
            while (is_client_registered_) {
                pthread_cond_wait(&release_sync_, &release_mutex_);
            }
            pthread_mutex_unlock(&release_mutex_);
        }
    }

    // signal release of interface to upstream element
    frame_access_ = nullptr;

    EXIT_(OPENCV_TAG);
}


void OpenCV::Process(CameraFrame* camera_frame) {

    pthread_mutex_lock(&frame_input_mutex_);

    if ( camera_frame && camera_frame->height == frame_input_->height &&
         camera_frame->width == frame_input_->width &&
         camera_frame->data_bytes == frame_input_->data_bytes) {

        // ProcessDemo performs mutex unlock
        cv::Mat frame_out = ProcessDemo(camera_frame);

        memcpy(frame_output_->data, frame_out.data, frame_output_->data_bytes);

        // broadcast frame availability to clients
        pthread_cond_broadcast(&frame_output_sync_);

    } else {
        LOGI_(OPENCV_TAG, "release");

        // return frame
        frame_access_->ReleaseFrame(camera_frame);
        pthread_mutex_unlock(&frame_input_mutex_);
    }

}


CameraError OpenCV::GetFrameDescription(CameraFormat* mode, int* height, int* width) {

    CameraError camera_error = CAMERA_ERROR_OTHER;

    *mode = frame_output_->frame_format;
    *height = frame_output_->height;
    *width = frame_output_->width;
    camera_error = CAMERA_SUCCESS;

    return camera_error;
}


CameraFrame* OpenCV::GetFrame(void) {

    pthread_mutex_lock(&frame_output_mutex_);
    // lock held across frame get and release calls

    if (0 == pthread_cond_wait(&frame_output_sync_, &frame_output_mutex_)) {
        if (frame_output_) {
            client_owns_buffer_ = true;
        } else {
            client_owns_buffer_ = false;
            if (0 != pthread_mutex_unlock (&frame_output_mutex_)) {
                // log failure ... i.e. EBUSY
                LOGE_(OPENCV_TAG, "pthread_mutex_unlock() failure");
            }
        }
    } else {
        // TODO log here
    }

    return frame_output_;
}


void OpenCV::ReleaseFrame(CameraFrame* camera_frame) {

    client_owns_buffer_ = false;

    if (0 != pthread_mutex_unlock (&frame_output_mutex_)) {
        LOGE_(OPENCV_TAG, "pthread_mutex_unlock() failure");
    }
}


FrameAccessSharedPtr OpenCV::RegisterClient(void) {
    ENTER_(OPENCV_TAG);

    FrameAccessSharedPtr shared_ptr;

    if (nullptr == (shared_ptr = frame_access_ifc_weak_ptr_.lock())) {

        shared_ptr = std::make_shared<WebcamFrameAccessIfc>(this);
        frame_access_ifc_weak_ptr_ = shared_ptr;

        pthread_mutex_lock(&release_mutex_);
        is_client_registered_ = true;
        pthread_mutex_unlock(&release_mutex_);
    }
    PRE_EXIT_(OPENCV_TAG);

    return shared_ptr;
}


void OpenCV::UnregisterClient(void) {
    ENTER_(OPENCV_TAG);

    // last client to unregister releases thread

    pthread_mutex_lock(&release_mutex_);
    is_client_registered_ = false;
    pthread_cond_signal(&release_sync_);
    pthread_mutex_unlock(&release_mutex_);

    EXIT_(OPENCV_TAG);
}
