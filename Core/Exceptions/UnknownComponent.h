
#ifndef Manta_Core_UnknownComponent_h
#define Manta_Core_UnknownComponent_h

#include <Core/Exceptions/Exception.h>
#include <string>

namespace Manta {
  using namespace std;
  class UnknownComponent : public Exception {
  public:
    UnknownComponent(const std::string& error, const std::string& spec);
    UnknownComponent(const UnknownComponent&);
    virtual ~UnknownComponent() throw();
    virtual const char* message() const;
    virtual const char* type() const;
  protected:
  private:
    std::string message_;
    UnknownComponent& operator=(const UnknownComponent&);
  };
}

#endif
