#ifndef WEBCAM_CAMERA_STATUS_CALLBACK_H
#define WEBCAM_CAMERA_STATUS_CALLBACK_H

#include <jni.h>
#include <string>
#include <pthread.h>

// for stringify JSON
#include "rapidjson/prettywriter.h"
using namespace rapidjson;

enum class CameraStatusSchema {
    CONNECTED = 0,
    STARTED = 1,
    TIMESTAMP = 2,
    ERROR_RX_FIFO_OVERFLOW = 3,
    STOPPED = 4,
    RELEASED = 5,
    FRAME_DROP = 6,
    USB_STATS = 7,
    UNKNOWN = 0xff,
};

// for callback to Java object
typedef struct {
    jmethodID onStatus;
} Fields_istatuscallback;


class WebcamStatusCallback {

private:

    pthread_mutex_t callback_mutex_;

    // java side callback object
    jobject status_callback_obj_;

    Fields_istatuscallback istatuscallback_fields_;

    /** callback issued as json string */
    void IssueCallback(CameraStatusSchema camera_status_schema, const char *json_string);

public:

    WebcamStatusCallback();

    ~WebcamStatusCallback();

    int SetCallback(JNIEnv *env, jobject status_callback_obj);

    void IssueSchemaErrorRxFifoOverflow(uint64_t num_fifo_enqueue_overflows_, float overflow_percentage);

    void IssueSchemaDropFrame(uint64_t num_drop_frames, uint64_t out_of_frames);

    void IssueSchemaUsbStats(uint64_t num_dropped_frames, uint64_t num_total_frames);

};

#endif // WEBCAM_CAMERA_STATUS_CALLBACK_H
