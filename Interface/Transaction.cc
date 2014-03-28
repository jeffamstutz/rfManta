
#include <Interface/Transaction.h>

using namespace Manta;

TransactionBase::TransactionBase(const char* name, int flag_ )
  : name(name), flag( flag_ )
{
}

TransactionBase::~TransactionBase()
{
}
