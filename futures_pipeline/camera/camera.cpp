#include "camera.h"

#include "input.h"
#include "sync-log.h"

#include "frame-queue.h"
#include "exec-shell.h"
// #include "webcam_include."
// #include "libuvc/libuvc_internal.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/schema.h"
// #include "prettywriter.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "schema.h"
#include "schema_invalid.h"
#include "utils.h"


// #include <ctype.h>
#include <cstring>
#include <memory>
#include <stdio.h>
#include <string>
#include <sstream>  
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>


using namespace rapidjson;
using namespace std;

#include <sys/ioctl.h>
#include <fcntl.h> 
#include <sys/types.h>
// #include <sys/stat.h>
#include <sys/mman.h>

// TODO client to chose from enumerated json presented cameraS
CameraConfig camera_config = {2, 640, 640, 480, 640*480*3, 30};
    int bytes_per_pixel;
    int width_enumerated;
    int width_actual;
    int height;
    int bytes_per_row;
    int fps;

shared_ptr<Output> async_function(shared_ptr<Input> input)
{
    sched_param sch;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &sch);
    if (SCHED_OTHER == policy || SCHED_BATCH == policy || SCHED_IDLE == policy) {

        pid_t tid;
        tid = syscall(SYS_gettid);
        int niceness = getpriority(PRIO_PROCESS, tid);

        /* a non-root users can only make threads nicer 0->19
           root can make threads greedy -20->19
           root can use FIFO scheduling class
        */

        if (-1 == setpriority(PRIO_PROCESS, tid, input->niceness_)) {
            SyncLog::GetLog()->Log("setpriority() failure");
        }

        int niceness_new = getpriority(PRIO_PROCESS, tid);

        SyncLog::GetLog()->Log("thread id = " + to_string(tid) + ", cores id = " + to_string(sched_getcpu())
            + ", priority = " + to_string(niceness) + ":" + to_string(niceness_new)
            + ", policy = " + string(((policy == SCHED_OTHER)  ? "SCHED_OTHER" :
                (policy == SCHED_BATCH) ? "SCHED_RR" :
                "SCHED_IDLE")));
    }

    int run_count{0};
    bool running{true};
    while (running) {
        //SyncLog::GetLog()->Log(to_string(input->instance_)+": async_function pass : "+to_string(run_count));
        this_thread::sleep_for(chrono::milliseconds(input->load_));
        if (input->load_ == run_count++) running = false;
    }

    shared_ptr<Output> output = make_shared<Output>(input->instance_);

    return output;
}

const V4l2FormatInfo Camera::pixel_formats_[] = {
    { "RGB332", V4L2_PIX_FMT_RGB332, 1 },
    { "RGB555", V4L2_PIX_FMT_RGB555, 1 },
    { "RGB565", V4L2_PIX_FMT_RGB565, 1 },
    { "RGB555X", V4L2_PIX_FMT_RGB555X, 1 },
    { "RGB565X", V4L2_PIX_FMT_RGB565X, 1 },
    { "BGR24", V4L2_PIX_FMT_BGR24, 1 },
    { "RGB24", V4L2_PIX_FMT_RGB24, 1 },
    { "BGR32", V4L2_PIX_FMT_BGR32, 1 },
    { "RGB32", V4L2_PIX_FMT_RGB32, 1 },
    { "Y8", V4L2_PIX_FMT_GREY, 1 },
    { "Y10", V4L2_PIX_FMT_Y10, 1 },
    { "Y12", V4L2_PIX_FMT_Y12, 1 },
    { "Y16", V4L2_PIX_FMT_Y16, 1 },
    { "UYVY", V4L2_PIX_FMT_UYVY, 1 },
    { "VYUY", V4L2_PIX_FMT_VYUY, 1 },
    { "YUYV", V4L2_PIX_FMT_YUYV, 1 },
    { "YVYU", V4L2_PIX_FMT_YVYU, 1 },
    { "NV12", V4L2_PIX_FMT_NV12, 1 },
    { "NV12M", V4L2_PIX_FMT_NV12M, 2 },
    { "NV21", V4L2_PIX_FMT_NV21, 1 },
    { "NV21M", V4L2_PIX_FMT_NV21M, 2 },
    { "NV16", V4L2_PIX_FMT_NV16, 1 },
    { "NV16M", V4L2_PIX_FMT_NV16M, 2 },
    { "NV61", V4L2_PIX_FMT_NV61, 1 },
    { "NV61M", V4L2_PIX_FMT_NV61M, 2 },
    { "NV24", V4L2_PIX_FMT_NV24, 1 },
    { "NV42", V4L2_PIX_FMT_NV42, 1 },
    { "YUV420M", V4L2_PIX_FMT_YUV420M, 3 },
    { "SBGGR8", V4L2_PIX_FMT_SBGGR8, 1 },
    { "SGBRG8", V4L2_PIX_FMT_SGBRG8, 1 },
    { "SGRBG8", V4L2_PIX_FMT_SGRBG8, 1 },
    { "SRGGB8", V4L2_PIX_FMT_SRGGB8, 1 },
    { "SBGGR10_DPCM8", V4L2_PIX_FMT_SBGGR10DPCM8, 1 },
    { "SGBRG10_DPCM8", V4L2_PIX_FMT_SGBRG10DPCM8, 1 },
    { "SGRBG10_DPCM8", V4L2_PIX_FMT_SGRBG10DPCM8, 1 },
    { "SRGGB10_DPCM8", V4L2_PIX_FMT_SRGGB10DPCM8, 1 },
    { "SBGGR10", V4L2_PIX_FMT_SBGGR10, 1 },
    { "SGBRG10", V4L2_PIX_FMT_SGBRG10, 1 },
    { "SGRBG10", V4L2_PIX_FMT_SGRBG10, 1 },
    { "SRGGB10", V4L2_PIX_FMT_SRGGB10, 1 },
    { "SBGGR12", V4L2_PIX_FMT_SBGGR12, 1 },
    { "SGBRG12", V4L2_PIX_FMT_SGBRG12, 1 },
    { "SGRBG12", V4L2_PIX_FMT_SGRBG12, 1 },
    { "SRGGB12", V4L2_PIX_FMT_SRGGB12, 1 },
    { "DV", V4L2_PIX_FMT_DV, 1 },
    { "MJPEG", V4L2_PIX_FMT_MJPEG, 1 },
    { "MPEG", V4L2_PIX_FMT_MPEG, 1 },
};

 const Field Camera::fields_[] = {
    { "any", V4L2_FIELD_ANY },
    { "none", V4L2_FIELD_NONE },
    { "top", V4L2_FIELD_TOP },
    { "bottom", V4L2_FIELD_BOTTOM },
    { "interlaced", V4L2_FIELD_INTERLACED },
    { "seq-tb", V4L2_FIELD_SEQ_TB },
    { "seq-bt", V4L2_FIELD_SEQ_BT },
    { "alternate", V4L2_FIELD_ALTERNATE },
    { "interlaced-tb", V4L2_FIELD_INTERLACED_TB },
    { "interlaced-bt", V4L2_FIELD_INTERLACED_BT },
};

// +++++ transform macros

static inline unsigned char sat(int i) {
  return (unsigned char)( i >= 255 ? 255 : (i < 0 ? 0 : i));
}
#define IYUYV2RGB_2(pyuv, prgb) { \
    int r = (22987 * ((pyuv)[3] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[1] - 128) - 11698 * ((pyuv)[3] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[1] - 128)) >> 14; \
    (prgb)[0] = sat(*(pyuv) + r); \
    (prgb)[1] = sat(*(pyuv) + g); \
    (prgb)[2] = sat(*(pyuv) + b); \
    (prgb)[3] = sat((pyuv)[2] + r); \
    (prgb)[4] = sat((pyuv)[2] + g); \
    (prgb)[5] = sat((pyuv)[2] + b); \
    }
#define IYUYV2RGB_4(pyuv, prgb) IYUYV2RGB_2(pyuv, prgb); IYUYV2RGB_2(pyuv + 4, prgb + 6);
#define IYUYV2RGB_8(pyuv, prgb) IYUYV2RGB_4(pyuv, prgb); IYUYV2RGB_4(pyuv + 8, prgb + 12);

// ----- transform macros

Camera::Camera() :
    detected_{false},
    synclog_(SyncLog::GetLog()),
    camera_frame_queue_{nullptr},
