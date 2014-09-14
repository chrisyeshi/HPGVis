#ifndef __hpgv_Parameter_h__
#define __hpgv_Parameter_h__

#include <vector>
#include "boost/shared_ptr.hpp"
#include "json.h"

namespace hpgv
{

class Parameter
{
public:
    //
    //
    // Static Const Variables
    //
    //
    static const int tfSizeDefault = 1024;
    static const int colorSize = 4;

public:
    //
    //
    // Structs to store data
    //
    //
    struct View
    {
        View() : modelview(16), projection(16), viewport(4),
                width(0), height(0), angle(0.f), scale(0.f) {}
        std::vector<double> modelview;
        std::vector<double> projection;
        std::vector<int> viewport;
        int width;
        int height;
        float angle;
        float scale;
    };
    struct Light
    {
        Light() : enable(false), parameter(4) {}
        bool enable;
        std::vector<float> parameter;
    };
    struct Particle
    {
        Particle() : count(0), radius(0.f), volume(0) {}
        int count;
        float radius;
        int volume;
        boost::shared_ptr<float[tfSizeDefault * 4]> tf;
        Light light;
    };
    struct Volume
    {
        Volume() : id(0) {}
        int id;
        Light light;
    };
    struct Image
    {
        Image() : sampleSpacing(1.f), binTicks(17) {}
        Particle particle;
        float sampleSpacing;
        std::vector<float> tf;
        std::vector<Volume> volumes;
//        float binTicks[HPGV_RAF_BIN_NUM + 1];
        std::vector<float> binTicks;
    };

public:
    //
    //
    // Constructor / Destructor
    //
    //
    Parameter();
    ~Parameter();
    //
    //
    // I/O
    //
    //
    bool                        open(const std::string& filename);
    void                        save(const std::string& filename) const;
    std::vector<char>           serialize() const;
    bool                        deserialize(const char * head);
    bool                        deserialize(const std::vector<char>& buffer);
    Json::Value                 toJSON() const;
    bool                        fromJSON(const Json::Value &root);
    //
    //
    // Standard Accessors
    //
    //
    const View&                 getView() const { return view; }
    const int&                  getFormat() const { return format; }
    const int&                  getType() const { return type; }
    const int&                  getNBins() const { return nBins; }
    const std::vector<Image>&   getImages() const { return images; }
    void                        setView(const View& view) { this->view = view; }
    void                        setFormat(const int& format) { this->format = format; }
    void                        setType(const int& type) { this->type = type; }
    void                        setImages(const std::vector<Image>& images) { this->images = images; }
    bool                        isMinMaxAuto() const { return autoMinmax; }
    double                      getMin() const { return minmax[0]; }
    double                      getMax() const { return minmax[1]; }
    //
    //
    // Reference Accessors
    //
    //
    View&                       rView() { return view; }
    std::vector<Image>&         rImages() { return images; }

private:
    //
    //
    // Member Variables
    //
    //
    View view;
    int format;
    int type;
    int nBins;
    std::vector<Image> images;
    bool autoMinmax;
    double minmax[2];
};

} // namespace hpgv

#endif // __hpgv_Parameter_h__
