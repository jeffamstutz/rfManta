
#include <Core/Exceptions/NullPointerException.h>

using namespace Manta;
using namespace std;

NullPointerException::NullPointerException(const std::type_info& t)
  : message_(string("Reference to null pointer of type: ") + t.name())
{
}

NullPointerException::NullPointerException(const NullPointerException& copy)
    : message_(copy.message_)
{
}

NullPointerException::~NullPointerException() throw()
{
}

const char* NullPointerException::message() const
{
    return message_.c_str();
}

const char* NullPointerException::type() const
{
    return "NullPointerException";
}

