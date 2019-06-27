#include "inotify_app.h"
#include <iostream>
#include <new>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <memory>

#define BUFFER_SIZE 1024
#define MAX_FILE_NAME 256
#define MAX_EVENTS 10

namespace ty {
namespace inotify {

InotifyApp::InotifyApp(char** monitor_file_dir, int monitor_num) :
    _file_dirs(monitor_file_dir),
    _monitor_num(monitor_num),
    _fd(-1),
    _epoll_fd(-1),
    _is_running(false) {
}

InotifyApp::~InotifyApp() {
}

int InotifyApp::start() {
    struct epoll_event ev, events[MAX_EVENTS];

    if (_is_running) {
        printf("Started!!!\n");
        return -1;
    }
    _is_running = true;

    if(_file_dirs == nullptr || _monitor_num <= 0) {
        printf("Input vaild directory, please\n");
        return -1;
    }

    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1) {
        printf("create epoll failed.\n");
        return -1;
    }

    _fd = inotify_init();
    if (_fd < 0) {
        printf("Failed to init inotify.\n");
        exit(1);
    }
    
    ev.events = EPOLLIN;
    ev.data.fd = _fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _fd, &ev) == -1) {
        printf("epoll_ctl: inotify_fd\n");
        return -1;
    }
    // 多个目录添加add_watch
    for (int i = 0; i < _monitor_num; i++) {
        std::shared_ptr<FilesAttrs> file_attr = std::make_shared<FilesAttrs>();
        file_attr->name = _file_dirs[i];
        // 监测文件是否存在，不存在则不需要加监控
        int wd = inotify_add_watch(_fd, _file_dirs[i], IN_DELETE | IN_CLOSE_WRITE | IN_MOVED_TO | IN_MOVED_FROM | IN_IGNORED);
        if (wd < 0) {
            printf("Failed to add watch, file_name:%s\n", _file_dirs[i]);
            continue;
        }
        file_attr->wd = wd;
        _file_attrs_vec.push_back(file_attr);
    }
    
    //依次对各个directory实时监控
    int read_len = 0;
    int event_size = 0;
    int event_pos = 0;
    struct inotify_event* event;
    
    while (_is_running) {
        int nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            printf("epoll_wait fail.\n");
            return -1;
        }

        //判断文件描述符是否符合预期
        int fd_num = 0;
        for (fd_num = 0; fd_num < nfds; fd_num++) {
            if (events[fd_num].data.fd == _fd) {
                int revents = events[fd_num].events;
                if (revents & (EPOLLERR|EPOLLHUP)) {
                    continue;
                }
                if (revents & EPOLLIN) { 
                    char event_buf[BUFFER_SIZE];
                    read_len = read(_fd, event_buf, sizeof(event_buf));
                    int count_size = read_len;
                    while (count_size > (int)sizeof(struct inotify_event)) {
                        if (read_len <= 0 || event_buf == NULL) {
                            printf("error read, len:%d\n", read_len);
                            break;
                        }
                        
                        event = reinterpret_cast<inotify_event*>(event_buf + event_pos);
                        if (event->len >= MAX_FILE_NAME) {
                            printf("invaild event len:%d\n", event->len);
                            break;
                        }
                        if (event->len > 0 && event->name != NULL) {
                            if (event->mask & (IN_MOVED_TO | IN_CLOSE_WRITE)) {
                                printf("File changed, reload: %s\n", event->name);
                            } else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
                                printf("File deleted, clear: %s\n", event->name);
                            } else if (event->mask & IN_IGNORED) {
                                printf("Cancel watching directory");
                            } else {
                                printf("Unexpected event, file: %s, mask:%d\n", event->name, event->mask);
                            }
                        }
                        
                        event_size = sizeof(struct inotify_event) + event->len;
                        count_size -= event_size;
                        event_pos += event_size;
                    }
 
                }
            }
        }
    }
    return 0;
}

int InotifyApp::stop() {
    if (_is_running == false) {
        printf("inotify_app is stopped!");
        return 0;
    }
    _is_running = false;
    for (int i = 0; i < _monitor_num; i++) {
        // 监测文件是否存在，不存在则不需要加监控
        int wd = inotify_rm_watch(_fd, _file_attrs_vec[i]->wd);
        if (wd < 0) {
            printf("Failed to add watch, file_name:%s\n", _file_dirs[i]);
            continue;
        }
    }
    close(_fd);
    close(_epoll_fd);
    return 0;
}

}
}
