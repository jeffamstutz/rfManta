
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>
#include <Model/Groups/TimeSteppedParticles.h>
#include <Model/Primitives/GridSpheres.h>
#include <Model/Readers/ParticleNRRD.h>

#include <fstream>
using std::ifstream;

#include <iostream>
using std::cerr;

using namespace Manta;

TimeSteppedParticles::TimeSteppedParticles(const string& filename, int ncells,
                                           int depth, Real radius, int ridx,
                                           RegularColorMap* cmap, int cidx,
                                           unsigned int min, unsigned int max) :
  tstep(0)
{
  // Check for a single timestep
  string::size_type pos=filename.find(".nrrd", 0);
  if (pos != string::npos) {
    ParticleNRRD pnrrd(filename);
    add(new GridSpheres(pnrrd.getParticleData(), pnrrd.getNParticles(),
                        pnrrd.getNVars(), ncells, depth, radius, ridx, cmap,
                        cidx));

    return;
  }

  // Load multiple timesteps
  ifstream in(filename.c_str());
  if (!in.is_open())
    throw InputError("Failed to open \"" + filename + "\" for reading\n");

  string fname;
  unsigned int nskipped=0;
  unsigned int nloaded=0;
  while (!in.eof() && min + nloaded < max) {
    // Read the timestep filename
    in>>fname;

    // Ignore timesteps below the minimum
    if (nskipped<min) {
      ++nskipped;
      continue;
    }

    // Load the particle data
    ParticleNRRD pnrrd(fname);

    add(new GridSpheres(pnrrd.getParticleData(), pnrrd.getNParticles(),
                        pnrrd.getNVars(), ncells, depth, radius, ridx, cmap,
                        cidx));


    ++nloaded;
  }

  in.close();
}

TimeSteppedParticles::~TimeSteppedParticles(void)
{
  // Do nothing
}

void TimeSteppedParticles::intersect(const RenderContext& context,
                                     RayPacket& rays) const
{
  // Fetch the current timestep
  const Object* obj=get(tstep);
  obj->intersect(context, rays);
}

void TimeSteppedParticles::computeBounds(const PreprocessContext& context,
                                         BBox& bbox) const
{
  // Fetch the current timestep
  const Object* obj=get(tstep);
  obj->computeBounds(context, bbox);
}