//    request_stop_{false},
    release_frame_queue_{false},
    frame_avail_{false},
    request_start_{false},
    is_running_{false},
    frame_pull_thread_mutex_{}
    // camera_frame_queue_{nullptr}
        // : uvc_fd_(0),
        //   usb_fs_(nullptr),
        //   camera_context_(nullptr),
        //   camera_device_(nullptr),
        //   camera_device_handle_(nullptr),
        //   webcam_fifo_(nullptr),
        //   status_callback_(nullptr)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    camera_doc_.SetObject();

    /* v4l2-ctl camera mode detection */
    if (DetectCameras()) {
        cout<<"cameras detected"<<endl;
    } else {
        cout<<"no cameras detected"<<endl;
    }

    // synclog_= SyncLog::GetLog();

    // +++++ move to queue
    // TODO keep camera source queing in native aquired format (probably yuyv)
    //   construct the pipe of CameraFrames and let client perform any needed colorspace conversion
    uvc_frame_.data = nullptr;
    uvc_frame_.data_bytes = 0;
    uvc_frame_.actual_bytes = 0;
    uvc_frame_.width = 0;
    uvc_frame_.height = 0;
    uvc_frame_.frame_format = CAMERA_FRAME_FORMAT_YUYV;
    uvc_frame_.step = 0;
    uvc_frame_.sequence = 0;
    uvc_frame_.capture_time.tv_sec = 0;
    uvc_frame_.capture_time.tv_usec = 0;

    rgb_frame_.data = nullptr;
    rgb_frame_.data_bytes = 0;
    rgb_frame_.actual_bytes = 0;
    rgb_frame_.width = 0;
    rgb_frame_.height = 0;
    rgb_frame_.frame_format = CAMERA_FRAME_FORMAT_RGB;
    rgb_frame_.step = 0;
    rgb_frame_.sequence = 0;
    rgb_frame_.capture_time.tv_sec = 0;
    rgb_frame_.capture_time.tv_usec = 0;
    // ----- move to queue

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit()");
}


Camera::~Camera()
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    // Stop();

    // if (camera_context_) {
    //     uvc_exit(camera_context_);
    //     camera_context_ = nullptr;
    // }
    // if (usb_fs_) {
    //     free(usb_fs_);
    //     usb_fs_ = nullptr;
    // }

    // if (status_callback_) {
    //     delete status_callback_;
    // }

    // +++++ queue management of native colorspace frames
    // TODO
	if (uvc_frame_.data != nullptr) {
        delete [] uvc_frame_.data;
        uvc_frame_.data = nullptr;
    }
    uvc_frame_.data_bytes = 0;
    uvc_frame_.actual_bytes = 0;
    uvc_frame_.width = 0;
    uvc_frame_.height = 0;
    uvc_frame_.frame_format = CAMERA_FRAME_FORMAT_YUYV;
    uvc_frame_.step = 0;
    uvc_frame_.sequence = 0;
    uvc_frame_.capture_time.tv_sec = 0;
    uvc_frame_.capture_time.tv_usec = 0;

    if (rgb_frame_.data != nullptr) {
        delete [] rgb_frame_.data;
        rgb_frame_.data = nullptr;
    }
    rgb_frame_.data = nullptr;
    rgb_frame_.data_bytes = 0;
    rgb_frame_.actual_bytes = 0;
    rgb_frame_.width = 0;
    rgb_frame_.height = 0;
    rgb_frame_.frame_format = CAMERA_FRAME_FORMAT_RGB;
    rgb_frame_.step = 0;
    rgb_frame_.sequence = 0;
    rgb_frame_.capture_time.tv_sec = 0;
    rgb_frame_.capture_time.tv_usec = 0;
    // ---- queue management of native colorspace frames

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
}


// int WebCam::Prepare(int vid, int pid, int fd, int busnum, int devaddr, const char* usbfs) {
//     ENTER_(WEBCAM_TAG);

//     uvc_error_t result = UVC_ERROR_BUSY;
//     if (!camera_device_handle_ && fd) {
//         if (usb_fs_) {
//             free(usb_fs_);
//         }
//         usb_fs_ = strdup(usbfs);
//         if (!camera_context_) {
//             result = uvc_init2(&camera_context_, nullptr, usb_fs_);
//             if (result < 0) {
//                 LOGE("failed to init libuvc");
//                 RETURN_(WEBCAM_TAG, result, int);
//             }
//         }
//         fd = dup(fd);
//         result = uvc_get_device_with_fd(camera_context_, &camera_device_, vid, pid, nullptr, fd, busnum, devaddr);
//         if (!result) {
//             LOGE_(WEBCAM_TAG, "uvc_get_device_with_fd() success");
//             result = uvc_open(camera_device_, &camera_device_handle_);
//             if (!result) {
//                 LOGE("uvc_open() success");
//                 uvc_fd_ = fd;
//                 status_callback_ = new WebcamStatusCallback();
//                 webcam_fifo_ = new WebcamFifo(camera_device_handle_);
//             } else {
//                 LOGE("uvc_open() failure : err = %d", result);
//                 uvc_unref_device(camera_device_);
//                 camera_device_ = nullptr;
//                 camera_device_handle_ = nullptr;
//                 close(fd);
//             }
//         } else {
//             LOGE_(WEBCAM_TAG, "uvc_get_device_with_fd() failure : err = %d", result);
//             close(fd);
//         }
//     } else {
//         LOGW_(WEBCAM_TAG, "camera is already opened. you should release first");
//     }
//     RETURN_(CAMERA_TAG, result, int);
// }


// int WebCam::SetStatusCallback(JNIEnv* env, jobject status_callback_obj) {
//     ENTER_(WEBCAM_TAG);

//     int result = EXIT_FAILURE;

//     if (status_callback_) {
//         // allow only one callback client
//         if (EXIT_SUCCESS == (result = status_callback_->SetCallback(env, status_callback_obj))) {
//             result = webcam_fifo_->SetStatusCallback(status_callback_);
//         }
//     }

//     RETURN_(WEBCAM_TAG, result, int);
// }


void FindAllOccurances(std::vector<size_t>& vec, std::string search_this, std::string search_for)
{
    /* first occurrence */
    size_t pos = search_this.find(search_for);

    /* search for all occurances */
    while (pos != std::string::npos) {

        /* add starting position */
        vec.push_back(pos);

        /* next position */
        pos = search_this.find(search_for, pos + search_for.size());
    }
}

/* query for installed cameras
    v4l2-ctl --list-devices
    v4l2-ctl --device=/dev/video<device number> -D 
    v4l2-ctl --device=/dev/video<device number> -D --list-formats-ext
    // if no modes that nodes is likely used for timestamp delivery
*/


