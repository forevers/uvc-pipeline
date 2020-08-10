#include "camera.h"

#include "input.h"
#include "sync-log.h"

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


// #include <ctype.h>
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


Camera::Camera() :
    synclog_(SyncLog::GetLog()),
    detected_{false}
        // : uvc_fd_(0),
        //   usb_fs_(nullptr),
        //   camera_context_(nullptr),
        //   camera_device_(nullptr),
        //   camera_device_handle_(nullptr),
        //   webcam_fifo_(nullptr),
        //   status_callback_(nullptr)
{
    synclog_->LogV(__FUNCTION__, " entry");
    // ENTER_(CAMERA_TAG);

    camera_doc_.SetObject();

    /* v4l2-ctl camera mode detection */
    if (DetectCameras()) {
        cout<<"cameras detected"<<endl;
    } else {
        cout<<"no cameras detected"<<endl;
    }
    // EXIT_(CAMERA_TAG);
}


Camera::~Camera() {
    synclog_->LogV(__FUNCTION__, " entry");

    // ENTER_(WEBCAM_TAG);

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

    // EXIT_(WEBCAM_TAG);
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

        } // if more lines for this device node

        // only add dev nodes with VIDIOC_ENUM_FMT
        if (!camera_device_value.ObjectEmpty()) {
            camera_array.PushBack(camera_device_value, allocator);
        }

    } // while (device_list_ss >> device_node)

    camera_doc_.AddMember("cameras", camera_array, allocator);

    cout<<"render json document"<<endl;
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    camera_doc_.Accept(writer);
    //if (!camera_doc_.Null()) {
    if (camera_doc_.MemberCount()) {
        detected_ = true;
    }
    puts(sb.GetString());

    {
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
    } 

    // valid schema
    {
        rapidjson::Document document;
        document.Parse(g_schema);

        rapidjson::SchemaDocument schemaDocument(document);
        rapidjson::SchemaValidator validator(schemaDocument);

        /* parse JSON string */
        rapidjson::Document modelDoc;
        modelDoc.Parse(sb.GetString());

        if (!modelDoc.Accept(validator)) {
            cout<<"schema invalidated"<<endl;
        } else {
            cout<<"schema validated"<<endl;
        }
    }

    // invalid schema
    {
        rapidjson::Document document;
        document.Parse(g_schema_invalid);

        rapidjson::SchemaDocument schemaDocument(document);
        rapidjson::SchemaValidator validator(schemaDocument);

        rapidjson::Document modelDoc;
        modelDoc.Parse(sb.GetString());

        if (!modelDoc.Accept(validator)) {
            cout<<"schema invalidated"<<endl;
        } else {
            cout<<"schema validated"<<endl;
        }
    }

    // {
    //     /* Compile a Document to SchemaDocument */
    //     SchemaDocument schema(document);

    //     rapidjson::Document sd;
        
    //     schema.GetRoot
    //     sd.Parse(schemaJson.c_str());
    // }

    return detected_;
}

// char* WebCam::GetSupportedVideoModes() {
//     ENTER_(WEBCAM_TAG);

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

// }


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

int Camera::Start() 
{
    synclog_->LogV(__FUNCTION__, " entry");

    std::cout << "FUTURE - ASYNC EXAMPLE" << std::endl;

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
        SyncLog::GetLog()->Log("got instance: "+std::to_string(val->instance_));
        val = nullptr;
    }

    // ENTER_(WEBCAM_TAG);

    // int result = EXIT_FAILURE;
    // if (camera_device_handle_) {
    //     return webcam_fifo_->StartFrameAcquisition();
    // }
    // RETURN_(WEBCAM_TAG, result, int);
    return 0;
}

int Camera::Stop() {
    synclog_->LogV(__FUNCTION__, " entry");

    // ENTER_(WEBCAM_TAG);

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

    // RETURN_(WEBCAM_TAG, 0, int);
    return 0;
}
