
#include <Core/Persistent/Archive.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>
#include <Core/Exceptions/InternalError.h>
#include <string>

using namespace Manta;
using namespace std;

struct ArchiveType {
  string name;
  Archive* (*readopener)(const std::string&);
  Archive* (*writeopener)(const std::string&);
};

static ArchiveType* archiveTypes[10];
static int numArchiveTypes = 0;

bool Archive::registerArchiveType(const std::string& name,
                                  Archive* (*readopener)(const std::string&),
                                  Archive* (*writeopener)(const std::string&))
{
  int maxArchiveTypes = sizeof(archiveTypes)/sizeof(ArchiveType);
  if(numArchiveTypes >= maxArchiveTypes)
    throw InternalError("Maximum number of archive types exceeded");
  archiveTypes[numArchiveTypes] = new ArchiveType();
  archiveTypes[numArchiveTypes]->name = name;
  archiveTypes[numArchiveTypes]->readopener = readopener;
  archiveTypes[numArchiveTypes]->writeopener = writeopener;
  numArchiveTypes++;
  return true;
}

Archive* Archive::openForReading(const std::string& filename)
{
  for(int i=0;i<numArchiveTypes;i++){
    Archive* (*readopener)(const std::string&) = archiveTypes[i]->readopener;
    if(readopener){
      Archive* archive = (*readopener)(filename);
      if(archive)
        return archive;
    }
  }
  throw InputError("Cannot determine type of file for scene: " + filename);
}

Archive* Archive::openForWriting(const std::string& filename)
{
  Archive* result = 0;
  for(int i=0;i<numArchiveTypes;i++){
    Archive* (*writeopener)(const std::string&) = archiveTypes[i]->writeopener;
    if(writeopener){
      Archive* archive = (*writeopener)(filename);
      if(archive){
        if(result)
          throw OutputError("Multiple archive writers support filename: " + filename);
        result = archive;
      }
    }
  }
  if(!result)
    throw OutputError("Cannot determine archive writer for filename: " + filename);
  return result;
}

Archive::Archive(bool isreading)
  : isreading(isreading)
{
}

Archive::~Archive()
{
}