bool Camera::DetectCameras() 
{
    size_t pos = 0;
    size_t pos_end = 0;

    string camera_card_type;

    Value camera_array(kArrayType);
    Document::AllocatorType& allocator = camera_doc_.GetAllocator();

    /* active video device nodes */
    string device_list = exec("ls -1 /dev/video*");
    stringstream device_list_ss(device_list);

    /* extract supported rate and resolutions data from camera devices */
    string device_node;
    while (device_list_ss >> device_node) {

        Value camera_device_value;
        camera_device_value.SetObject();

        string list_formats = "v4l2-ctl --device=" + device_node + " -D --list-formats-ext";
        string device_formats = exec(list_formats.c_str());

        /* /dev/video<x> */
        Value device_node_val;
        device_node_val.SetString(device_node.c_str(), device_node.length(), allocator);

        Value camera_card_type_val;
        pos = 0;
        string delimiter = "Card type     : ";
        if (string::npos != (pos = device_formats.find(delimiter, pos))) {

            pos += delimiter.length();

            if (string::npos != (pos_end = device_formats.find("\n", pos))) {

                size_t len = pos_end - pos;
                camera_card_type = device_formats.substr(pos, len);
                pos = pos_end;
                camera_card_type_val.SetString(camera_card_type.c_str(), camera_card_type.length(), allocator);
            }
        }

        Value capture_type_array(kArrayType);
        delimiter = "ioctl: VIDIOC_ENUM_FMT";
        if (string::npos != (pos = device_formats.find(delimiter, pos))) {
            
            pos += delimiter.length();

            int index; 
            string type, format, name;

            /* ioctl: VIDIOC_ENUM_FMT Index's */
            delimiter = "Index       : ";
            while (string::npos != (pos = device_formats.find(delimiter, pos))) {

                pos += delimiter.length();
                if (string::npos != (pos_end = device_formats.find("\n", pos))) {
                    size_t len = pos_end - pos;
                    index = stoi(device_formats.substr(pos, len));
                    pos = pos_end;
                }

                delimiter = "Type        : ";
                if (string::npos != (pos = device_formats.find(delimiter, pos))) {
                    pos += delimiter.length();
                    if (string::npos != (pos_end = device_formats.find("\n", pos))) {
                        size_t len = pos_end - pos;
                        type = device_formats.substr(pos, len);
                        pos = pos_end;
                    }
                }

                delimiter = "Pixel Format: ";
                if (string::npos != (pos = device_formats.find(delimiter, pos))) {
                    pos += delimiter.length();
                    if (string::npos != (pos_end = device_formats.find('\n', pos))) {
                        size_t len = pos_end - pos;
                        format = device_formats.substr(pos, len);
                        pos = pos_end;
                    }
                }

                delimiter = "Name        :";
                if (string::npos != (pos = device_formats.find(delimiter, pos))) {
                    pos += delimiter.length();
                    if (string::npos != (pos_end = device_formats.find('\n', pos))) {
                        size_t len = pos_end - pos;
                        name = device_formats.substr(pos, len);
                        pos = pos_end;
                    }
                }

                /* add capture types */
                if (!type.compare("Video Capture")) {

                    Value capture_format_value;
                    capture_format_value.SetObject();

                    Value index_val;
                    index_val.SetInt(index);
                    capture_format_value.AddMember("index", index_val, allocator);

                    Value type_val;
                    type_val.SetString(type.c_str(), type.length(), allocator);
                    capture_format_value.AddMember("type", type_val, allocator);

                    Value format_val;
                    format_val.SetString(format.c_str(), format.length(), allocator);
                    capture_format_value.AddMember("format", format_val, allocator);

                    Value name_val;
                    name_val.SetString(name.c_str(), name.length(), allocator);
                    capture_format_value.AddMember("name", name_val, allocator);

                    /* each resolution supports multiple rates */
                    Value res_rate_array(kArrayType);

                    string res_rate = device_formats.substr(pos);
                    stringstream res_rate_ss(res_rate);
                    string line;
                    string resolution;

                    // TODO change above the getline

                    /* eat a blank line */
                    getline(res_rate_ss, line);
                    getline(res_rate_ss, line);

                    while (!res_rate_ss.eof()) {
                        
                        Value res_rate_val;
                        res_rate_val.SetObject();

                        Value resolution_val;
                        resolution_val.SetObject();

                        Value rate_array(kArrayType);

                        delimiter = "Size: Discrete ";
                        if (string::npos != (pos = line.find(delimiter))) {

                            resolution = line.substr(pos + delimiter.length());
                            resolution_val.SetString(resolution.c_str(), resolution.length(), allocator);

                            Value rate_value;
                            rate_value.SetObject();
                            do {

                                getline(res_rate_ss, line);

                                delimiter = "(";
                                if (string::npos != (pos = line.find(delimiter))) {

                                    string rate = line.substr(pos+1, string::npos);

                                    delimiter = ")";
                                    if (string::npos != (pos_end = rate.find(delimiter))) {

                                        Value rate_value;
                                        rate_value.SetObject();
                                        rate_value.SetString(rate.c_str(), rate.length()-1, allocator);
                                        rate_array.PushBack(rate_value, allocator);
                                    }
                                }
                            } while (string::npos != pos);
                        } else {
                            getline(res_rate_ss, line);
                        }

                        if (rate_array.Size()) {
                            res_rate_val.AddMember("resolution", resolution_val, allocator);
                            res_rate_val.AddMember("rates", rate_array, allocator);
                            res_rate_array.PushBack(res_rate_val, allocator);
                        }
                    }

                    if (res_rate_array.Size()) {
                        capture_format_value.AddMember("res rates", res_rate_array, allocator);
                        capture_type_array.PushBack(capture_format_value, allocator);
                    }
                }

                /* next format */
                delimiter = "Index       : ";

            } /* end while ioctl: VIDIOC_ENUM_FMT */

            if (capture_type_array.Size()) {
                camera_device_value.AddMember("device node", device_node_val, allocator);
                camera_device_value.AddMember("card type", camera_card_type_val, allocator);
                camera_device_value.AddMember("formats", capture_type_array, allocator);
            }

            // if (camera_device_value.ObjectEmpty()) {
            //     cout<<"empty"<<endl;
            // }

        } /* if more lines for this device node */

        /* only add dev nodes with VIDIOC_ENUM_FMT */
        if (!camera_device_value.ObjectEmpty()) {
            camera_array.PushBack(camera_device_value, allocator);
        }

    } /* while (device_list_ss >> device_node) */

    camera_doc_.AddMember("cameras", camera_array, allocator);

    if (camera_doc_.MemberCount()) {
        detected_ = true;
    }

    // {
        // online schema generator
        // https://www.liquid-technologies.com/online-json-to-schema-converter

        // schema online validator
        // https://www.jsonschemavalidator.net/


        /* test schema */

        // /* Compile a Document to SchemaDocument */
        // SchemaDocument schema(camera_doc_);

        // Document d;

        // /* construct a SchemaValidator */
        // SchemaValidator validator(schema);
        // // if (!d.Accept(validator)) {
        // if (!camera_doc_.Accept(validator)) {
        //     // Input JSON is invalid according to the schema
        //     // Output diagnostic information
        //     StringBuffer sb;
        //     validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
        //     printf("Invalid schema: %s\n", sb.GetString());
        //     printf("Invalid keyword: %s\n", validator.GetInvalidSchemaKeyword());
        //     sb.Clear();
        //     validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
        //     printf("Invalid document: %s\n", sb.GetString());
        // } else {
        //     cout<<"schema passes *************"<<endl;

        //     StringBuffer sb;
        //     // sb.Clear();
        //     // PrettyWriter<StringBuffer> writer(sb);
        //     Writer<StringBuffer> writer(sb);
        //     camera_doc_.Accept(writer);
        //     puts(sb.GetString());
        // }
    // } 

    // // valid schema
    // {
    //     rapidjson::Document document;
    //     document.Parse(g_schema);

    //     rapidjson::SchemaDocument schemaDocument(document);
    //     rapidjson::SchemaValidator validator(schemaDocument);

    //     /* parse JSON string */
    //     rapidjson::Document modelDoc;
    //     modelDoc.Parse(sb.GetString());

    //     if (!modelDoc.Accept(validator)) {
    //         cout<<"schema invalidated"<<endl;
    //     } else {
    //         cout<<"schema validated"<<endl;
    //     }
    // }

    // // invalid schema
    // {
    //     rapidjson::Document document;
    //     document.Parse(g_schema_invalid);

    //     rapidjson::SchemaDocument schemaDocument(document);
    //     rapidjson::SchemaValidator validator(schemaDocument);

    //     rapidjson::Document modelDoc;
    //     modelDoc.Parse(sb.GetString());

    //     if (!modelDoc.Accept(validator)) {
    //         cout<<"schema invalidated"<<endl;
    //     } else {
    //         cout<<"schema validated"<<endl;
    //     }
    // }

    // {
    //     /* Compile a Document to SchemaDocument */
    //     SchemaDocument schema(document);

    //     rapidjson::Document sd;
        
    //     schema.GetRoot
    //     sd.Parse(schemaJson.c_str());
    // }

    return detected_;
}

string Camera::GetSupportedVideoModes() 
{
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    camera_doc_.Accept(writer);

    return string(sb.GetString(), sb.GetSize());

    // ENTER_(WEBCAM_TAG);

//     char buf[1024];

//     if (camera_device_handle_) {

//         StringBuffer buffer;
//         Writer<StringBuffer> writer(buffer);

//         writer.StartObject();
//         {
//             if (camera_device_handle_->info->stream_ifs) {
//                 uvc_streaming_interface_t* stream_if;
//                 int stream_idx = 0;

//                 writer.String("formats");
//                 writer.StartArray();
//                 DL_FOREACH(camera_device_handle_->info->stream_ifs, stream_if)
//                 {
//                     ++stream_idx;
//                     uvc_format_desc_t* fmt_desc;
//                     uvc_frame_desc_t* frame_desc;
//                     DL_FOREACH(stream_if->format_descs, fmt_desc)
//                     {
//                         writer.StartObject();
//                         {
//                             switch (fmt_desc->bDescriptorSubtype) {
//                                 case UVC_VS_FORMAT_UNCOMPRESSED:
//                                 case UVC_VS_FORMAT_MJPEG:
//                                     // only ascii fourcc codes currently accepted
//                                     if ( isprint(fmt_desc->fourccFormat[0]) && isprint(fmt_desc->fourccFormat[1]) &&
//                                          isprint(fmt_desc->fourccFormat[2]) && isprint(fmt_desc->fourccFormat[3]) ) {

//                                         writer.String("index");
//                                         writer.Uint(fmt_desc->bFormatIndex);
//                                         writer.String("type");
//                                         writer.Int(fmt_desc->bDescriptorSubtype);
//                                         writer.String("default");
//                                         writer.Uint(fmt_desc->bDefaultFrameIndex);
//                                         writer.String("fourcc");
//                                         buf[0] = fmt_desc->fourccFormat[0];
//                                         buf[1] = fmt_desc->fourccFormat[1];
//                                         buf[2] = fmt_desc->fourccFormat[2];
//                                         buf[3] = fmt_desc->fourccFormat[3];
//                                         buf[4] = '\0';
//                                         writer.String(buf);
//                                         writer.String("sizes");
//                                         writer.StartArray();
//                                         DL_FOREACH(fmt_desc->frame_descs, frame_desc) {
//                                             writer.StartObject();
//                                             writer.Key("resolution");
//                                             snprintf(buf, sizeof(buf), "%dx%d",
//                                                      frame_desc->wWidth, frame_desc->wHeight);
//                                             buf[sizeof(buf) - 1] = '\0';
//                                             writer.String(buf);
//                                             writer.Key("default_frame_interval");
//                                             writer.Uint(frame_desc->dwDefaultFrameInterval);
//                                             writer.Key("interval_type");
//                                             writer.Uint(frame_desc->bFrameIntervalType);
//                                             writer.Key("interval_type_array");
//                                             writer.StartArray();
//                                             if (frame_desc->bFrameIntervalType) {
//                                                 for (auto i = 0; i < frame_desc->bFrameIntervalType; i++) {
//                                                     writer.Uint((frame_desc->intervals)[i]);
//                                                 }
//                                             }
//                                             writer.EndArray();
//                                             writer.EndObject();
//                                         }
//                                         writer.EndArray();
//                                     }
//                                     break;
//                                 default:
//                                     break;
//                             }
//                         }
//                         writer.EndObject();
//                     }
//                 }
//                 writer.EndArray();
//             }
//         }
//         writer.EndObject();
//         return strdup(buffer.GetString());

//     } else {
//         return nullptr;
//     }
}





