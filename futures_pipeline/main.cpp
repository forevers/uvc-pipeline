/* 
compiler options
  g++ -g -O0 -Wunused -std=c++17 -I../utils -pthread -ggdb -lpthread main.cpp ../util/SyncLog.cpp -o async_future_demo
  clang++ -g -O0 -Wunused-variable -Wunused-parameter -std=c++17 -I$(pwd)/camera/include/linux -I./camera -I./utils -I./utils/queue -I./utils/boost_1_74_0 -I$(pwd)/utils/rapidjson_004e8e6/include -I./utils/rapidjson_004e8e6/include/rapidjson -pthread -lpthread main.cpp ./camera/camera.cpp ./utils/sync-log.cpp ./utils/exec-shell.cpp -o async_future_demo

valgrind options
  valgrind --leak-check=yes ./async_future_demo 
  valgrind -v --leak-check=yes --leak-check=full --show-leak-kinds=all  ./async_future_demo 
*/
// simple demo of thread async-future synchronizationsynchronization

#include "camera.h"
#include "sync-log.h"

// #include <future>
#include <memory>
#include <sched.h>
#include <functional>
#include <string>
// #include <sys/resource.h>
// #include <sys/syscall.h>
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


/* use main temporarily as client context */
int main() 
{
    shared_ptr<Camera> camera = make_shared<Camera> ();

    string camera_modes = camera->GetSupportedVideoModes();

    cout<<"json video modes: "<<camera_modes<<endl;

    // valid schema
    // TODO client side api construct camera_modes object for iteration and GUI render
    {
        rapidjson::Document document;
        document.Parse(g_schema);

        rapidjson::SchemaDocument schemaDocument(document);
        rapidjson::SchemaValidator validator(schemaDocument);

        /* parse JSON string */
        rapidjson::Document camera_modes_doc;
        camera_modes_doc.Parse(camera_modes.c_str());

        if (camera_modes_doc.Accept(validator)) {
            cout<<"schema validated"<<endl;
            cout<<"num cameras: "<<camera_modes_doc["cameras"].Size()<<endl;
            for (auto& camera : camera_modes_doc["cameras"].GetArray()) {
                cout<<camera["device node"].GetString()<<endl;
                cout<<"num formats: "<<camera["formats"].Size()<<endl;
                for (auto& format : camera["formats"].GetArray()) {
                    cout<<format["index"].GetInt()<<endl;   
                    cout<<format["type"].GetString()<<endl;  
                    cout<<format["format"].GetString()<<endl;  
                    cout<<format["name"].GetString()<<endl;  
                    cout<<"num res rates: "<<format["res rates"].Size()<<endl;
                    for (auto& res_rate : format["res rates"].GetArray()) {
                        cout<<res_rate["resolution"].GetString()<<endl; 
                        cout<<"num rates: "<<res_rate["rates"].Size()<<endl;
                        for (auto& rate : res_rate["rates"].GetArray()) {
                            cout<<rate.GetString()<<endl;
                        }
                    }
                }
            }
        } else {
            cout<<"schema invalidated"<<endl;
        }
    }

    camera->Start();
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

        camera->Stop();
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
