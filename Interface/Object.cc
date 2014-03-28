
#include <Interface/Object.h>
#include <Interface/Context.h>

using namespace Manta;

Object::Object()
{
}

Object::~Object()
{
}

void Object::performUpdate(const UpdateContext& context) {
  // By default, claim we're done already
  context.finish(this);
}