#define V4L_BUFFERS_DEFAULT 8
#define V4L_BUFFERS_MAX     32
int Camera::UvcV4l2Init(/*IFrameQueue* frame_queue_ifc, */std::string device_node, int enumerated_width, int enumerated_height/*, int actual_width, int actual_height*/)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    int ret;

    /* Options parsings */
    unsigned int capabilities = V4L2_CAP_VIDEO_CAPTURE;
    //unused int do_set_time_per_frame = 1;
    //unused int do_capture = 0;
    const struct V4l2FormatInfo *info;
    //unused int do_set_format = 0;
    //unused int no_query = 0; 

    /* Video buffers */
    enum v4l2_memory memtype = V4L2_MEMORY_MMAP;
    unsigned int pixelformat = V4L2_PIX_FMT_YUYV;
    unsigned int stride = 0;
    unsigned int quality = (unsigned int)-1;
    unsigned int userptr_offset = 0;
    unsigned int nbufs = V4L_BUFFERS_DEFAULT;
    //unsigned int buffer_size = 0;
    enum v4l2_field field = V4L2_FIELD_ANY;
    struct v4l2_fract time_per_frame = {1, 30};
    // struct v4l2_fract time_per_frame = {1, 5};

    /* Capture loop */
    BufferFillMode fill_mode = BUFFER_FILL_NONE;
    const char *filename = "frame-#.bin";

    VideoInit(&device_);

    string f_optarg;
    /* fourcc */
    // TODO HC for now, enventually MPEG and others will be supported as enumerated in JSON
    f_optarg = "YUYV";

    info = V4l2FormatByName(f_optarg.c_str());
    if (info == NULL) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Unsupported video format "+f_optarg);
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return 1;
    }
    pixelformat = info->fourcc;
    synclog_->LogV("fourcc : "+f_optarg);

    pixelformat = info->fourcc;
    synclog_->LogV("enumerated_height : ",to_string(enumerated_height)
        +", enumerated_width : ",to_string(enumerated_width));


    /* number of v4l2 buffers in kernel queue */
    nbufs = 4;
    if (nbufs > V4L_BUFFERS_MAX) nbufs = V4L_BUFFERS_MAX;

    /* video device node */
    if (!VideoHasFd(&device_)) {
        ret = VideoOpen(&device_, device_node.c_str());
            /* uvc camera device node at /dev/video<x> */
        if (ret < 0) {
            synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
            return 1;
        } else {
            synclog_->Log("video_has_fd() success");
        }
    }

    /* query capabilities */
    ret = VideoQueryCap(&device_, &capabilities);
    if (ret < 0) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return 1;
    }

    /* capture, output, ... */
    ret = CapGetBufType(capabilities);
    if (ret < 0) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return 1;
    }
    
    if (!VideoHasValidBufType(&device_))
        VideoSetBufType(&device_, (v4l2_buf_type)ret);

    device_.memtype = memtype;

    /* Set the video format. */
    if (VideoSetFormat(&device_, enumerated_width, enumerated_height, pixelformat, stride, field) < 0) {
        VideoClose(&device_);
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return 1;
    }

    // +++++ queue management of native colorspace frames
    // TODO 
    uvc_frame_.data_bytes = 2 * enumerated_width * enumerated_height;
    uvc_frame_.actual_bytes = 2 * enumerated_width * enumerated_height;
    uvc_frame_.width = enumerated_width;
    uvc_frame_.height = enumerated_height;
    uvc_frame_.frame_format = CAMERA_FRAME_FORMAT_YUYV;
    uvc_frame_.step = 0;
    uvc_frame_.sequence = 0;
    uvc_frame_.capture_time.tv_sec = 0;
    uvc_frame_.capture_time.tv_usec = 0;
    uvc_frame_.data = new uint8_t[uvc_frame_.data_bytes];

    rgb_frame_.data_bytes = 3 * enumerated_width * enumerated_height;
    rgb_frame_.actual_bytes = 3 * enumerated_width * enumerated_height;
    rgb_frame_.width = enumerated_width;
    rgb_frame_.height = enumerated_height;
    rgb_frame_.frame_format = CAMERA_FRAME_FORMAT_RGB;
    rgb_frame_.step = 0;
    rgb_frame_.sequence = 0;
    rgb_frame_.capture_time.tv_sec = 0;
    rgb_frame_.capture_time.tv_usec = 0;
    rgb_frame_.data = new uint8_t[rgb_frame_.data_bytes];
    // ----- queue management of native colorspace frames

    VideoGetFormat(&device_);

    /* Set the frame rate. */
    if (VideoSetFramerate(&device_, &time_per_frame) < 0) {
        VideoClose(&device_);
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return 1;
    }

    /* Set the compression quality. */
    if (VideoSetQuality(&device_, quality) < 0) {
        VideoClose(&device_);
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return 1;
    }

    if (VideoPrepareCapture(&device_, nbufs, userptr_offset, filename, fill_mode)) {
        VideoClose(&device_);
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return 1;
    }

    /* Start streaming. */
    ret = VideoEnable(&device_, 1);
    if (ret < 0) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","video_enable() failure");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        VideoFreeBuffers(&device_);
        return 1;
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::UvcV4l2Exit(void) {

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    /* signal stop streaming */

    /* joing */

    /* Stop streaming. */
    // VideoEnable(&device_, 0);

    VideoFreeBuffers(&device_);

    VideoClose(&device_);

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");

    return 0;
}


void Camera::VideoInit(Device *device)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    memset(device, 0, sizeof(Device));
    device->fd = -1;

    device->memtype = V4L2_MEMORY_MMAP;
    device->buffers = NULL;
    device->type = (enum v4l2_buf_type)-1;
    synclog_->LogV("dev->type = "+to_string(device->type));

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
}


bool Camera::VideoIsCapture(Device *dev)
{
    return dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
           dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE;
}


bool Camera::VideoIsOutput(Device* device)
{
    return device->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
           device->type == V4L2_BUF_TYPE_VIDEO_OUTPUT;
}


bool Camera::VideoIsMplane(Device *dev)
{
    return dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
           dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
}


bool Camera::VideoHasFd(Device* device)
{
    return device->fd != -1;
}


int Camera::VideoOpen(Device* device, const char *devname)
{
    if (VideoHasFd(device)) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Can't open device (already open)");
        return -1;
    }

    /* obtain camera device node file descriptor */
    device->fd = open(devname, O_RDWR);
    if (device->fd < 0) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Error opening device "+string(devname)+": "+strerror(errno)+" ("+to_string(errno)+")");
        return device->fd;
    }

    synclog_->LogV("Device "+string(devname)+" opened");

    device->opened = 1;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit ");
    return 0;
}


bool Camera::VideoHasValidBufType(Device *device)
{
    return (int)device->type != -1;
}


void Camera::VideoSetBufType(Device *device, enum v4l2_buf_type type)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    synclog_->Log("dev->type = "+to_string(type));

    device->type = type;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
}


int Camera::VideoSetFormat(Device* device, unsigned int w, unsigned int h,
    unsigned int format, unsigned int stride,
    enum v4l2_field field)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    struct v4l2_format fmt;
    unsigned int i;
    int ret;

    memset(&fmt, 0, sizeof fmt);
    fmt.type = device->type;

    if (VideoIsMplane(device)) {

        const V4l2FormatInfo *info = V4l2FormatByFourCC(format);

        fmt.fmt.pix_mp.width = w;
        fmt.fmt.pix_mp.height = h;
        fmt.fmt.pix_mp.pixelformat = format;
        fmt.fmt.pix_mp.field = field;
        /* even though we are mplane format we will use just 1 plane */
        fmt.fmt.pix_mp.num_planes = info->n_planes;

        for (i = 0; i < fmt.fmt.pix_mp.num_planes; i++)
            fmt.fmt.pix_mp.plane_fmt[i].bytesperline = stride;
    } else {
        fmt.fmt.pix.width = w;
        fmt.fmt.pix.height = h;
        fmt.fmt.pix.pixelformat = format;
        fmt.fmt.pix.field = field;
        fmt.fmt.pix.bytesperline = stride;
    }

    synclog_->Log("***** VIDIOC_S_FMT *****");
    
    ret = ioctl(device->fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        synclog_->LogV("Unable to set format: "+string(strerror(errno))+" ("+to_string(errno)+")");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }

    if (VideoIsMplane(device)) {
        stringstream ss;
        ss<<"Video format set: "<<V4l2FormatName(fmt.fmt.pix_mp.pixelformat);
        ss<<"("<<hex<<"0x"<<fmt.fmt.pix_mp.pixelformat<<") ";
        ss<<dec<<fmt.fmt.pix_mp.width<<"x"<<fmt.fmt.pix_mp.height;
        ss<<", field "<< V4l2FieldName((v4l2_field)fmt.fmt.pix_mp.field);
        ss<<", colorspace : "<<hex<<"0x"<<fmt.fmt.pix_mp.colorspace;
        ss<<", planes : "<<fmt.fmt.pix_mp.num_planes;
        synclog_->Log(ss.str());

        for (i = 0; i < fmt.fmt.pix_mp.num_planes; i++) {
            synclog_->LogV("* Stride "+to_string(fmt.fmt.pix_mp.plane_fmt[i].bytesperline)+", buffer size "+to_string(fmt.fmt.pix_mp.plane_fmt[i].sizeimage));
        }

    } else {
        stringstream ss;
        ss<<"Video format set: "<<V4l2FormatName(fmt.fmt.pix_mp.pixelformat);
        ss<<"("<<hex<<"0x"<<fmt.fmt.pix_mp.pixelformat<<") ";
        ss<<dec<<fmt.fmt.pix_mp.width<<"x"<<fmt.fmt.pix_mp.height;
        ss<<", stride : "<<fmt.fmt.pix.bytesperline;
        ss<<", field "<< V4l2FieldName((v4l2_field)fmt.fmt.pix_mp.field);
        ss<<", buffer size :"<<fmt.fmt.pix.sizeimage;
        synclog_->LogV(ss.str());
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");

    return 0;
}


