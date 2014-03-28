
#include <Engine/ImageTraversers/NullImageTraverser.h>
#include <Interface/Context.h>
#include <Interface/Image.h>
#include <Core/Util/NotFinished.h>

using namespace Manta;

ImageTraverser* NullImageTraverser::create(const vector<string>& args)
{
  return new NullImageTraverser(args);
}

NullImageTraverser::NullImageTraverser(const vector<string>& /*args*/)
{
}

NullImageTraverser::~NullImageTraverser()
{
}

void NullImageTraverser::setupBegin(SetupContext&, int)
{
}

void NullImageTraverser::setupDisplayChannel(SetupContext&)
{
}

void NullImageTraverser::setupFrame(const RenderContext&)
{
}

void NullImageTraverser::renderImage(const RenderContext& context, Image* image)
{
  if(context.proc == 0)
    image->setValid(true);
}
