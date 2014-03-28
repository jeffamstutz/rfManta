
#ifndef Manta_Archive_h
#define Manta_Archive_h

#include <string>

namespace Manta {
  class ArchiveElement;
  class Archive {
  public:
    bool reading() const {
      return isreading;
    }
    bool writing() const {
      return !isreading;
    }
    static Archive* openForReading(const std::string& filename);
    static Archive* openForWriting(const std::string& filename);

    virtual ArchiveElement* getRoot() = 0;
    virtual ~Archive();
  protected:
    static bool registerArchiveType(const std::string& name,
                                    Archive* (*readopener)(const std::string&),
                                    Archive* (*writeopener)(const std::string&));
    Archive(bool isreading);
    bool isreading;
  private:
  };

}//namespace

#endif
