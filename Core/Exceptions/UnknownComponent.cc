
#include <Core/Exceptions/UnknownComponent.h>
#include <sstream>

using namespace Manta;
using namespace std;

UnknownComponent::UnknownComponent(const std::string& error, const std::string& spec )
{
  ostringstream msg;
  msg << error << " Component spec: " << spec;
  
  message_ = msg.str();
}

UnknownComponent::UnknownComponent(const UnknownComponent& copy)
    : message_(copy.message_)
{
}

UnknownComponent::~UnknownComponent() throw()
{
}

const char* UnknownComponent::message() const
{
    return message_.c_str();
}

const char* UnknownComponent::type() const
{
    return "UnknownComponent";
}