const V4l2FormatInfo* Camera::V4l2FormatByFourCC(unsigned int fourcc)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pixel_formats_); ++i) {
        if (pixel_formats_[i].fourcc == fourcc)
            return &pixel_formats_[i];
    }

    return NULL;
}


const V4l2FormatInfo* Camera::V4l2FormatByName(const char *name)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pixel_formats_); ++i) {
        if (strcasecmp(pixel_formats_[i].name, name) == 0)
            return &pixel_formats_[i];
    }

    return NULL;
}


const char* Camera::V4l2FieldName(enum v4l2_field field)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(fields_); ++i) {
        if (fields_[i].field == field)
            return fields_[i].name;
    }

    return "unknown";
}


void Camera::VideoClose(Device* device)
{
    unsigned int i;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    for (i = 0; i < device->num_planes; i++)
        free(device->pattern[i]);

    free(device->buffers);
    if (device->opened)
        close(device->fd);

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
}


int Camera::VideoGetFormat(Device* device)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    struct v4l2_format fmt;
    unsigned int i;
    int ret;

    memset(&fmt, 0, sizeof fmt);
    fmt.type = device->type;

    synclog_->Log("***** VIDIOC_G_FMT *****");

    ret = ioctl(device->fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
        synclog_->LogV("Unable to get format: "+string(strerror(errno))+ "("+to_string(errno)+").");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }

    if (VideoIsMplane(device)) {
        device->width = fmt.fmt.pix_mp.width;
        device->height = fmt.fmt.pix_mp.height;
        device->num_planes = fmt.fmt.pix_mp.num_planes;

        stringstream ss;
        ss<<"Video format set: "<<V4l2FormatName(fmt.fmt.pix_mp.pixelformat);
        ss<<"("<<hex<<"0x"<<fmt.fmt.pix_mp.pixelformat<<") ";
        ss<<fmt.fmt.pix_mp.width<<"x"<<fmt.fmt.pix_mp.height;
        ss<<", field "<< V4l2FieldName((v4l2_field)fmt.fmt.pix_mp.field);
        ss<<", colorspace : "<<hex<<"0x"<<fmt.fmt.pix_mp.colorspace;
        ss<<", planes : "<<fmt.fmt.pix_mp.num_planes;
        synclog_->Log(ss.str());

        for (i = 0; i < fmt.fmt.pix_mp.num_planes; i++) {
            device->plane_fmt[i].bytesperline =
                    fmt.fmt.pix_mp.plane_fmt[i].bytesperline;
            device->plane_fmt[i].sizeimage =
                    fmt.fmt.pix_mp.plane_fmt[i].bytesperline ?
                        fmt.fmt.pix_mp.plane_fmt[i].sizeimage : 0;

            synclog_->LogV(" * Stride "+to_string(fmt.fmt.pix_mp.plane_fmt[i].bytesperline)+", buffer size "+to_string(fmt.fmt.pix_mp.plane_fmt[i].sizeimage));
        }
    } else {
        device->width = fmt.fmt.pix.width;
        device->height = fmt.fmt.pix.height;
        device->num_planes = 1;

        device->plane_fmt[0].bytesperline = fmt.fmt.pix.bytesperline;
        device->plane_fmt[0].sizeimage = fmt.fmt.pix.bytesperline ? fmt.fmt.pix.sizeimage : 0;

        stringstream ss;
        ss<<"Video format set: "<<V4l2FormatName(fmt.fmt.pix_mp.pixelformat);
        ss<<"("<<hex<<"0x"<<fmt.fmt.pix_mp.pixelformat<<") ";
        ss<<dec<<fmt.fmt.pix_mp.width<<"x"<<fmt.fmt.pix_mp.height;
        ss<<", stride : "<<fmt.fmt.pix.bytesperline;
        ss<<", field "<< V4l2FieldName((v4l2_field)fmt.fmt.pix_mp.field);
        ss<<", buffer size :"<<fmt.fmt.pix.sizeimage;
        synclog_->Log(ss.str());
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::VideoSetFramerate(Device* device, v4l2_fract* time_per_frame)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");
    
    struct v4l2_streamparm parm;
    int ret;

    memset(&parm, 0, sizeof parm);
    parm.type = device->type;

    ret = ioctl(device->fd, VIDIOC_G_PARM, &parm);
    if (ret < 0) {
        synclog_->Log("Unable to get frame rate: "+string(strerror(errno))+" ("+to_string(errno)+")");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }

    synclog_->Log("Current frame rate: "+to_string(parm.parm.capture.timeperframe.numerator)+"/"+to_string(parm.parm.capture.timeperframe.denominator));

    parm.parm.capture.timeperframe.numerator = time_per_frame->numerator;
    parm.parm.capture.timeperframe.denominator = time_per_frame->denominator;

    ret = ioctl(device->fd, VIDIOC_S_PARM, &parm);
    if (ret < 0) {
        synclog_->Log("Unable to set frame rate: "+string(strerror(errno))+" ("+to_string(errno)+")");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }

    ret = ioctl(device->fd, VIDIOC_G_PARM, &parm);
    if (ret < 0) {
        synclog_->Log("Unable to get frame rate: "+string(strerror(errno))+" ("+to_string(errno)+")");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit\n");
        return ret;
    }

    synclog_->Log("Frame rate set : "+to_string(parm.parm.capture.timeperframe.numerator)+"/"+to_string(parm.parm.capture.timeperframe.denominator));
    
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::VideoSetQuality(Device* device, unsigned int quality)
{

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    v4l2_jpegcompression jpeg;
    int ret;

    if (quality == (unsigned int)-1) {
        synclog_->Log(to_string(__LINE__)+" > invalid quality setting : "+to_string(quality));
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return 0;
    }

    memset(&jpeg, 0, sizeof jpeg);
    jpeg.quality = quality;

    /*
        https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/vidioc-g-jpegcomp.html?highlight=vidioc_s_jpegcomp

            These ioctls are deprecated. New drivers and applications should use JPEG class controls for image quality and JPEG markers control.
    */
    synclog_->Log("***** VIDIOC_S_JPEGCOMP *****");

    ret = ioctl(device->fd, VIDIOC_S_JPEGCOMP, &jpeg);
    if (ret < 0) {
        synclog_->Log("Unable to set quality to "+to_string(quality)+": "+strerror(errno)+" ("+to_string(errno)+")");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }

    synclog_->Log("***** VIDIOC_G_JPEGCOMP *****");

    ret = ioctl(device->fd, VIDIOC_G_JPEGCOMP, &jpeg);
    if (ret >= 0)
        synclog_->Log("Quality set to "+to_string(jpeg.quality));

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::VideoPrepareCapture(Device*device, 
    int nbufs, 
    unsigned int offset,
    const char *filename, 
    enum BufferFillMode fill)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");
    (void)filename;

    unsigned int padding;
    unsigned int i;
    int ret;

    /* Allocate and map buffers. */
    padding = (fill & BUFFER_FILL_PADDING) ? 4096 : 0;
    if ((ret = VideoAllocBuffers(device, nbufs, offset, padding)) < 0) {
        synclog_->Log("video_alloc_buffers() failure");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }


    if (VideoIsOutput(device)) {
    //     ret = video_load_test_pattern(dev, filename);
    //     if (ret < 0) {
    //         LOG("video_load_test_pattern() failure\n");
    //         LOG("exit\n");
    //         return ret;
    //     }
    // TODO remove support for video output
        synclog_->Log("VIDEO OUTPUT NOT SUPPORTED");
    }

    /* Queue the buffers. */
    for (i = 0; i < device->nbufs; ++i) {
        ret = VideoQueueBuffer(device, i, fill);
        if (ret < 0) {
            synclog_->Log("video_queue_buffer() failure");
            synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
            return ret;
        }
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::VideoAllocBuffers(Device* device, int nbufs, unsigned int offset, unsigned int padding)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    struct v4l2_requestbuffers rb;
    struct v4l2_buffer buf;
    struct buffer *buffers;
    unsigned int i;
    int ret;

    memset(&rb, 0, sizeof rb);
    rb.count = nbufs;
    rb.type = device->type;
    rb.memory = device->memtype;

    ret = ioctl(device->fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        synclog_->Log("Unable to request buffers: "+string(strerror(errno))+" ("+to_string(errno)+")");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }

    synclog_->Log(to_string(rb.count)+" buffers requested.");

    buffers = (buffer*)malloc(rb.count * sizeof buffers[0]);
    if (buffers == NULL) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return -ENOMEM;
    }

    /* Map the buffers. */
    for (i = 0; i < rb.count; ++i) {
        const char *ts_type, *ts_source;

        memset(&buf, 0, sizeof buf);
        memset(planes, 0, sizeof planes);

        buf.index = i;
        buf.type = device->type;
        buf.memory = device->memtype;
        buf.length = VIDEO_MAX_PLANES;
        buf.m.planes = planes;

        synclog_->Log(to_string(__LINE__)+" > ***** VIDIOC_QUERYBUF *****");

        ret = ioctl(device->fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            synclog_->Log("Unable to query buffer "+to_string(i)+": "+string(strerror(errno))+")"+to_string(errno)+")");
            synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
            return ret;
        }
        GetTsFlags(buf.flags, &ts_type, &ts_source);
        synclog_->Log("length: "+to_string(buf.length)+"offset: "+to_string(buf.m.offset)+
            "timestamp type/source: "+string(ts_type)+"/"+ts_source);

        buffers[i].idx = i;

        switch (device->memtype) {
        case V4L2_MEMORY_MMAP:
            ret = VideoBufferMmap(device, &buffers[i], &buf);
            break;

        case V4L2_MEMORY_USERPTR:
            ret = VideoBufferAllocUserptr(device, &buffers[i], &buf, offset, padding);
            break;

        default:
            break;
        }

        if (ret < 0)
            return ret;
    }

    device->timestamp_type = buf.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK;
    device->buffers = buffers;
    device->nbufs = rb.count;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


void Camera::GetTsFlags(uint32_t flags, const char **ts_type, const char **ts_source)
{
    switch (flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) {
    case V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN:
        *ts_type = "unk";
        break;
    case V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC:
        *ts_type = "mono";
        break;
    case V4L2_BUF_FLAG_TIMESTAMP_COPY:
        *ts_type = "copy";
        break;
    default:
        *ts_type = "inv";
    }
    switch (flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK) {
    case V4L2_BUF_FLAG_TSTAMP_SRC_EOF:
        *ts_source = "EoF";
        break;
    case V4L2_BUF_FLAG_TSTAMP_SRC_SOE:
        *ts_source = "SoE";
        break;
    default:
        *ts_source = "inv";
    }
}


int Camera::VideoBufferMmap(Device* dev, struct buffer* buffer, struct v4l2_buffer* v4l2buf)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    unsigned int length;
    unsigned int offset;
    unsigned int i;

    for (i = 0; i < dev->num_planes; i++) {
        if (VideoIsMplane(dev)) {
            length = v4l2buf->m.planes[i].length;
            offset = v4l2buf->m.planes[i].m.mem_offset;
        } else {
            length = v4l2buf->length;
            offset = v4l2buf->m.offset;
        }

        synclog_->Log("***** mmap *****");

        buffer->mem[i] = (uint8_t*)mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED,
                      dev->fd, offset);
        if (buffer->mem[i] == MAP_FAILED) {
            synclog_->Log("Unable to map buffer "+to_string(buffer->idx)+"/"+to_string(i)+": "+strerror(errno)+"("+to_string(errno)+")");
            synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
            return -1;
        }

        buffer->size[i] = length;
        buffer->padding[i] = 0;

        synclog_->Log("Buffer "+to_string(buffer->idx)+"/"+to_string(i)+"mapped at address "+to_string(long(buffer->mem[i])));
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit\n");
    return 0;
}


int Camera::VideoBufferAllocUserptr(Device* device, struct buffer *buffer, struct v4l2_buffer *v4l2buf, unsigned int offset, unsigned int padding)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter\n");

    int page_size = getpagesize();
    unsigned int length;
    unsigned int i;
    int ret;

    for (i = 0; i < device->num_planes; i++) {
        if (VideoIsMplane(device))
            length = v4l2buf->m.planes[i].length;
        else
            length = v4l2buf->length;

        ret = posix_memalign((void**)(&buffer->mem[i]), page_size,
                     length + offset + padding);
        if (ret < 0) {
            synclog_->Log("Unable to allocate buffer "+to_string(buffer->idx)+"/"+to_string(i)+" "+to_string(ret));
            synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
            return -ENOMEM;
        }

        buffer->mem[i] += offset;
        buffer->size[i] = length;
        buffer->padding[i] = padding;

        synclog_->Log("Buffer "+to_string(buffer->idx)+"/"+to_string(i)+"allocated at address "+to_string(long(buffer->mem[i])));
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::VideoQueueBuffer(Device* device, int index, BufferFillMode fill)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;
    unsigned int i;

    memset(&buf, 0, sizeof buf);
    buf.index = index;
    buf.type = device->type;
    buf.memory = device->memtype;

    if (VideoIsOutput(device)) {
        buf.flags = device->buffer_output_flags;
        if (device->timestamp_type == V4L2_BUF_FLAG_TIMESTAMP_COPY) {
            struct timespec ts;

            clock_gettime(CLOCK_MONOTONIC, &ts);
            buf.timestamp.tv_sec = ts.tv_sec;
            buf.timestamp.tv_usec = ts.tv_nsec / 1000;
        }
    }

    if (VideoIsMplane(device)) {
        buf.m.planes = planes;
        buf.length = device->num_planes;
    } else {
        buf.length = device->buffers[index].size[0];
    }

    if (device->memtype == V4L2_MEMORY_USERPTR) {
        if (VideoIsMplane(device)) {
            for (i = 0; i < device->num_planes; i++)
                buf.m.planes[i].m.userptr = (unsigned long)device->buffers[index].mem[i];
        } else {
            buf.m.userptr = (unsigned long)device->buffers[index].mem[0];
        }
    }

    for (i = 0; i < device->num_planes; i++) {
        if (VideoIsOutput(device)) {
            if (VideoIsMplane(device))
                buf.m.planes[i].bytesused = device->patternsize[i];
            else
                buf.bytesused = device->patternsize[i];

            memcpy(device->buffers[buf.index].mem[i], device->pattern[i],
                   device->patternsize[i]);
        } else {
            if (fill & BUFFER_FILL_FRAME)
                memset(device->buffers[buf.index].mem[i], 0x55,
                       device->buffers[index].size[i]);
            if (fill & BUFFER_FILL_PADDING)
                memset(device->buffers[buf.index].mem[i] +
                    device->buffers[index].size[i],
                       0x55, device->buffers[index].padding[i]);
        }
    }

    ret = ioctl(device->fd, VIDIOC_QBUF, &buf);
    if (ret < 0)
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Unable to queue buffer: "+string(strerror(errno))+" ("+to_string(errno)+")");

    return ret;
}


int Camera::VideoEnable(Device* device, int enable)
{
    int type = device->type;
    int ret;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    synclog_->Log("***** "+string((enable ? "VIDIOC_STREAMON" : "VIDIOC_STREAMOFF"))+" *****");
    
    ret = ioctl(device->fd, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Unable to %s streaming: "+string(enable ? "start" : "stop")+" "+ strerror(errno)+" ("+to_string(errno)+")");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::VideoFreeBuffers(Device* device)
{
    struct v4l2_requestbuffers rb;
    unsigned int i;
    int ret;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    if (device->nbufs == 0)
        return 0;

    for (i = 0; i < device->nbufs; ++i) {
        switch (device->memtype) {
        case V4L2_MEMORY_MMAP:
            ret = VideoBufferMunmap(device, &device->buffers[i]);
            if (ret < 0)
                return ret;
            break;
        case V4L2_MEMORY_USERPTR:
            VideoBufferFreeUserptr(device, &device->buffers[i]);
            break;
        default:
            break;
        }
    }

    memset(&rb, 0, sizeof rb);
    rb.count = 0;
    rb.type = device->type;
    rb.memory = device->memtype;

    ret = ioctl(device->fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Unable to release buffers: "+string(strerror(errno))+" ("+to_string(errno)+")");
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
        return ret;
    }

    synclog_->Log(to_string(device->nbufs)+" buffers released");

    free(device->buffers);
    device->nbufs = 0;
    device->buffers = NULL;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::VideoBufferMunmap(Device* device, buffer* buffer)
{
    unsigned int i;
    int ret;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    for (i = 0; i < device->num_planes; i++) {
        ret = munmap(buffer->mem[i], buffer->size[i]);
        if (ret < 0) {
            synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Unable to unmap buffer "+to_string(buffer->idx)+"/"+to_string(i)+": "+strerror(errno)+" ("+to_string(errno)+")");
        }

        buffer->mem[i] = NULL;
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


void Camera::VideoBufferFreeUserptr(Device* device, buffer* buffer)
{
    unsigned int i;

    for (i = 0; i < device->num_planes; i++) {
        free(buffer->mem[i]);
        buffer->mem[i] = NULL;
    }
}


// int Camera::VideoQueryCap(Device* device, unsigned int* capabilities)
// {
//     SyncLog* sync_log = SyncLog::GetLog();

//     struct v4l2_capability cap;
//     int ret;

//     sync_log->Log("enter "+string(__func__));
//     sync_log->Log(__LINE__+" > ***** VIDIOC_QUERYCAP *****");

//     memset(&cap, 0, sizeof cap);
//     ret = ioctl(device->fd, VIDIOC_QUERYCAP, &cap);
//     if (ret < 0) {
//         sync_log->Log("exit "+string(__func__));
//         return 0;
//     }

//     sync_log->Log(__LINE__+" > cap.capabilities : "+cap.capabilities);

//     *capabilities = cap.capabilities & V4L2_CAP_DEVICE_CAPS ? cap.device_caps : cap.capabilities;

//     sync_log->Log(__LINE__+" > *capabilities : "+*capabilities);

//     LOG("Device `%s' on `%s' is a video %s (%s mplanes) device.\n",
//         cap.card, cap.bus_info,
//         video_is_capture(dev) ? "capture" : "output",
//         video_is_mplane(dev) ? "with" : "without");

//     LOG("exit\n");

//     return 0;    
// }


// int Camera::QueryCapabilities(Device *dev, unsigned int *capabilities)
int Camera::VideoQueryCap(Device* device, unsigned int *capabilities)
{
    stringstream ss;
    struct v4l2_capability cap;
    int ret;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");
    synclog_->Log("ioctl VIDIOC_QUERYCAP");

    /* https://www.kernel.org/doc/html/v4.15/media/uapi/v4l/vidioc-querycap.html
       TODO capture capabilities in object
    */

    memset(&cap, 0, sizeof cap);
    ret = ioctl(device->fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","ioctl error "+to_string(ret)+": "+strerror(errno));
        return 0;
    }

    ss<<"cap.capabilities : "<<hex<<"0x"<<cap.capabilities;
    synclog_->Log(ss.str());
    ss.str(string());
    ss<<"cap.device_caps : "<<hex<<"0x"<<cap.device_caps;
    synclog_->Log(ss.str());

    *capabilities = cap.capabilities & V4L2_CAP_DEVICE_CAPS ? cap.device_caps : cap.capabilities;

    ss.str(string());
    ss<<"*capabilities : "<<hex<<"0x"<<*capabilities;
    synclog_->Log(ss.str());

    string cap_card_str((char*)(cap.card), sizeof(cap.card)/sizeof(uint8_t));
    string cap_bus_info_str((char*)(cap.bus_info), sizeof(cap.bus_info)/sizeof(uint8_t));
    synclog_->Log("Device "+cap_card_str+" on "+cap_bus_info_str+" is a video "+
        (VideoIsCapture(device) ? "capture" : "output")+" ("+ (VideoIsMplane(device) ? "with" : "without")+
        " mplanes device)");

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


int Camera::CapGetBufType(unsigned int capabilities)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","enter");

    /* select the capability to use */
    if (capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
        synclog_->Log("V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE\nexit");
        return V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    } else if (capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE) {
        synclog_->Log("V4L2_CAP_VIDEO_OUTPUT_MPLANE\exit");
        return V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    } else if (capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        synclog_->Log("V4L2_BUF_TYPE_VIDEO_CAPTURE\nexit");
        return  V4L2_BUF_TYPE_VIDEO_CAPTURE;
    } else if (capabilities & V4L2_CAP_VIDEO_OUTPUT) {
        synclog_->Log("V4L2_CAP_VIDEO_OUTPUT\nexit");
        return V4L2_BUF_TYPE_VIDEO_OUTPUT;
    } else {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Device supports neither capture nor output.\nexit");
        return -EINVAL;
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


const char* Camera::V4l2FormatName(unsigned int fourcc)
{
    const V4l2FormatInfo *info;
    static char name[5];
    unsigned int i;

    info = V4l2FormatByFourCC(fourcc);
    if (info)
        return info->name;

    for (i = 0; i < 4; ++i) {
        name[i] = fourcc & 0xff;
        fourcc >>= 8;
    }

    name[4] = '\0';
    return name;
}

// int WebCam::SetPreviewSize(int width, int height, int min_fps, int max_fps, CameraFormat camera_format) {
//     ENTER_(WEBCAM_TAG);

//     int result = EXIT_FAILURE;
//     if (webcam_fifo_) {
//         result = webcam_fifo_->SetFrameSize(width, height, min_fps, max_fps, camera_format);
//     }

//     RETURN_(WEBCAM_TAG, result, int);
// }


// IFrameAccessRegistration* WebCam::GetFrameAccessIfc(int interface_number) {
//     ENTER_(WEBCAM_TAG);

//     IFrameAccessRegistration* camera_frame_access_registration = webcam_fifo_;

//     PRE_EXIT_(WEBCAM_TAG);
//     return camera_frame_access_registration;
// }

// string exec(const char* cmd) {
//     array<char, 256> buffer;
//     string result;
//     shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
//     if (!pipe) throw runtime_error("popen() failed!");
//     while (!feof(pipe.get())) {
//         if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
//             result += buffer.data();
//     }
//     return result;
// }


bool Camera::GetFrame(CameraFrame** frame)
{
    *frame = camera_frame_queue_->WaitForFrame(true);

    // TODO false or other EOS signal here ?
    return true;
}
bool Camera::ReturnFrame(CameraFrame* frame)
{
    camera_frame_queue_->ReturnPoolFrame(frame);

    return true;
}


void Camera::Release() 
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    /* block for client release of frame queue interface */
    std::unique_lock<std::mutex> lk(release_frame_queue_mtx_);
    release_frame_queue_ = true;
    release_frame_queue_cv.notify_one();

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
}


bool Camera::GetFrame(uint8_t** frame)
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    // TODO replace with circ buffer tail read and release
    unique_lock<std::mutex> lk(frame_avail_mtx_);
    frame_avail_cv_.wait(lk, [this]{return frame_avail_;});
    *frame = rgb_frame_.data;

    frame_avail_ = false;
    lk.unlock();

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");

    // TODO false or other EOS signal here ?
    return true;
}


// thread worker method
void Camera::FramePull(__attribute__((unused)) int width, __attribute__((unused)) int height)
{
    request_start_ = false;
    is_running_ = true;

    /* test v4l2 frame pull */
    // temp test 5 seconds of 30 fps
    // long num_frames_to_capture = 30;//5*30;
    long num_frames_to_capture = 0;

    while (IsRunning()) {

        int get_frame_retval = UvcV4l2GetFrame();

        // terminated v4l2 ioctl return is 22
        if (22 == get_frame_retval) {
            SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","UvcV4l2GetFrame() failure");
            // break;
            is_running_ = false;
            // TODO validate v4l2 termination errno
        }
        {
            lock_guard<std::mutex> lk(frame_avail_mtx_);
            frame_avail_ = true;
        }
        frame_avail_cv_.notify_one();

        SyncLog::GetLog()->LogV("frame # : ", num_frames_to_capture++);

        // if (num_frames_to_capture > 0) {
        //     num_frames_to_capture--;
        // } else {
        //     SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","num_frames_to_capture == 0");
        // }

        // if (0 == num_frames_to_capture) {
        //     /* Stop streaming. */
        //     if (!Stop()) {
        //         SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","Stop() camera");
        //         break;
        //     } else {
        //         SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","Stop() failure");
        //     }
        // }

    }
}


int Camera::Start() 
{
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    int ret = 0;

    camera_frame_queue_ = make_shared<FrameQueue<CameraFrame>>();

    if (!IsRunning()) {

// TODO not terminated correctly
#if 0
        // create promise ... can be issued anywhere in a context ... not just return value
        // vector<future<shared_ptr<Output>>> future_vec;

        // future from async
        for (int64_t instance = 0; instance < 16; instance++) {
            auto input = make_shared<Input>(instance % 0x10);
            input->load_ = instance;
            future<shared_ptr<Output>> future_0 = std::async(async_function, input);
            future_vec_.push_back(move(future_0));
        }

        // run some other routines ...
        //other_routine(1);

        for (auto& elem : future_vec_) {
            auto val = elem.get();
            synclog_->Log("got instance: "+std::to_string(val->instance_));
            val = nullptr;
        }
#endif

        int enumerated_width{640};
        int enumerated_height{480};

        camera_frame_queue_->Init(8, enumerated_width*enumerated_height*3);

        // TODO dev node and camera config params to be passed in
        string device_node_string{"/dev/video0"};
        if (!UvcV4l2Init(device_node_string, enumerated_width, enumerated_height)) {
            cout<<"UvcV4l2Init() success"<<endl;

            // TODO enum into post_request_
            request_start_ = true;

            // Start a new worker thread.
            frame_pull_thread_ = thread(
            [this]
            {
                FramePull(width_, height_);
            });

        } else {
            cout<<"UvcV4l2Init() failure"<<endl;
        }
    } else {
        SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","stop camera before starting");
        ret = -1;
    }

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return ret;
}


int Camera::Stop() {
    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","entry");

    // if (camera_device_handle_) {
    //     webcam_fifo_->StopFrameAcquisition();

    //     SAFE_DELETE(webcam_fifo_);
    //     uvc_close(camera_device_handle_);
    //     camera_device_handle_ = nullptr;
    // }

    // LOGI_(WEBCAM_TAG, "camera_device_");

    // if (camera_device_) {
    //     uvc_unref_device(camera_device_);
    //     camera_device_ = nullptr;
    // }

    // LOGI_(WEBCAM_TAG, "usb_fs_");

    // if (usb_fs_) {
    //     close(uvc_fd_);
    //     uvc_fd_ = 0;
    //     free(usb_fs_);
    //     usb_fs_ = nullptr;
    // }

    /* Stop streaming. */
    int ret;
    if (0 != (ret = VideoEnable(&device_, 0))) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","VideoEnable() failure");
        // TODO still need to shutdown pipeline
        return -1;
    }

    if (frame_pull_thread_.joinable()) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","pre join");
        frame_pull_thread_.join();
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","post join");
    }

    /* block for client release of frame queue interface */
    std::unique_lock<std::mutex> lk(release_frame_queue_mtx_);
    if (!release_frame_queue_) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","pre release_frame_queue_cv.wait()");
        release_frame_queue_cv.wait(lk);
        release_frame_queue_ = false;
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","post release_frame_queue_cv.wait()");
    }

    // TODO create method for client to release queue ... should be abstract shared ifc ptr set to nullptr which in its desctructor signal this cv_status

    // release resources
    UvcV4l2Exit();

    camera_frame_queue_ = nullptr;

    synclog_->LogV("[",__func__,": ",__LINE__,"]: ","exit");
    return 0;
}


bool Camera::IsRunning() 
{
    return is_running_;
}


int Camera::UvcV4l2GetFrame(void)
{
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    struct v4l2_buffer buf;
    int ret;

    /* Dequeue a buffer. */
    memset(&buf, 0, sizeof buf);
    memset(planes, 0, sizeof planes);

    buf.type = device_.type;
    buf.memory = device_.memtype;
    buf.length = VIDEO_MAX_PLANES;
    buf.m.planes = planes;

    ret = ioctl(device_.fd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {

        if (errno != EIO) {
        //     stringstream ss;
        // ss<<"Video format set: "<<V4l2FormatName(fmt.fmt.pix_mp.pixelformat);
        // ss<<"("<<hex<<"0x"<<fmt.fmt.pix_mp.pixelformat<<") ";
        // ss<<dec<<fmt.fmt.pix_mp.width<<"x"<<fmt.fmt.pix_mp.height;
        // ss<<", field "<< V4l2FieldName((v4l2_field)fmt.fmt.pix_mp.field);
        // ss<<", colorspace : "<<hex<<"0x"<<fmt.fmt.pix_mp.colorspace;
        // ss<<", planes : "<<fmt.fmt.pix_mp.num_planes;
        // synclog_->Log(ss.str());

            synclog_->LogV("[",__func__,": ",__LINE__,"]: "+string("Unable to dequeue buffer: ")+strerror(errno)+" ("+to_string(errno)+")");
            if (rgb_frame_.data != nullptr) {
                delete [] rgb_frame_.data;
                rgb_frame_.data = nullptr;
            }
            // rgb_frame_.data = nullptr;

            /* pall null frame to client as signal to terminate stream */
            // CameraFrame* rgb_frame = camera_frame_queue_->GetPoolFrame();
            // if (rgb_frame == nullptr) {
            //     synclog_->LogV("[",__func__,": ",__LINE__,"]: ","GetPoolFrame() failure");
            //     // TODO defer termination until frame available
            // } else {
                // rgb_frame->actual_bytes = rgb_frame->data_bytes = 0;
                // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","actual_bytes: ", rgb_frame->actual_bytes);
                // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","data_bytes: ", rgb_frame->data_bytes);
                // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","reuse ",rgb_frame->reuse_token," allocated @: ", static_cast<void*>(rgb_frame->data));
        
                // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","copy size: ",width*3*height);

                // memcpy(rgb_frame->data, rgb_frame_.data, width*3*height);
                // TODO why 2 params?
                /* signal client of stream end */
                camera_frame_queue_->AddFrameToQueue(nullptr/*rgb_frame*/, true);
            // }

            return errno;
        }
        buf.type = device_.type;
        buf.memory = device_.memtype;
        if (device_.memtype == V4L2_MEMORY_USERPTR) {
            // TODO review the use over userptr in error cases ...
            //video_buffer_fill_userptr(&dev, &(dev.buffers[i]), &buf);
        }
    }

    /* render image */
    uint8_t* buffer = (uint8_t*)(device_.buffers[buf.index].mem[0]);

    int width = camera_config.width_actual;
    int height = camera_config.height;
//    int bytes_per_pixel = camera_config.bytes_per_pixel;
//    int bytes_per_row = bytes_per_pixel * width;

    // try libuvc macros
    // uvc_error_t uvc_yuyv2rgb(uvc_frame_t *in, uvc_frame_t *out)


// TODO tmp raw v4l2 pull
#if 1
    // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","RAW V4LW BUFFER PULL");

    CameraFrame frame = uvc_frame_;
    uint8_t* yuyv_buffer = frame.data;

    // TODO place in queue
    memcpy(yuyv_buffer, buffer, width*height*2);

    // +++++ place color transform in client
    // TODO

    uint8_t *pyuv = uvc_frame_.data;
    uint8_t* prgb = rgb_frame_.data;

    // TODO allocation units for stride
    uint8_t *prgb_end = prgb + (width*3*height);

    // utilize libuvc yuv->rgb library
    while (prgb < prgb_end) {
        IYUYV2RGB_8(pyuv, prgb);

        prgb += 3 * 8;
        pyuv += 2 * 8;
    }
    // ----- place color transform in client

#if 1
    CameraFrame* rgb_frame = camera_frame_queue_->GetPoolFrame();
    if (rgb_frame == nullptr) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","GetPoolFrame() failure");
        // return -1;
    } else {
        // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","actual_bytes: ", rgb_frame->actual_bytes);
        // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","data_bytes: ", rgb_frame->data_bytes);
        // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","reuse ",rgb_frame->reuse_token," allocated @: ", static_cast<void*>(rgb_frame->data));

        // synclog_->LogV("[",__func__,": ",__LINE__,"]: ","copy size: ",width*3*height);

        memcpy(rgb_frame->data, rgb_frame_.data, width*3*height);
        // TODO why 2 params?
        camera_frame_queue_->AddFrameToQueue(rgb_frame, true);
    }
#endif
#else
    uint8_t *pyuv = buffer;
// TODO initally return yuv and decode in opencv module
// allow client to request RGB formats later ...
#if 1
    // uint8_t *prgb = g_rgb_buffer; 
    CameraFrame frame = frame_queue_ifc_->GetFrame();
    uint8_t* yuyv_buffer = frame.data;
    width = dev.width;
    height = dev.height;

    memcpy(yuyv_buffer, pyuv, width*height*2);
#else
    uint8_t *prgb = g_rgb_buffer; 
    width = frame.frame_size.width;
    height = frame.frame_size.height;
    // TODO allocation units for stride

    //uint8_t *prgb_end = prgb + out->data_bytes;
    uint8_t *prgb_end = prgb + (width*3*height);//out->data_bytes;

    while (prgb < prgb_end) {
        IYUYV2RGB_8(pyuv, prgb);

        prgb += 3 * 8;
        pyuv += 2 * 8;
    }
#endif
#endif /* TODO tmp raw v4l2 pull */

    /* signal waiting clients of frame availability */


    BufferFillMode fill_mode = BUFFER_FILL_NONE;
    ret = VideoQueueBuffer(&device_, buf.index, fill_mode);
    if (ret < 0) {
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Unable to requeue buffer: %s (%d).\n",
            strerror(errno), errno);
        return -1;
    }

    return 0;
}


int Camera::VideoQueuebuffer(/*Device* device, */int index, BufferFillMode fill)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;
    unsigned int i;

    memset(&buf, 0, sizeof buf);
    buf.index = index;
    buf.type = device_.type;
    buf.memory = device_.memtype;

    if (VideoIsOutput(&device_)) {
        buf.flags = device_.buffer_output_flags;
        if (device_.timestamp_type == V4L2_BUF_FLAG_TIMESTAMP_COPY) {
            struct timespec ts;

            clock_gettime(CLOCK_MONOTONIC, &ts);
            buf.timestamp.tv_sec = ts.tv_sec;
            buf.timestamp.tv_usec = ts.tv_nsec / 1000;
        }
    }

    if (VideoIsMplane(&device_)) {
        buf.m.planes = planes;
        buf.length = device_.num_planes;
    } else {
        buf.length = device_.buffers[index].size[0];
    }

    if (device_.memtype == V4L2_MEMORY_USERPTR) {
        if (VideoIsMplane(&device_)) {
            for (i = 0; i < device_.num_planes; i++)
                buf.m.planes[i].m.userptr = (unsigned long)device_.buffers[index].mem[i];
        } else {
            buf.m.userptr = (unsigned long)device_.buffers[index].mem[0];
        }
    }

    for (i = 0; i < device_.num_planes; i++) {
        if (VideoIsOutput(&device_)) {
            if (VideoIsMplane(&device_))
                buf.m.planes[i].bytesused = device_.patternsize[i];
            else
                buf.bytesused = device_.patternsize[i];

            memcpy(device_.buffers[buf.index].mem[i], device_.pattern[i],
                   device_.patternsize[i]);
        } else {
            if (fill & BUFFER_FILL_FRAME)
                memset(device_.buffers[buf.index].mem[i], 0x55,
                       device_.buffers[index].size[i]);
            if (fill & BUFFER_FILL_PADDING)
                memset(device_.buffers[buf.index].mem[i] +
                    device_.buffers[index].size[i],
                       0x55, device_.buffers[index].padding[i]);
        }
    }

    ret = ioctl(device_.fd, VIDIOC_QBUF, &buf);
    if (ret < 0)
        synclog_->LogV("[",__func__,": ",__LINE__,"]: ","Unable to queue buffer: %s (%d).\n", strerror(errno), errno);

    return ret;
}
