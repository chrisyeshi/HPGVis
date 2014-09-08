#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>
#include "parameter.h"

int main(void)
{
    std::ifstream fin("hpgv_render_1408_1080_1100_FLOAT_1024_1024.cfg", std::ios::binary | std::ios::ate);
    assert(fin);
    std::streampos size = fin.tellg();
    std::vector<char> buffer(size);
    fin.seekg(0, std::ios::beg);
    fin.read(&buffer[0], size);
    fin.close();

    hpgv::Parameter para;
    assert(para.deserialize(buffer));
    std::vector<char> toCompare = para.serialize();

    assert(toCompare == buffer);

//    assert(para.getColormap().size == 1024);
//    assert(para.getColormap().format == 6408);
//    assert(para.getColormap().type == 5126);
    assert(para.getFormat() == 6408);
    assert(para.getType() == 5126);
    assert(para.getView().width == 1024);
    assert(para.getView().height == 1024);
    assert(fabs(para.getView().angle - 40.000000) < 0.0001);
    assert(fabs(para.getView().scale - 3.056652) < 0.0001);
    assert(para.getImages().size() == 1);

    return 0;
}
