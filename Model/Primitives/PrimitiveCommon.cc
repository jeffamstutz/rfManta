
#include <Model/Primitives/PrimitiveCommon.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Interface/Material.h>
#include <Interface/InterfaceRTTI.h>
#include <Model/TexCoordMappers/UniformMapper.h>
#include <assert.h>

using namespace Manta;

static UniformMapper default_map;

PrimitiveCommon::PrimitiveCommon(Material* material,
                                 const TexCoordMapper* in_tex)
  : material(material), tex(in_tex)
{
  if(!tex)
    tex = &default_map;
}

PrimitiveCommon::~PrimitiveCommon()
{
}

PrimitiveCommon* PrimitiveCommon::clone(CloneDepth depth, Clonable* incoming)
{
  PrimitiveCommon *copy;
  assert (incoming); //PrimitiveCommon is an abstract base class, can't new it.
  copy = dynamic_cast<PrimitiveCommon*>(incoming);
  copy->material = material;
  copy->tex = tex;

  return copy;
}

void PrimitiveCommon::preprocess(const PreprocessContext& context)
{
  material->preprocess(context);
}

void PrimitiveCommon::setTexCoordMapper(const TexCoordMapper* new_tex)
{
  tex = new_tex;
}

Interpolable::InterpErr
PrimitiveCommon::interpolate(const std::vector<Interpolable::keyframe_t> &keyframes)
{
  //don't know how to interpolate between two materials, so lets
  //do nearest neighbor.


  float maxT=-1;
  int maxTindex=-1;
  for (unsigned int frame=0; frame < keyframes.size(); ++frame) {
    if (keyframes[frame].t > maxT) {
      maxT = keyframes[frame].t;
      maxTindex = frame;
    }
  }
  if (maxTindex < 0)
    return notInterpolable;

  PrimitiveCommon *pc = dynamic_cast<PrimitiveCommon*>(keyframes[maxTindex].keyframe);
  if (pc == NULL)
    return notInterpolable;

  material = pc->material;
  tex = pc->tex;

  return success;
}

MANTA_REGISTER_CLASS(PrimitiveCommon);

void PrimitiveCommon::readwrite(ArchiveElement* archive)
{
  MantaRTTI<Primitive>::readwrite(archive, *this);
  archive->readwrite("material", material);
  if(archive->hasField("tex"))
    archive->readwrite("tex", tex);
  else
    tex = &default_map;
}
