#ifndef Manta_Model_ParticleNRRD_h
#define Manta_Model_ParticleNRRD_h

#include <string>
#include <vector>

using namespace std;

namespace Manta {
  class ParticleNRRD {
  public:
    ParticleNRRD(bool nuke=false);
    ParticleNRRD(string const& filename, bool nuke=false);
    ~ParticleNRRD(void);

    unsigned int getNVars(void) const { return nvars; }
    unsigned int getNParticles(void) const { return nparticles; }
    float* getParticleData(void) const { return pdata; }
    float* getParticleData(void) { return pdata; }

    void readFile(string const& filename);
    
    std::vector<std::pair<std::string, std::string> >& getKeyValuePairs()
      { return keyValuePairs; }

  private:
    float* pdata;
    unsigned int nvars;
    unsigned int nparticles;
    bool nuke;
    std::vector<std::pair<std::string, std::string> > keyValuePairs;
  };
}

#endif // Manta_Model_ParticleNRRD_h
