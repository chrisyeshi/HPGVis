#include "imagecache.h"
#include <sys/time.h>
#include <string>
#include <thread>
#include <chrono>
#include <QFileDialog>
#include <QDir>

ImageCache::ImageCache() : taskThread(handleTask, this), currIdx(-1)
{
}

ImageCache::~ImageCache()
{
    mutexTasks.lock();
    tasks.push_front(Task(Task::Exit));
    mutexTasks.unlock();
    taskThread.join();
}

int ImageCache::open(const QString &qfilename)
{
    // open file
    QFileInfo qfi(qfilename);
    dir = qfi.absoluteDir();
    QStringList temp = dir.entryList(QStringList(QObject::tr("*.raf")));
    if (temp.empty())
        return -1;
    // clean up
    mutexTasks.lock();
    tasks.clear();
    mutexTasks.unlock();
    mutexCache.lock();
    files.clear();
    cache.clear();
    mutexCache.unlock();
    // setup cache
    files = temp;
    int iFile = files.indexOf(qfi.fileName());
    // return current index
    return iFile;
}

const hpgv::ImageRAF* ImageCache::getImage(int idx)
{
    currIdx = idx;
//    timeval start; gettimeofday(&start, NULL);
    // if cache miss
    if (0 == cache.count(idx))
    {
        std::string filename = dir.absoluteFilePath(files[idx]).toUtf8().constData();
        mutexCurrTask.lock();
        assert(currTask.code == Task::Empty);
        mutexCurrTask.unlock();
        mutexCache.lock();
        cache[idx].open(filename);
        mutexCache.unlock();
        mutexTasks.lock();
        tasks.clear();
        mutexTasks.unlock();
    }

    performCache();

//    timeval end; gettimeofday(&end, NULL);
//    double time_getImage = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
//    std::cout << "Time: GetImage:  " << time_getImage << " ms" << std::endl;

    // return current image
    return &cache[currIdx];
}

void ImageCache::handleTask(ImageCache *self)
{
    while (true)
    {
        if (self->tasks.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        self->mutexCurrTask.lock();
        self->currTask = self->tasks.front();
        self->mutexTasks.lock();
        self->tasks.pop_front();
        self->mutexTasks.unlock();
        // exit condition
        if (self->currTask.code == Task::Exit)
        {
            self->currTask = Task(Task::Empty);
            self->mutexCurrTask.unlock();
            break;
        }
        // handle tasks
        self->handleCurrTask();
        self->currTask = Task(Task::Empty);
        self->mutexCurrTask.unlock();
    }
}

void ImageCache::handleCurrTask()
{
//    assert(currTask.idx >= 0 && currTask < files.size());
    if (Task::Sleep == currTask.code)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return;
    }
    if (Task::Delete == currTask.code)
    {
        mutexCache.lock();
        auto itr = cache.find(currTask.idx);
        if (itr != cache.end())
            cache.erase(itr);
        mutexCache.unlock();
//        std::cout << "Thread Deleted " << currTask.idx << std::endl;
        return;
    }
    if (Task::Load == currTask.code)
    {
        if (0 < cache.count(currTask.idx))
            return;
        std::string filename = dir.absoluteFilePath(files[currTask.idx]).toUtf8().constData();
        mutexCache.lock();
        cache[currTask.idx].open(filename);
        mutexCache.unlock();
//        std::cout << "Thread loaded " << currTask.idx << std::endl;
        return;
    }
}

void ImageCache::performCache()
{
    // delay start
    mutexTasks.lock();
    tasks.push_back(Task(Task::Sleep));
    mutexTasks.unlock();
    // delete tasks
    mutexCache.lock();
    for (auto& itr : cache)
    {
        if (itr.first < currIdx - nEntries / 2 || itr.first > currIdx + nEntries / 2)
        {
            mutexTasks.lock();
            tasks.push_back(Task(Task::Delete, itr.first));
            mutexTasks.unlock();
        }
    }
    mutexCache.unlock();
    // forward
    for (int i = currIdx + 1; i <= std::min(currIdx + nEntries / 2, files.size() - 1); ++i)
    {
        mutexTasks.lock();
        tasks.push_back(Task(Task::Load, i));
        mutexTasks.unlock();
    }
    // backward
    for (int i = currIdx - 1; i >= std::max(currIdx - nEntries / 2, 0); --i)
    {
        mutexTasks.lock();
        tasks.push_back(Task(Task::Load, i));
        mutexTasks.unlock();
    }
}
