

#ifndef Manta_Interface_UserInterface_h
#define Manta_Interface_UserInterface_h

namespace Manta {

  class UserInterface {
  public:
    UserInterface();
    virtual ~UserInterface();

    // Call this function when you want the User interface to startup
    virtual void startup() = 0;

  private:
    UserInterface(const UserInterface&);
    UserInterface& operator=(const UserInterface&);
  };
}

#endif
