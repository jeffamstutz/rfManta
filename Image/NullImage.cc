
#include <Image/NullImage.h>
#include <Core/Exceptions/IllegalValue.h>

using namespace Manta;

Image* NullImage::create(const vector<string>& args, bool stereo,
			 int xres, int yres)
{
  if(args.size() != 0)
    throw IllegalValue<string>("Unknown argument for NullImage", args[0]);
  return new NullImage(stereo, xres, yres);
}

NullImage::NullImage(bool stereo, int xres, int yres)
  : stereo(stereo), xres(xres), yres(yres)
{
  valid = false;
}

NullImage::~NullImage()
{
}

bool NullImage::isValid() const
{
  return valid;
}

void NullImage::setValid(bool to)
{
  valid = to;
}

void NullImage::getResolution(bool& out_stereo, int& out_xres, int& out_yres) const
{
  out_stereo = stereo;
  out_xres = xres;
  out_yres = yres;
}

void NullImage::set(const Fragment&)
{
  // Do nothing
}

void NullImage::get(Fragment&) const
{
  // Do nothing
}

