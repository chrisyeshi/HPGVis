//
//
// This is a small object to encapsulates a volume pointer.
// WARNING: It doesn't take care of the memory deallocation.
//
//

#ifndef __hpgv_volume__
#define __hpgv_volume__

#include <vector>

namespace hpgv
{

struct Volume
{
	Volume() : data(NULL) {}
	Volume(double* data,
			const std::vector<double>& xCoords, 
			const std::vector<double>& yCoords,
			const std::vector<double>& zCoords)
	  : data(data), xCoords(xCoords), yCoords(yCoords), zCoords(zCoords)
	{}

	double* data;
	std::vector<double> xCoords, yCoords, zCoords;
};

} // namespace hpgv

#endif // __hpgv_volume__