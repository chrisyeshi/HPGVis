#ifndef __hpgv_ImageRGBA__
#define __hpgv_ImageRGBA__

#include <iostream>
#include "image.h"

namespace hpgv
{

class ImageRGBA : public Image
{
public:
	ImageRGBA();
    ImageRGBA(const ImageRGBA& image);
    ImageRGBA& operator=(const ImageRGBA& image);
	virtual ~ImageRGBA();

	void set(int format, int type, const void* data);

	virtual bool read(std::istream& in);
	virtual void write(std::ostream& out) const;

protected:

private:
	int format;
	int type;
	char* data;

	int nBytesData() const;
};

} // namespace hpgv

#endif // __hpgv_ImageRGBA__