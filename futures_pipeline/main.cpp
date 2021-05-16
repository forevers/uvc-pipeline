/* 
compiler options
  g++ -g -O0 -Wunused -std=c++17 -I../utils -pthread -ggdb -lpthread main.cpp ../util/SyncLog.cpp -o async_future_demo
  clang++ -g -O0 -Wunused-variable -Wunused-parameter -std=c++17 -I$(pwd)/camera/include/linux -I./camera -I./utils -I./utils/queue -I./utils/boost_1_74_0 -I$(pwd)/utils/rapidjson_004e8e6/include -I./utils/rapidjson_004e8e6/include/rapidjson -pthread -lpthread main.cpp ./camera/camera.cpp ./utils/sync-log.cpp ./utils/exec-shell.cpp -o async_future_demo

  cmake -D CMAKE_C_COMPILER=clang-6.0 -D CMAKE_CXX_COMPILER=clang++-6.0 ..

utilities
  v4l2-ctl --device /dev/video2 -D --list-formats
  v4l2-ctl --device /dev/video2 -D --list-formats-ext

valgrind options
  valgrind --leak-check=yes ./async_future_demo 
  valgrind -v --leak-check=yes --leak-check=full --show-leak-kinds=all  ./async_future_demo 
*/
// simple demo of thread async-future synchronization

#include "camera.h"
#include "sync-log.h"

// #include <future>
#include <memory>
#include <sched.h>
#include <functional>
#include <signal.h>
#include <string>
// #include <sys/resource.h>
// #include <sys/syscall.h>
#include <termios.h>
// #include <unistd.h>
#include <utility>
// #include <vector>

#include "schema.h"
#include "rapidjson/schema.h"
#include "rapidjson/document.h"

extern "C"
{
    #include <linux/videodev2.h>
}
#define V4L_BUFFERS_DEFAULT 8
#include <unistd.h>



using namespace rapidjson;
using namespace std;

// TODO start pipeline elements
shared_ptr<Camera> camera_ = nullptr;
enum class PipelineState 
{
    Closed,
    Open,
    Running
};
PipelineState pipeline_state_{PipelineState::Closed};
// TODO end pipeline elements

// TODO start client camara caps
rapidjson::Document camera_modes_doc_;
static bool camera_types_validated_{false};
static size_t camera_dev_nodes_size_{0};
static string camera_dev_node_{"none"};
static size_t camera_dev_node_index_{0};

static size_t camera_formats_size_{0};
static string camera_format_{"none"};
static size_t camera_format_index_{0};

static size_t camera_format_resolutions_size_{0};
static string camera_format_resolution_{"none"};
static size_t camera_format_resolution_index_{0};

static size_t camera_format_resolution_rates_size_{0};
static string camera_format_resolution_rate_{"none"};
static size_t camera_format_resolution_rate_index_{0};

// static string camera_rate_{"none"};
// TODO end client camara caps


static void command_line_usage()
{
    cout<<endl<<"*******************************************************"<<endl;
    cout<<"n: number of frames to stream"<<endl;
    cout<<"q: terminate stream"<<endl;
    cout<<endl<<"*******************************************************"<<endl;
}

// static void runtime_usage()
// {
//     switch (pipeline_state_) {
//         case PipelineState::Closed:
//             if (camera_dev_node_ == "none") {
//                 auto cameras = camera_modes_doc_["cameras"].GetArray();
//                 for (size_t node=0; node < camera_dev_nodes_size_; node++) {
//                      auto cameras = camera_modes_doc_["cameras"].GetArray();
//                     // for (auto& camera : camera_modes_doc_["cameras"].GetArray()) {
//                         // cout<<node<<": "<<camera["device node"].GetString()<<endl;
//                         cout<<node<<": "<<cameras[node]["device node"].GetString()<<endl;
//                     // }
//                 }
//             }
//             // cout<<"camera: "<<camera_dev_node_<<endl;
//             // cout<<"format: "<<camera_format_<<endl;
//             // cout<<"resolution: "<<camera_resolution_<<endl;
//             // cout<<"frame rate: "<<camera_rate_<<endl;
//             // cout<<endl<<"*******************************************************"<<endl;
//             // cout<<"c - select video device node"<<endl;
//             // cout<<"f - select video format"<<endl;
//             // cout<<endl<<"*******************************************************"<<endl;

