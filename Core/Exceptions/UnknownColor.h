
#ifndef Manta_Core_UnknownColor_h
#define Manta_Core_UnknownColor_h

#include <Core/Exceptions/Exception.h>
#include <string>

namespace Manta {
  using namespace std;
  class UnknownColor : public Exception {
  public:
    UnknownColor(const std::string&);
    UnknownColor(const UnknownColor&);
    virtual ~UnknownColor() throw();
    virtual const char* message() const;
    virtual const char* type() const;
  protected:
  private:
    std::string message_;
    UnknownColor& operator=(const UnknownColor&);
  };
}

#endif
