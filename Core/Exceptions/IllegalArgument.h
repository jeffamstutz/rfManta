
#ifndef Manta_Core_IllegalArgument_h
#define Manta_Core_IllegalArgument_h

#include <Core/Exceptions/Exception.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class IllegalArgument : public Exception {
  public:
    IllegalArgument(const std::string&, int, const vector<string>& args);
    IllegalArgument(const IllegalArgument&);
    virtual ~IllegalArgument() throw();
    virtual const char* message() const;
    virtual const char* type() const;
  protected:
  private:
    std::string message_;
    IllegalArgument& operator=(const IllegalArgument&);
  };
}

#endif