//             break;

//         case PipelineState::Open:
//         case PipelineState::Running:
//             break;
//     }
// }

// class Input
// {
// public:
//     Input(uint8_t niceness) :
//         niceness_(niceness)
//     {
//         synclog_ = SyncLog::GetLog();
//         synclog_->Log("Input()"); 
//         instance_ = ++instances_;
//     }
//     ~Input() {
//         synclog_->Log(to_string(instance_)+": ~Input()"); 
//     }
//     int64_t instance_;
//     static int64_t instances_;

//     uint8_t niceness_;

//     int32_t load_;
//     vector<uint8_t> data_[1024];

// private:
//     SyncLog* synclog_;
// };
// int64_t Input::instances_{0};


// class Output
// {
// public:
//     Output(int64_t instance) {
//         synclog_ = SyncLog::GetLog();
//         synclog_->Log("Output()"); 
//         instance_ = instance;
//     }
//     ~Output() {
//         synclog_->Log(to_string(instance_)+": ~Output()"); 
//     }

//     int64_t instance_;
//     vector<int8_t> data_[2048];

// private:
//     SyncLog* synclog_;
// };


// shared_ptr<Output> async_function(shared_ptr<Input> input)
// {
//     sched_param sch;
//     int policy; 
//     pthread_getschedparam(pthread_self(), &policy, &sch);
//     if (SCHED_OTHER == policy || SCHED_BATCH == policy || SCHED_IDLE == policy) {

//         pid_t tid;
//         tid = syscall(SYS_gettid);
//         int niceness = getpriority(PRIO_PROCESS, tid);

//         /* a non-root users can only make threads nicer 0->19 
//            root can make threads greedy -20->19
//            root can use FIFO scheduling class
//         */
        
//         if (-1 == setpriority(PRIO_PROCESS, tid, input->niceness_)) {
//             SyncLog::GetLog()->Log("setpriority() failure");
//         }

//         int niceness_new = getpriority(PRIO_PROCESS, tid);

//         SyncLog::GetLog()->Log("thread id = " + to_string(tid) + ", cores id = " + to_string(sched_getcpu())
//             + ", priority = " + to_string(niceness) + ":" + to_string(niceness_new)
//             + ", policy = " + string(((policy == SCHED_OTHER)  ? "SCHED_OTHER" :
//                 (policy == SCHED_BATCH) ? "SCHED_RR" :
//                 "SCHED_IDLE")));
//     }

//     int run_count{0};
//     bool running{true};
//     while (running) {
//         //SyncLog::GetLog()->Log(to_string(input->instance_)+": async_function pass : "+to_string(run_count));
//         this_thread::sleep_for(chrono::milliseconds(input->load_));
//         if (input->load_ == run_count++) running = false;
//     }

//     shared_ptr<Output> output = make_shared<Output>(input->instance_);

//     return output;
// }


void other_routine(int seconds_delay)
{
    static int count{0};
    SyncLog::GetLog()->Log("other_routine() call : " + std::to_string(count++));
    std::this_thread::sleep_for(std::chrono::seconds(seconds_delay));
}


/* start the pipeline */
void Start()
{

}


void handler(int signo)
{
    switch(signo) {
        case SIGINT:
        /* enable buffering */
        termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag |= ICANON;
        t.c_lflag |= ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        exit(1);
        break;
    }
}


