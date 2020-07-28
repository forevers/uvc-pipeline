/* 
compiler options
  g++ -g -O0 -Wunused -std=c++17 -I../utils -pthread -ggdb -lpthread main.cpp ../util/SyncLog.cpp -o async_future_demo
  clang++ -g -O0 -Wunused-variable -Wunused-parameter -std=c++17 -I./camera -I./utils -I/home/stevers/projects/education/clones/uvc-pipeline/futures_pipeline/utils/rapidjson_004e8e6/include -I./utils -I./utils/rapidjson_004e8e6/include/rapidjson -pthread -lpthread main.cpp ./camera/camera.cpp ./utils/sync-log.cpp ./utils/exec-shell.cpp -o async_future_demo

valgrind options
  valgrind --leak-check=yes ./async_future_demo 
*/
// simple demo of thread async-future synchronization

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


int main() 
{
    shared_ptr<Camera> camera = make_shared<Camera> ();
    camera->Start();

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

    camera->Stop();

    return 0;
}
