
#ifndef Manta_Core_NullPointerException_h
#define Manta_Core_NullPointerException_h

#include <Core/Exceptions/Exception.h>
#include <typeinfo>
#include <string>

namespace Manta {
  using namespace std;
  class NullPointerException : public Exception {
  public:
    NullPointerException(const std::type_info& t);
    NullPointerException(const NullPointerException&);
    virtual ~NullPointerException() throw();
    virtual const char* message() const;
    virtual const char* type() const;
  protected:
  private:
    std::string message_;
    NullPointerException& operator=(const NullPointerException&);
  };
}

#endif
