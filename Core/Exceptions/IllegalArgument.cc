
#include <Core/Exceptions/IllegalArgument.h>
#include <sstream>

using namespace Manta;
using namespace std;

IllegalArgument::IllegalArgument(const std::string& why, int which,
				 const vector<string>& args)
{
  ostringstream msg;
  msg << "Illegal argument for " << why << ", rest of arguments are: ";
  for(unsigned long i = which; i<args.size();i++)
    msg << args[i] << " ";
  message_ = msg.str();
}

IllegalArgument::IllegalArgument(const IllegalArgument& copy)
    : message_(copy.message_)
{
}

IllegalArgument::~IllegalArgument() throw()
{
}

const char* IllegalArgument::message() const
{
    return message_.c_str();
}

const char* IllegalArgument::type() const
{
    return "IllegalArgument";
}

