#include "imagecache.h"
#include <string>
#include <thread>
#include <QFileDialog>
#include <QDir>

ImageCache::ImageCache()
{
}

ImageCache::~ImageCache()
{
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
    files.clear();
    cache.clear();
    // setup cache
    files = temp;
    int iFile = files.indexOf(qfi.fileName());
    // return current index
    return iFile;
}

const hpgv::ImageRAF* ImageCache::getImage(int idx)
{
    // if cache miss
    if (0 == cache.count(idx))
    {
        std::string filename = dir.absoluteFilePath(files[idx]).toUtf8().constData();
        cache[idx].open(filename);
    }
    // clean far away cached images and threads
    int iBeg = idx - nEntries / 2;
    int iEnd = idx + nEntries / 2;
    for (auto itr = cache.begin(); itr != cache.end();)
    {
        if (itr->first < iBeg || itr->first > iEnd)
            itr = cache.erase(itr);
        else
            ++itr;
    }
    for (auto itr = threads.begin(); itr != threads.end();)
    {
        if (itr->first < iBeg || itr->first > iEnd)
            itr = threads.erase(itr);
        else
            ++itr;
    }
    // setup threads to cache nearby images
    for (int i = iBeg; i <= iEnd; ++i)
    {
        threads[i] = std::thread(cacheImage, this, i);
        threads[i].detach();
    }
    // return current image
    return &cache[idx];
}

void ImageCache::cacheImage(ImageCache* self, int idx)
{
    if (idx < 0 || idx >= self->files.size())
        return;
    if (0 < self->cache.count(idx))
        return;
    std::string filename = self->dir.absoluteFilePath(self->files[idx]).toUtf8().constData();
    self->cache[idx].open(filename);
    std::cout << "Cached " << idx << std::endl;
    return;
}
