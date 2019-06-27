#include <atomic> 
#include <memory>
#include <vector>
#include <sys/epoll.h>

#pragma once

namespace ty {
namespace inotify {

class InotifyApp {
public:
    struct FilesAttrs {
        int wd;
        char* name;
    };
    InotifyApp(char** monitor_file_dir, int monitor_num);
    ~InotifyApp();

    int start();
    int stop();

private:
    char** _file_dirs;
    int _monitor_num;
    int _fd;
    int _epoll_fd;
    std::atomic<bool> _is_running;
    std::vector<std::shared_ptr<FilesAttrs>> _file_attrs_vec;
};

}
}
