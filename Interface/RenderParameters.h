
#ifndef Manta_Interface_RenderParameters_h
#define Manta_Interface_RenderParameters_h

#include <MantaTypes.h>

namespace Manta {
  class ArchiveElement;
  class RenderParameters {
  public:
    RenderParameters() {
      // These are default values.  The scene can override them.
      maxDepth = 15;
      importanceCutoff = 0.01;
    }
    int maxDepth;
    Manta::Real importanceCutoff;

    // Helper functions for GUIs
    void changeMaxDepth(int change) {
      setMaxDepth(maxDepth + change);
    }
    void setMaxDepth(int new_maxDepth) {
      if (new_maxDepth >= 0)
        maxDepth = new_maxDepth;
    }

    void setImportanceCutoff(Manta::Real new_importanceCutoff) {
      if (new_importanceCutoff >= 0)
        importanceCutoff = new_importanceCutoff;
    }

    void readwrite(ArchiveElement* archive);
  private:
  };
}

#endif
