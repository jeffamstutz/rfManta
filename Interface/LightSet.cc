
#include <Interface/LightSet.h>
#include <Interface/AmbientLight.h>
#include <Interface/Light.h>
#include <Interface/InterfaceRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/stdRTTI.h>
#include <Core/Util/Assert.h>

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace Manta;
using namespace std;

void LightSet::remove(Light* light)
{
  vector<Light*>::iterator iter = find(_lights.begin(), _lights.end(), light);
  if (iter != _lights.end())
    _lights.erase(iter);
}

LightSet* LightSet::merge(LightSet* l1, LightSet* l2)
{
  LightSet* r = new LightSet();
  // L1 takes precedence for ambient lights
  if(l1->_ambientLight)
    r->_ambientLight = l1->_ambientLight;
  else
    r->_ambientLight = l2->_ambientLight;
  for(size_t i = 0; i < l1->numLights(); i++)
    r->add(l1->getLight(i));
  for(size_t i = 0; i < l2->numLights(); i++)
    r->add(l2->getLight(i));
  return r;
}

void LightSet::preprocess(const PreprocessContext& context)
{
  // This won't work in many of the shadow algorithms
  // Call preprocess on ambient light.
  _ambientLight->preprocess(context);

  // Call preprocess on each light.
  for(size_t i = 0; i < _lights.size(); ++i) {
    _lights[i]->preprocess(context);
  }
}

string LightSet::toString() const {
  ostringstream out;
  out << "ambientLight = "<<_ambientLight<<"\n";
  if (_ambientLight)
    out << _ambientLight->toString();
  out << "Num lights = "<<_lights.size()<<"\n";
  for(size_t i = 0; i < _lights.size(); ++i) {
    out << "lights["<<i<<"] = "<<_lights[i]<<"\n";
  }
  return out.str();
}

void LightSet::readwrite(ArchiveElement* archive)
{
  archive->readwrite("lights", _lights);
  archive->readwrite("ambientLight", _ambientLight);
}
