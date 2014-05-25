#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <cmath>
#include <vector>
#include <list>

const size_t MIN_NUM_PIXELS = 50;
const int FT_DIRECT = 0;
const int FT_LINEAR = 1;
const int FT_POLYNO = 2;
const int FT_FORWARD  = 0;
const int FT_BACKWARD = 1;

namespace util {
    template<class T>
    class vec2 {
    public:
        T x, y;
        vec2(T x_ = 0, T y_ = 0) : x(x_), y(y_) {}
        T*    data()                              { return &x; }
        T     nData()                             { return x * y; }
        float magnituteSquared()                  { return x*x + y*y; }
        float magnitute()                         { return sqrt((*this).magnituteSquared()); }
        float distanceFrom(vec2 const& rhs) const { return (*this - rhs).magnitute(); }
        vec2  operator -  ()                      { return vec2(-x, -y); }
        vec2  operator +  (vec2 const& rhs) const { vec2 t(*this); t+=rhs; return t; }
        vec2  operator -  (vec2 const& rhs) const { vec2 t(*this); t-=rhs; return t; }
        vec2  operator *  (vec2 const& rhs) const { vec2 t(*this); t*=rhs; return t; }
        vec2  operator /  (vec2 const& rhs) const { vec2 t(*this); t/=rhs; return t; }
        vec2  operator *  (float scale)     const { vec2 t(*this); t*=scale; return t; }
        vec2  operator /  (float scale)     const { vec2 t(*this); t/=scale; return t; }
        vec2& operator += (vec2 const& rhs)       { x+=rhs.x, y+=rhs.y; return *this; }
        vec2& operator -= (vec2 const& rhs)       { x-=rhs.x, y-=rhs.y; return *this; }
        vec2& operator *= (vec2 const& rhs)       { x*=rhs.x, y*=rhs.y; return *this; }
        vec2& operator /= (vec2 const& rhs)       { x/=rhs.x, y/=rhs.y; return *this; }
        vec2& operator *= (float scale)           { x*=scale, y*=scale; return *this; }
        vec2& operator /= (float scale)           { x/=scale, y/=scale; return *this; }
        bool  operator == (vec2 const& rhs) const { return x==rhs.x && y==rhs.y; }
        bool  operator != (vec2 const& rhs) const { return !(*this == rhs); }

        static inline vec2<T> min(const vec2<T>& v1, const vec2<T>& v2) {
            return vec2<T>(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
        }

        static inline vec2<T> max(const vec2<T>& v1, const vec2<T>& v2) {
            return vec2<T>(std::max(v1.x, v2.x), std::max(v1.y, v2.y));
        }
    };
}

typedef util::vec2<int> vec2i;

struct Feature {
    int   id;        // Unique ID for each feature
    int   maskValue; // color id of the feature
    vec2i ctr;       // Centroid position of the feature
    vec2i min;       // Minimum position (x, y) of the feature
    vec2i max;       // Maximum position (x, y) of the feature
    std::vector<vec2i> edgePixels;
    std::vector<vec2i> bodyPixels;
};

#endif // UTILS_H
