#ifndef __IMAGECACHE_H__
#define __IMAGECACHE_H__

#include <map>
#include <thread>
#include <QDir>
#include <QStringList>
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
    static void cacheImage(ImageCache* self, int idx);

private:
    static const int nEntries = 10;

    QDir dir;
    QStringList files;
    std::map<int,hpgv::ImageRAF> cache;
    std::map<int,std::thread> threads;
};

#endif //__IMAGECACHE_H__