static void process_key_entry()
{
    int ch;
    termios t;

    // runtime_usage();

    /* disable buffering */
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~ICANON;
    t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    while ('q' != (ch = getchar())) {
       
        if ('h' == ch) {
            // runtime_usage();
        } else {
            switch (pipeline_state_)  {
                case PipelineState::Closed: {
                    camera_dev_node_index_ = 0;
                    cout<<endl<<endl;
                    while (camera_dev_node_ == "none")  {
                        auto cameras = camera_modes_doc_["cameras"].GetArray();

                        cout<<"select camera: ["<<camera_dev_node_index_<<": "<<cameras[camera_dev_node_index_]["device node"].GetString()<<"]"<<endl;
                        for (size_t node = 0; node < camera_dev_nodes_size_; node++) {
                            cout<<node<<": "<<cameras[node]["device node"].GetString()<<endl;
                        }
                        ch = getchar();
                        switch (ch) {
                            case 10:
                            case 13: { // enter
                                camera_dev_node_ = cameras[camera_dev_node_index_]["device node"].GetString();
                                cout<<"selected camera: ["<<camera_dev_node_<<"]"<<endl;
                                break;
                            }
                            // 27, 91, 65 - up arrow
                            // 27, 91, 66 - down arrow
                            case 27: {
                                if (91 == getchar()) {
                                    int key_val = getchar();
                                    if (65 == key_val) {
                                        if (camera_dev_node_index_ == 0) {
                                            camera_dev_node_index_ = camera_dev_nodes_size_ - 1;
                                        } else {
                                            camera_dev_node_index_--;
                                        }
                                    } else if (66 == key_val) {
                                        if (camera_dev_node_index_ == (camera_dev_nodes_size_-1)) {
                                            camera_dev_node_index_ = 0;
                                        } else {
                                            camera_dev_node_index_++;
                                        }
                                    } 
                                }
                                break;
                            }
                        }
                    } 
                    cout<<endl<<endl;
                    while (camera_format_ == "none") {
                        cout<<"num formats: "<<camera_formats_size_<<endl;

                        auto formats = camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"].GetArray();
                        cout<<"select format: ["<<camera_format_index_<<": "<<formats[camera_format_index_]["index"].GetInt()<<"]"<<endl;
                        cout<<string(4, ' ')<<formats[camera_format_index_]["type"].GetString()<<endl;
                        cout<<string(4, ' ')<<formats[camera_format_index_]["format"].GetString()<<endl;
                        cout<<string(4, ' ')<<formats[camera_format_index_]["name"].GetString()<<endl; 
                        cout<<string(4, ' ')<<"num res rates: "<<formats[camera_format_index_]["res rates"].Size()<<endl;

                        for (size_t format = 0; format < camera_formats_size_; format++) {
                            cout<<format<<": "<<formats[camera_format_index_]["format"].GetString()<<endl;
                        }
                        ch = getchar();
                        switch (ch) {
                            case 10:
                            case 13: { // enter
                                camera_format_ = formats[camera_format_index_]["format"].GetString();
                                camera_format_resolutions_size_ = formats[camera_format_index_]["res rates"].Size();
                                cout<<"selected camera: ["<<camera_dev_node_<<"]"<<endl;
                                cout<<"selected format: ["<<camera_format_<<"]"<<endl;
                                break;
                            }
                            // 27, 91, 65 - up arrow
                            // 27, 91, 66 - down arrow
                            case 27: {
                                if (91 == getchar()) {
                                    int key_val = getchar();
                                    if (65 == key_val) {
                                        if (camera_format_index_ == 0) {
                                            camera_format_index_ = camera_formats_size_ - 1;
                                        } else {
                                            camera_format_index_--;
                                        }
                                    } else if (66 == key_val) {
                                        if (camera_format_index_ == (camera_formats_size_-1)) {
                                            camera_format_index_ = 0;
                                        } else {
                                            camera_format_index_++;
                                        }
                                    } 
                                }
                                break;
                            }
                        }
                    }
                    cout<<endl<<endl;
                    while (camera_format_resolution_ == "none") {
                        cout<<"num resolutions: "<<camera_format_resolutions_size_<<endl;

                        auto resolution_rates = camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"][camera_format_index_]["res rates"].GetArray();
                        cout<<"select resolution: ["<<camera_format_resolution_index_<<": "<<resolution_rates[camera_format_resolution_index_]["resolution"].GetString()<<"]"<<endl;

                        for (size_t format_resolution = 0; format_resolution < camera_format_resolutions_size_; format_resolution++) {
                            cout<<format_resolution<<": "<<resolution_rates[format_resolution]["resolution"].GetString()<<endl;
                        }
                        ch = getchar();
                        switch (ch) {
                            case 10:
                            case 13: { // enter
                                camera_format_resolution_ = resolution_rates[camera_format_resolution_index_]["resolution"].GetString();
                                cout<<"selected camera: ["<<camera_dev_node_<<"]"<<endl;
                                cout<<"selected format: ["<<camera_format_<<"]"<<endl;
                                cout<<"selected resolution: ["<<camera_format_resolution_<<"]"<<endl;
                                camera_format_resolution_rates_size_ = resolution_rates[camera_format_resolution_index_]["rates"].Size();

                                break;
                            }
                            // 27, 91, 65 - up arrow
                            // 27, 91, 66 - down arrow
                            case 27: {
                                if (91 == getchar()) {
                                    int key_val = getchar();
                                    if (65 == key_val) {
                                        if (camera_format_resolution_index_ == 0) {
                                            camera_format_resolution_index_ = camera_format_resolutions_size_ - 1;
                                        } else {
                                            camera_format_resolution_index_--;
                                        }
                                    } else if (66 == key_val) {
                                        if (camera_format_resolution_index_ == (camera_format_resolutions_size_-1)) {
                                            camera_format_resolution_index_ = 0;
                                        } else {
                                            camera_format_resolution_index_++;
                                        }
                                    } 
                                }
                                break;
                            }
                        }                       
                    }
                    cout<<endl<<endl;
                    while (camera_format_resolution_rate_ == "none") {
                        cout<<"num rate: "<<camera_format_resolution_rates_size_<<endl;

                        auto rates = camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"][camera_format_index_]["res rates"][camera_format_resolution_index_]["rates"].GetArray();
                        cout<<"select rate: ["<<camera_format_resolution_rate_index_<<": "<<rates[camera_format_resolution_rate_index_].GetString()<<"]"<<endl;

                        for (size_t format_resolution_rate = 0; format_resolution_rate < camera_format_resolution_rates_size_; format_resolution_rate++) {
                            cout<<format_resolution_rate<<": "<<rates[format_resolution_rate].GetString()<<endl;
                        }
                        ch = getchar();
                        switch (ch) {
                            case 10:
                            case 13: { // enter
                                camera_format_resolution_rate_ = rates[camera_format_resolution_rate_index_].GetString();
                                cout<<"selected camera: ["<<camera_dev_node_<<"]"<<endl;
                                cout<<"selected format: ["<<camera_format_<<"]"<<endl;
                                cout<<"selected resolution: ["<<camera_format_resolution_<<"]"<<endl;
                                cout<<"selected resolution: ["<<camera_format_resolution_rate_<<"]"<<endl;
                                break;
                            }
                            // 27, 91, 65 - up arrow
                            // 27, 91, 66 - down arrow
                            case 27: {
                                if (91 == getchar()) {
                                    int key_val = getchar();
                                    if (65 == key_val) {
                                        if (camera_format_resolution_rate_index_ == 0) {
                                            camera_format_resolution_rate_index_ = camera_format_resolution_rates_size_ - 1;
                                        } else {
                                            camera_format_resolution_rate_index_--;
                                        }
                                    } else if (66 == key_val) {
                                        if (camera_format_resolution_rate_index_ == (camera_format_resolution_rates_size_-1)) {
                                            camera_format_resolution_rate_index_ = 0;
                                        } else {
                                            camera_format_resolution_rate_index_++;
                                        }
                                    } 
                                }
                                break;
                            }
                        }                       
                    }
                    break;
                }
                case PipelineState::Open:  {
                    break;
                }
                case PipelineState::Running:  {
                    break;
                }
            }
        }
    }
}


