
#include <Core/Exceptions/InputError.h>
#include <Model/Readers/ParticleNRRD.h>

#include <teem/nrrd.h>

#include <iostream>

using namespace Manta;
using namespace std;

ParticleNRRD::ParticleNRRD(bool nuke) :
  pdata(0), nvars(0), nparticles(0), nuke(nuke)
{
  // Do nothing
}

ParticleNRRD::ParticleNRRD(string const& filename, bool nuke) :
  pdata(0), nvars(0), nparticles(0), nuke(nuke)
{
  readFile(filename);
}

ParticleNRRD::~ParticleNRRD(void)
{
  if (nuke && pdata)
    delete pdata;
}

void ParticleNRRD::readFile(string const& filename)
{
  // Load particle data
  Nrrd* pnrrd=nrrdNew();
  if (nrrdLoad(pnrrd, filename.c_str(), 0)) {
    char* err=biffGetDone(NRRD);
    throw InputError("Failed to open \"" + filename + "\":  " + string(err));
  }
  
  // Initialize variables
  pdata=static_cast<float*>(pnrrd->data);
  nvars=pnrrd->axis[0].size;
  nparticles=pnrrd->axis[1].size;

  int ki, nk;
  char* key = NULL, *val = NULL;
  nk = nrrdKeyValueSize(pnrrd);
  for(ki=0;ki<nk;ki++) {
    nrrdKeyValueIndex(pnrrd, &key, &val, ki);
    keyValuePairs.push_back(pair<string, string>(key, val));
    free(key);
    free(val);
    key = val = NULL;
  }

  cout<<"Loading "<<nparticles<<" particles ("<<nvars
      <<" data values/particles) from \""<<filename<<"\"\n";

  pnrrd=nrrdNix(pnrrd);
}
