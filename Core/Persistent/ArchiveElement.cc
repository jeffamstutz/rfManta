
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>
#include <Core/Exceptions/InternalError.h>
#include <string>

using namespace Manta;
using namespace std;

ArchiveElement::ArchiveElement(bool isreading)
  : isreading(isreading)
{
}

ArchiveElement::~ArchiveElement()
{
}