/* use main as a command line client context */
int main(int argc, char *argv[]) 
{
    int opt;
    // char *n = nullptr;

    /* command line options */
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
            case 'n':
                // n = optarg;
                cout<<stoi(optarg)<<endl;
            default:
                command_line_usage();
        }
    }

    // shared_ptr<Camera> camera = make_shared<Camera> ();
    camera_ = make_shared<Camera>();

    string camera_modes = camera_->GetSupportedVideoModes();
    // cout<<"json video modes: "<<camera_modes<<endl;

    /* validate schema */
    // TODO client side api construct camera_modes object for iteration and GUI render
    {
        rapidjson::Document document;
        document.Parse(g_schema);

        rapidjson::SchemaDocument schemaDocument(document);
        rapidjson::SchemaValidator validator(schemaDocument);

        /* parse JSON string */
        camera_modes_doc_.Parse(camera_modes.c_str());

        if (camera_modes_doc_.Accept(validator)) {
            camera_types_validated_ = true;
            cout<<"schema validated"<<endl;
            size_t align = 0;
            camera_dev_nodes_size_ = camera_modes_doc_["cameras"].Size();
            cout<<string(align, ' ')<<"num cameras: "<<camera_dev_nodes_size_<<endl;
            for (auto& camera : camera_modes_doc_["cameras"].GetArray()) {
                align = 2;
                cout<<string(align, ' ')<<camera["device node"].GetString()<<endl;
                camera_formats_size_ = camera["formats"].Size();
                cout<<string(align, ' ')<<"num formats: "<<camera_formats_size_<<endl;
                for (auto& format : camera["formats"].GetArray()) {
                    align += 4;
                    cout<<string(align, ' ')<<format["index"].GetInt()<<endl;   
                    cout<<string(align, ' ')<<format["type"].GetString()<<endl;  
                    cout<<string(align, ' ')<<format["format"].GetString()<<endl;  
                    cout<<string(align, ' ')<<format["name"].GetString()<<endl;  
                    cout<<string(align, ' ')<<"num res rates: "<<format["res rates"].Size()<<endl;
                    for (auto& res_rate : format["res rates"].GetArray()) {
                        align = 6;
                        cout<<string(align, ' ')<<res_rate["resolution"].GetString()<<endl; 
                        cout<<string(align, ' ')<<"num rates: "<<res_rate["rates"].Size()<<endl;
                        for (auto& rate : res_rate["rates"].GetArray()) {
                            align = 8;
                            cout<<string(align, ' ')<<rate.GetString()<<endl;
                        }
                    }
                }
            }
        } else {
            cout<<"schema invalidated"<<endl;
        }
    }

    struct sigaction sig_action;
    sigset_t set;
    sigemptyset(&set);
    sig_action.sa_flags = SA_RESTART;
    sig_action.sa_mask = set;
    sig_action.sa_handler = &handler;
    /* ctrl-C handler */
    sigaddset(&set, SIGINT);
    sigaction(SIGINT, &sig_action, NULL);

    /* user interface */
    process_key_entry();

    int width;
    int height;
    int start = 0;
    string delim = "x";
    int end = camera_format_resolution_.find(delim);
    width = stoi(camera_format_resolution_.substr(start, end - start));
    start = end + delim.size();
    end = camera_format_resolution_.find(delim, start);
    height = stoi(camera_format_resolution_.substr(start, end - start));

    CameraConfig camera_config(camera_dev_node_, "none", 2, width, width, height, width*480*3, -1, {1, 30});

    camera_->Start(camera_config);
    {        
    //     // TODO scoped code to move to Start()
    //     string device_node_string{"/dev/video0"};
    //     int enumerated_width{640};
    //     int enumerated_height{480};
    //     if (!camera->UvcV4l2Init(device_node_string, enumerated_width, enumerated_height)) {
    //         cout<<"UvcV4l2Init() success"<<endl;

    //         /* test v4l2 frame pull */
    //         // temp test 5 seconds of 30 fps
    //         long num_frames_to_capture = 30;//5*30;
    //         // while (!camera->IsRunning() && num_frames_to_capture) {
    //         while (!camera->IsRunning()) {

    //             // if (uvc_mode == UVC_MODE_LIBUSB) {

    //             //     //std::this_thread::sleep_for(std::chrono::milliseconds(250));
    //             //     // 60fps simulation
    //             //     std::this_thread::sleep_for(std::chrono::microseconds(16667));
    //             //     {
    //             //         std::lock_guard<std::mutex> lock(m_Mutex);

    //             //         if (m_shall_stop)
    //             //         {
    //             //             m_message += "Stopped";
    //             //             break;
    //             //         }
    //             //     }
    //             // } else {
    //                 // {
    //                     //std::lock_guard<std::mutex> lock(m_Mutex);
    //                     // 4l2 will block, issue signal to release mutex until frame arrives
    //             if (0 != camera->UvcV4l2GetFrame()) {
    //                 SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","UvcV4l2GetFrame() failure");
    //                 break;
    //             }
    //             if (num_frames_to_capture > 0) {
    //                 num_frames_to_capture--;
    //             } else {
    //                 SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","num_frames_to_capture == 0");
    //             }
    //             if (0 == num_frames_to_capture) {
    //                 /* Stop streaming. */
    //                 SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","Stop() camera");
    //                 if (camera->Stop()) {
    //                     SyncLog::GetLog()->LogV("[",__func__,": ",__LINE__,"]: ","Stop() failure");
    //                 }
    //             }
    //         // }

    //             // if (m_shall_stop)
    //             // {
    //             //     m_message += "Stopped";
    //             //     break;
    //             // }

    //             // TODO signal the  pipeline flush ... blocking for return to indicate all pipe elements are flushed and in stop state
    //             // frame_queue_ifc->Signal();
    //             // }
    //         }

    //     } else {
    //         cout<<"UvcV4l2Init() failure"<<endl;
    //     }

        sleep(3);

        // camera->Stop();
        camera_->Stop();

        // {
        //     camera->UvcV4l2Exit();
        // }

#if 0 // this works
        // obtain camera device node file descriptor
        int fd = open("/dev/video0", O_RDWR);
        if (fd < 0) {
            cout<<"Error opening device "<<"/dev/video0 "<<strerror(errno)<<":"<<errno<<endl;
            return -1;
        } else {
            cout<<"open() success"<<endl;

            Device device;
            device.fd = fd;
            unsigned int capabilities;
            if (0 == camera->QueryCapabilities(&device, &capabilities)) {

            }

            if (0 != close(fd)) {
                cout<<"Error closing device "<<"/dev/video0 "<<strerror(errno)<<":"<<errno<<endl;
                return -1;   
            } else {
                cout<<"close() success"<<endl;
            }
        }
#endif

    }
    // void RenderUI::StartStream()
    // // Start a new worker thread.
    // worker_thread_ = new thread(
    //   [this]
    //   {
    //     worker_.do_work(this, uvc_mode_, device_node_string_, vid_, pid_, enumerated_width_, enumerated_height_, actual_width_, actual_height_);
    //   });
            // if (0 != uvc_v4l2_init(frame_queue_ifc, device_node_string, enumerated_width, enumerated_height, actual_width, actual_height)) {
        //     LOG("%d > uvc_v4l2_init() failure\n", __LINE__);
        //     return;
        // }
    //void ExampleWorker::do_work(IFrameQueue* frame_queue_ifc, UvcMode uvc_mode, std::string device_node_string, int vid, int pid, int enumerated_width, int enumerated_height, int actual_width, int actual_height)
    //int uvc_v4l2_init(IFrameQueue* frame_queue_ifc, std::string device_node, int enumerated_width, int enumerated_height, int actual_width, int actual_height)
    //v4l2_acquisition video_open

    // std::cout << "FUTURE - ASYNC EXAMPLE" << std::endl;

    // // create promise ... can be issued anywhere in a context ... not just return value
    // vector<future<shared_ptr<Output>>> future_vec;

    // // future from async
    // for (int64_t instance = 0; instance < 16; instance++) {
    //     auto input = make_shared<Input>(instance % 0x10);
    //     input->load_ = instance;
    //     future<shared_ptr<Output>> future_0 = std::async(async_function, input);
    //     future_vec.push_back(move(future_0));
    // }

    // // run some other routines ...
    // //other_routine(1);

    // for (auto& elem : future_vec) {
    //     auto val = elem.get();
    //     SyncLog::GetLog()->Log("got instance: "+std::to_string(val->instance_));
    //     val = nullptr;
    // }

    // camera->Stop();
    // {
    //     camera->UvcV4l2Exit();
    // }
    return 0;
}
