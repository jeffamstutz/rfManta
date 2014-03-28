
#include <Core/Exceptions/UnknownColor.h>

using namespace Manta;
using namespace std;

UnknownColor::UnknownColor(const std::string& msg)
  : message_(msg)
{
}

UnknownColor::UnknownColor(const UnknownColor& copy)
    : message_(copy.message_)
{
}

UnknownColor::~UnknownColor() throw()
{
}

const char* UnknownColor::message() const
{
    return message_.c_str();
}

const char* UnknownColor::type() const
{
    return "UnknownColor";
}

