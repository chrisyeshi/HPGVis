#include "featuretracker.h"
#include <QImage>
#include <ctime>

FeatureTracker::FeatureTracker() : maskValue(0) { }

void FeatureTracker::setDim(const int &w, const int &h) {
    dim = vec2i(w, h);
    nPixels = dim.nData();
}

void FeatureTracker::extract() {
//    clock_t start = std::clock();

#pragma omp parallel for
    for (int y = 0; y < dim.y; ++y) {
#pragma omp parallel for
        for (int x = 0; x < dim.x; ++x) {
            vec2i seed = vec2i(x, y);
            int idx = getIdx(seed);
            if (pMask[idx] > 0 || pData[idx] == 0.0f || pData[idx] == 1.0f) {
                continue;
            }
            findNewFeature(seed);
        }
    }

//    double diff = (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC);
//    std::cout << "diff: " << diff << std::endl;
}

void FeatureTracker::findNewFeature(const vec2i& seed) {
    maskValue++;

    Feature f; {
        f.id        = -1;
        f.maskValue = maskValue;
        f.ctr       = vec2i();
        f.max       = vec2i();
        f.min       = dim;
        f.edgePixels.clear();
        f.bodyPixels.clear();
    }

    f.edgePixels.push_back(seed);

    expandRegion(f);

    if (f.bodyPixels.size() < 100) {
        maskValue--;
        return;
    }

    currentFeatures.push_back(f);
}

void FeatureTracker::track(const float* pData, float* pMask) {
//    delete [] pDataBuf;
//    delete [] pMaskBuf;

//    pDataBuf = new float[nPixels];
//    pMaskBuf = new int[nPixels];

//    std::copy(pData, pData+nPixels, pDataBuf);
//    std::copy(pMask, pMask+nPixels, pMaskBuf);

    this->pData = pData;
    this->pMask = pMask;

//    for (unsigned int i = 0; i < currentFeatures.size(); ++i) {
//        Feature f = currentFeatures[i];

//        std::cout << "bodySize: " << f.bodyPixels.size() << " -> ";
//        fillRegion(f);
//        std::cout << f.bodyPixels.size() << " -> ";
//        shrinkRegion(f);
//        std::cout << f.bodyPixels.size() << " -> ";
//        expandRegion(f);
//        std::cout << f.bodyPixels.size() << std::endl;

//        if (f.bodyPixels.size() < 100) {
//            currentFeatures.erase(currentFeatures.begin() + i);
////            for (const auto& p : f.bodyPixels) {
////                pMask[getIdx(p)] = 0;
////            }
////            continue;
//        } else {
//            currentFeatures[i] = f;
//        }
//    }

    extract();
}

void FeatureTracker::fillRegion(Feature& f) {
    // pixel on edge based on prediction
    for (const auto& pixel : f.edgePixels) {
        int idx = getIdx(pixel);
        if (pMask[idx] == 0) {
            pMask[idx] = (float)f.maskValue;
        }
        f.bodyPixels.push_back(pixel);
        f.ctr += pixel;
    }

    // currently not on edge but previously on edge
    for (const auto& pixel : f.edgePixels) {
        int idx = getIdx(pixel);
        if (pixel.x >= 0 && pixel.x <= dim.x && pixel.y >= 0 && pixel.y <= dim.y &&
            pMask[idx] == 0 && pMaskBuf[idx] == (float)f.maskValue) {
            // mark pixels that: 1. currently = 1; or 2. currently = 0 but previously = 1
            pMask[idx] = (float)f.maskValue;
            f.bodyPixels.push_back(pixel);
            f.ctr += pixel;
        }
    }
}

