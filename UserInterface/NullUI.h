

#ifndef Manta_UserInterface_NullUI_h
#define Manta_UserInterface_NullUI_h


#include <Interface/UserInterface.h>
#include <Core/Thread/Runnable.h>
#include <vector>
#include <string>

namespace Manta {

	class MantaInterface;

	// This class implements a null user interface which should be used for 
	// benchmarking.
  class NullUI : public UserInterface {
	public:
    NullUI() {  }
    virtual ~NullUI() {  }
		
    // Call this function when you want the User interface to startup
    virtual void startup() { /* Does nothing. */ };
		
		static UserInterface* create(const vector<string>& args, MantaInterface *rtrt_int) {
			return new NullUI();
		}
	};
}

#endif
