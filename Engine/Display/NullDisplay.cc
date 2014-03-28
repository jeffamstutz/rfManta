
#include <Engine/Display/NullDisplay.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/NotFinished.h>

using namespace Manta;

ImageDisplay* NullDisplay::create(const vector<string>& args)
{
  return new NullDisplay(args);
}

NullDisplay::NullDisplay(const vector<string>& args)
{
  if(args.size() != 0)
    throw IllegalArgument("NullDisplay", 0, args);
}

NullDisplay::~NullDisplay()
{
}

void NullDisplay::setupDisplayChannel(SetupContext&)
{
}

void NullDisplay::displayImage(const DisplayContext&, const Image*)
{
}