bool FeatureTracker::expandRegion(Feature& f) {
    std::vector<vec2i> tempPixels;

    while (!f.edgePixels.empty()) {
        vec2i p = f.edgePixels.back();
        f.edgePixels.pop_back();

        bool onEdge = false;
        if (p.x > 0)       { onEdge |= expandEdge(f, p, p-vec2i(1,0)); }
        if (p.y > 0)       { onEdge |= expandEdge(f, p, p-vec2i(0,1)); }
        if (p.x < dim.x-1) { onEdge |= expandEdge(f, p, p+vec2i(1,0)); }
        if (p.y < dim.y-1) { onEdge |= expandEdge(f, p, p+vec2i(0,1)); }
        if (onEdge)        { tempPixels.push_back(p); }
    }

    f.edgePixels.swap(tempPixels);

    if (f.bodyPixels.size() != 0) {
        f.ctr /= f.bodyPixels.size();
        f.id = getIdx(f.ctr);
    }

    if (f.bodyPixels.size() < 100) {
        for (const auto& p : f.bodyPixels) {
            int idx = getIdx(p);
            pMask[idx] = 0;
        }
    }
}

bool FeatureTracker::expandEdge(Feature& f, const vec2i& p, const vec2i& nb) {
    // return nb on edge or not

    int idx = getIdx(p);
    int idx_nb = getIdx(nb);

    if (pMask[idx_nb] > 0) {
        return false;
    }

    if (pData[idx_nb] == 0.0 || pData[idx_nb] == 1.0 ||
        std::abs(pData[idx_nb]-pData[idx]) > 0.001) {
        return true;
    }

    // update feature info
    pMask[idx_nb] = (float)f.maskValue;
    f.min = util::vec2<int>::min(f.min, nb);
    f.max = util::vec2<int>::max(f.max, nb);
    f.edgePixels.push_back(nb);
    f.bodyPixels.push_back(nb);
    f.ctr += nb;

    return false;
}

void FeatureTracker::shrinkRegion(Feature &f) {
    // mark all edge points as 0
    while (!f.edgePixels.empty()) {
        vec2i p = f.edgePixels.back();
        f.edgePixels.pop_back();
        shrinkEdge(f, p);
    }

    while (!f.bodyPixels.empty()) {
        vec2i p = f.bodyPixels.back();
        f.bodyPixels.pop_back();

        int idx = getIdx(p);
        bool onEdge = false;
        if (pData[idx] == 0.0 || pData[idx] == 1.0
            || std::abs(pData[idx] - pDataBuf[idx]) > 0.001) {
            onEdge = false;
            shrinkEdge(f, p);
            if (p.x > 0)       { shrinkEdge(f, p-vec2i(1,0)); }
            if (p.y > 0)       { shrinkEdge(f, p-vec2i(0,1)); }
            if (p.x < dim.x-1) { shrinkEdge(f, p+vec2i(1,0)); }
            if (p.y < dim.x-1) { shrinkEdge(f, p+vec2i(0,1)); }

//            if (++p.x < dim.x) { shrinkEdge(f, p); } p.x--;
//            if (++p.y < dim.y) { shrinkEdge(f, p); } p.y--;
//            if (--p.x >= 0)    { shrinkEdge(f, p); } p.x++;
//            if (--p.y >= 0)    { shrinkEdge(f, p); } p.y++;
        } else if (pMask[idx] == 0) { onEdge = true; }

        if (onEdge) {
            f.edgePixels.push_back(p);
        }

        for (const auto& pixel : f.edgePixels) {
            int idx = getIdx(pixel);
            if (pMask[idx] != (float)f.maskValue) {
                pMask[idx] = (float)f.maskValue;
                f.bodyPixels.push_back(pixel);
                f.ctr += pixel;
            }
        }
    }
}

void FeatureTracker::shrinkEdge(Feature& f, const vec2i& p) {
    int idx = getIdx(p);
    if (pMask[idx] == (float)f.maskValue) {
        pMask[idx] = 0;
        auto it = std::find(f.bodyPixels.begin(), f.bodyPixels.end(), p);
        if (it != f.bodyPixels.end()) {
            f.bodyPixels.erase(it);
            f.edgePixels.push_back(p);
            f.ctr -= p;
        }
    }
}
