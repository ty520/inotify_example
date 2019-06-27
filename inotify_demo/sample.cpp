#include <memory>
#include "inotify_app.h"

int main() {
    char* files[] = {
        "./tmp_dir",
        "./tmp_file",
        "./tmp_dirs"
    };
    std::shared_ptr<ty::inotify::InotifyApp> inotify;
    inotify.reset(new ty::inotify::InotifyApp(files, 3));
    inotify->start();
    return 0;
}
