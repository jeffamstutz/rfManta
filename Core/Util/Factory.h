
#ifndef Manta_Core_Factory_h
#define Manta_Core_Factory_h

#include <map>

namespace Manta {
  using namespace std;
  template<typename CreatorType>
  class Factory {
  public:
    Factory();
    ~Factory();

    vector<string> listKinds() const;
    void registerKind(const string& name, CreatorType creator);
    bool setCurrentKind(const string&);
    CreatorType getCurrent() const {
      return current;
    }
    CreatorType lookup(const string& name);
  private:
    Factory(const Factory<CreatorType>&);
    Factory& operator=(const Factory<CreatorType>&);

    typedef map<string, CreatorType> MapType;
    CreatorType current;
  };
}

#endif
