#pragma once

#include <iostream>
#include <mutex>
#include <string>


class SyncLog
{
public:

    static SyncLog* GetLog();

    void Log(std::string msg);

    template<class... Args>
    void LogV(Args... args)
    {
        (std::cout << ... << args) << "\n" << std::flush;
    };

private:

    std::mutex log_mtx_;
    static SyncLog* log_;

    SyncLog();
};
