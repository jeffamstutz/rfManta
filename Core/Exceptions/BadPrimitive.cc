
#include <Core/Exceptions/BadPrimitive.h>

using namespace Manta;
using namespace std;

BadPrimitive::BadPrimitive(const std::string& msg)
  : message_(msg)
{
}

BadPrimitive::BadPrimitive(const BadPrimitive& copy)
    : message_(copy.message_)
{
}

BadPrimitive::~BadPrimitive() throw()
{
}

const char* BadPrimitive::message() const
{
    return message_.c_str();
}

const char* BadPrimitive::type() const
{
    return "BadPrimitive";
}

