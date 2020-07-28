#include "exec-shell.h"

#include <array>
#include <memory>

#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

std::string exec(const char* cmd) {
    std::array<char, 256> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}