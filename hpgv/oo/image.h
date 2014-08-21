#ifndef __hpgv_image__
#define __hpgv_image__

#include <string>

namespace hpgv
{

class Image
{
public:
	Image() : width(0), height(0) {}
    virtual ~Image() {}

	void setSize(int w, int h) { width = w; height = h; }
	int getWidth() const { return width; }
	int getHeight() const { return height; }

	bool open(const std::string& filename);
    virtual void save(const std::string& filename) const = 0;

	virtual bool read(std::istream& in) = 0;
//	virtual void write(std::ostream& out) const = 0;

protected:
	int width, height;

private:

};

} // namespace hpgv

#endif // __hpgv_image__
