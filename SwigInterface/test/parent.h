
#include <iostream>
#include <string>

using namespace std;

namespace Test {
  class Parent {
  public:
    Parent(): name("Parent") {}
    Parent(const string& name): name(name) {}

    virtual void whoami() {
      cerr << "I am a Parent with name = "<<name<<"\n";
    }

    Parent* getSelf() { return this; }
  protected:
    string name;
  };

} // end namespace Test
