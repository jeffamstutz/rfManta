
#ifndef Manta_Model_TimeSteppedParticles_h
#define Manta_Model_TimeSteppedParticles_h

#include <MantaTypes.h>
#include <Model/Groups/Group.h>
#include <Core/Thread/Mailbox.h>

#include <string>
using std::string;

#include <limits.h>

namespace Manta
{
  class RegularColorMap;

  class TimeSteppedParticles : public Group
  {
  public:
    TimeSteppedParticles(const string& filename, int ncells, int depth,
                         Real radius, int ridx, RegularColorMap* cmap, int cidx,
                         unsigned int min=0, unsigned int max=UINT_MAX);
    ~TimeSteppedParticles(void);

    void intersect(const RenderContext& context, RayPacket& rays) const;
    void computeBounds(const PreprocessContext& context, BBox& bbox) const;

    // GUI interface
    void next(void) { tstep=(tstep + 1)%size(); }
    void previous(void) { tstep=(tstep + size() - 1)%size(); }

  private:
    size_t tstep;
  };
}

#endif // Manta_Model_TimeSteppedParticles_h
