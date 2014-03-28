
#include <Interface/RenderParameters.h>
#include <Core/Persistent/ArchiveElement.h>

using namespace Manta;

void RenderParameters::readwrite(ArchiveElement* archive)
{
  archive->readwrite("maxDepth", maxDepth);
  archive->readwrite("importanceCutoff", importanceCutoff);
}
