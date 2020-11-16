#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include <string>


class SyncLog
{
public:

    static std::shared_ptr<SyncLog> GetLog();

    void Log(std::string msg);

    template<class... Args>
    void LogV(Args... args)
    {
        (std::cout << ... << args) << "\n" << std::flush;
    };

private:

    std::mutex log_mtx_;
    static std::shared_ptr<SyncLog> log_;

// TODO make private
public:
    SyncLog();
};
