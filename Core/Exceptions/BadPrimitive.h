
#ifndef Manta_Core_BadPrimitive_h
#define Manta_Core_BadPrimitive_h

#include <Core/Exceptions/Exception.h>
#include <string>

namespace Manta {
  using namespace std;
  class BadPrimitive : public Exception {
  public:
    BadPrimitive(const std::string&);
    BadPrimitive(const BadPrimitive&);
    virtual ~BadPrimitive() throw();
    virtual const char* message() const;
    virtual const char* type() const;
  protected:
  private:
    std::string message_;
    BadPrimitive& operator=(const BadPrimitive&);
  };
}

#endif
