#ifndef FEATURETRACKER_H
#define FEATURETRACKER_H

#include "utils.h"

class FeatureTracker {

public:
    FeatureTracker();

    void track(const float* pData, float* pMask);
    void setDim(const int& w, const int& h);
    int getNumFeatures() const { return currentFeatures.size(); }

private:
    int   getIdx(const vec2i& p) const { return p.y * dim.x + p.x; }
    vec2i getPos(const int& idx) const { return vec2i(idx%dim.x, idx/dim.x); }

    void findNewFeature(const vec2i& seed);
    void fillRegion(Feature& f);
    bool expandRegion(Feature& f);
    void shrinkRegion(Feature& f);
    bool expandEdge(Feature& f, const vec2i& p, const vec2i& nb);
    void shrinkEdge(Feature& f, const vec2i& p);
    void extract();

    vec2i dim;
    int nPixels;
    int maskValue;
    int tAvailableForward;
    int tAvailableBackward;

    const float* pData;
    float* pDataBuf;
    float* pMask;
    float* pMaskBuf;

    std::vector<Feature> currentFeatures; // Features info in current time step
};

#endif // FEATURETRACKER_H
