#ifndef __IMAGECACHE_H__
#define __IMAGECACHE_H__

#include <deque>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <QDir>
#include <QStringList>
#include <QTimer>
#include "imageraf.h"

class ImageCache
{
public:
    ImageCache();
    virtual ~ImageCache();

    int open(const QString& qfilename);
    int getImageCount() const { return files.size(); }
    const hpgv::ImageRAF* getImage(int idx);

protected:
    static void handleTask(ImageCache* self);
    void handleCurrTask();
    void performCache();

private:
    static const int nEntries = 10;

    struct Task
    {
        enum Code { Exit, Delete, Load, Empty, Sleep };
        Task() : code(Empty), idx(-1) {}
        Task(Code c, int i = -1) : code(c), idx(i) {}
        Code code;
        int idx;
    };

    QDir dir;
    QStringList files;
    std::map<int,hpgv::ImageRAF> cache;
    std::deque<Task> tasks;
    std::thread taskThread;
    std::mutex mutexTasks;
    std::mutex mutexCache;
    Task currTask;
    int currIdx;
    QTimer timerCache;
};

#endif //__IMAGECACHE_H__
