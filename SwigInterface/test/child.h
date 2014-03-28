#include "parent.h"

#include <string>

using namespace std;

namespace Test {
  class Child: public Parent {
  public:
    Child(int arg);
    virtual void whoami();

  private:
    int arg;
  };

  class BadChild: public Parent {
  public:
    BadChild(int arg);
    virtual void whoami();

    void doBadStuff();
  private:
    int arg;
  };
  
} // end namespace Test
