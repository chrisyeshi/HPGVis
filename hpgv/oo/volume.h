//
//
// This is a small object to encapsulates a volume pointer.
// Changed from raw pointer to shared_ptr, so it actually cares about memory.
//
//

#ifndef __hpgv_volume__
#define __hpgv_volume__

#include <vector>
#include "boost/shared_ptr.hpp"

namespace hpgv
{

struct Volume
{
    Volume() {}
    Volume(boost::shared_ptr<double[]> data,
			const std::vector<double>& xCoords, 
			const std::vector<double>& yCoords,
			const std::vector<double>& zCoords)
	  : data(data), xCoords(xCoords), yCoords(yCoords), zCoords(zCoords)
	{}

    boost::shared_ptr<double[]> data;
	std::vector<double> xCoords, yCoords, zCoords;
};

} // namespace hpgv

#endif // __hpgv_volume__
