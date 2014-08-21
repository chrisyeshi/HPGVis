#include "image.h"
#include <fstream>
#include <cassert>

namespace hpgv
{

bool Image::open(const std::string& filename)
{
	std::ifstream fin(filename.c_str(), std::ios::binary);
	if (!fin) return false;
    return this->read(fin);
}

//void Image::save(const std::string& filename) const
//{
//	std::ofstream fout(filename.c_str(), std::ios::binary);
//	assert(fout);
//	this->write(fout);
//}

} // namespace hpgv
