
#ifndef Manta_Interface_LightSet_h
#define Manta_Interface_LightSet_h

#include <Interface/RayPacket.h>
#include <vector>
#include <string>

namespace Manta {
  class AmbientLight;
  class Light;
  class PreprocessContext;
  using namespace std;

  class LightSet {
  public:
    LightSet()            : _ambientLight(0) {  }
    LightSet( size_t size ) : _lights( size ), _ambientLight(0) {  }
    ~LightSet() {  }

    // Get and set the ambient light for the scene.
    const AmbientLight* getAmbientLight() const { return _ambientLight; }
    void setAmbientLight(AmbientLight* newamb) { _ambientLight = newamb; }

    // Determine the size of the light set.
    size_t numLights() const { return _lights.size(); }

    // Append a light to the light set.  Note that if the renderer has already
    // started preprocess will not be called on new lights.
    void add(Light* light) { _lights.push_back(light); }

    // Remove the light from the list.  This can have rerecusions if local
    // LightSet objects are being used in a material, as we can't track this
    // light to them.  It is up to the external entity managing the scene to
    // make sure consistency is observed.  USE WITH EXTREME CAUTION.
    void remove(Light* light);

    // Accessors.
    const Light* getLight(size_t which) const { return _lights[which]; }
    Light* getLight(size_t which)             { return _lights[which]; }

    // Combine two light sets.
    static LightSet* merge(LightSet* l1, LightSet* l2);

    // Calls preprocess on each light.
    void preprocess(const PreprocessContext&);

    string toString() const;

    void readwrite(ArchiveElement* archive);
  private:
    LightSet(const LightSet&);
    LightSet& operator=(const LightSet&);

    vector<Light*> _lights;
    AmbientLight* _ambientLight;
  };
}

#endif
