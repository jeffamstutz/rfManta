#include "child.h"

#include <iostream>

using namespace std;
using namespace Test;

Child::Child(int arg): Parent("Child"), arg(arg)
{
  whoami();
}

void Child::whoami() {
  cerr << "I am a Child with arg = "<<arg<<", and name = "<<name<<"\n";
}

BadChild::BadChild(int arg): Parent("BadChild"), arg(arg*2)
{
  whoami();
}

void BadChild::whoami()
{
  cerr << "I am a BadChild with arg = "<<arg<<", and name = "<<name<<"\n";
}

void BadChild::doBadStuff()
{
  arg *= 2;
}
  
