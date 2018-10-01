#include "webcam_status_callback.h"

#include "JsonStatus.h"
#include "util.h"

//#define TAG_WEBCAM_STATUS_CALLBACK "WEBCAM_STATUS_CALLBACK"
#define TAG_WEBCAM_STATUS_CALLBACK "test"


WebcamStatusCallback::WebcamStatusCallback()
        :  status_callback_obj_(nullptr) {
    ENTER_(TAG_WEBCAM_STATUS_CALLBACK);

    pthread_mutex_init(&callback_mutex_, nullptr);

    EXIT_(TAG_WEBCAM_STATUS_CALLBACK);
}


WebcamStatusCallback::~WebcamStatusCallback() {
    ENTER_(TAG_WEBCAM_STATUS_CALLBACK);

    pthread_mutex_destroy(&callback_mutex_);

    EXIT_(TAG_WEBCAM_STATUS_CALLBACK);
}


int WebcamStatusCallback::SetCallback(JNIEnv* env, jobject status_callback_obj) {
    ENTER_(TAG_WEBCAM_STATUS_CALLBACK);

    pthread_mutex_lock(&callback_mutex_);

    LOGE_(CAMERASTATUSCALLBACK_TAG, "status_callback_obj_ = %x\n", status_callback_obj_);

    if (!env->IsSameObject(status_callback_obj_, status_callback_obj)) {

        istatuscallback_fields_.onStatus = nullptr;

        if (status_callback_obj_) {
            env->DeleteGlobalRef(status_callback_obj_);
        }

        status_callback_obj_ = status_callback_obj;

        if (status_callback_obj) {

            // get method IDs of Java object for callback
            jclass clazz = env->GetObjectClass(status_callback_obj);

            if (clazz) {
                istatuscallback_fields_.onStatus = env->GetMethodID(clazz, "onStatus", "(ILjava/lang/String;)V");
            } else {
                LOGE_(TAG_WEBCAM_STATUS_CALLBACK, "failed to get object class");
            }

            env->ExceptionClear();

            if (!istatuscallback_fields_.onStatus) {
                LOGE_(CAMERASTATUSCALLBACK_TAG, "Can't find IStatusCallback#onStatus");
                env->DeleteGlobalRef(status_callback_obj);
                status_callback_obj_ = status_callback_obj = nullptr;
            }
        } else {
            LOGI_(TAG_WEBCAM_STATUS_CALLBACK, "status_callback_obj == 0");
        }
    }

    pthread_mutex_unlock(&callback_mutex_);

    RETURN_(TAG_WEBCAM_STATUS_CALLBACK, 0, int);
}


// http://rapidjson.org/classrapidjson_1_1_pretty_writer.html
void WebcamStatusCallback::IssueSchemaErrorRxFifoOverflow(uint64_t num_fifo_enqueue_overflows_, float overflow_percentage) {

    StringBuffer json_string_buffer;
    PrettyWriter<StringBuffer> writer(json_string_buffer);

    writer.StartObject();
    writer.String("ovflw");
    writer.Int(num_fifo_enqueue_overflows_);
    writer.String("ovflw_pcnt");
    writer.Double(overflow_percentage);
    writer.EndObject();

    IssueCallback(CameraStatusSchema::ERROR_RX_FIFO_OVERFLOW, json_string_buffer.GetString());
}


void WebcamStatusCallback::IssueSchemaUsbStats(uint64_t num_dropped_frames, uint64_t num_total_frames) {

    StringBuffer json_string_buffer;
    PrettyWriter<StringBuffer> writer(json_string_buffer);

    writer.StartObject();
    writer.String("num_dropped_frames");
    writer.Int(num_dropped_frames);
    writer.String("num_total_frames");
    writer.Double(num_total_frames);
    writer.EndObject();

    IssueCallback(CameraStatusSchema::USB_STATS, json_string_buffer.GetString());
}


void WebcamStatusCallback::IssueSchemaDropFrame(uint64_t num_drop_frames, uint64_t out_of_frames) {

    StringBuffer json_string_buffer;
    PrettyWriter<StringBuffer> writer(json_string_buffer);

    writer.StartObject();
    writer.String("num_drop_frames");
    writer.Int(num_drop_frames);
    writer.String("out_of_frames");
    writer.Int(out_of_frames);
    writer.EndObject();

    IssueCallback(CameraStatusSchema::FRAME_DROP, json_string_buffer.GetString());
}


void WebcamStatusCallback::IssueCallback(CameraStatusSchema camera_status_schema, const char* json_string) {

    // allowed to be called from a different context
    JavaVM* vm = getVM_();
    JNIEnv* env;
    // attach to JavaVM
    vm->AttachCurrentThread(&env, nullptr);

    pthread_mutex_lock(&callback_mutex_);

    if (status_callback_obj_) {
        jstring json_java_string = env->NewStringUTF(json_string);
        env->CallVoidMethod(status_callback_obj_, istatuscallback_fields_.onStatus, (int)camera_status_schema, json_java_string);
        env->ExceptionClear();
    }

    pthread_mutex_unlock(&callback_mutex_);

    vm->DetachCurrentThread();
}
