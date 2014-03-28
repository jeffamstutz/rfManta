#ifndef _MANTA_CORE_UTIL_PLUGIN_H_
#define _MANTA_CORE_UTIL_PLUGIN_H_

#include <string.h>
#include <string>
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <Core/Exceptions/InputError.h>

namespace Manta {
  template <class T>
  class Plugin {
  public:
    Plugin() {
      memset(name, 0, sizeof(name));
    }
    Plugin(const char* plugin_name) {
      memset(name, 0, sizeof(name));
      strncpy(name, plugin_name, sizeof(name));
      // load the shared object
#ifdef _WIN32
      object = LoadLibrary(name);
      if (!object)
        throw InputError("Can't open plugin: " + std::string(name));
#else
      object = dlopen(name, RTLD_NOW);
      if (!object)
        throw InputError("Can't open plugin : " + std::string(name) + ", dlerror = " + std::string(dlerror()));
#endif
    }

    ~Plugin() {
#ifdef _WIN32
      if (object)
        FreeLibrary(object);
#else
      if (object)
        dlclose(object);
#endif
    }

    bool loadSymbol(const char* symbol) {
      void* address = NULL;

#ifdef _WIN32
      address = (void*)GetProcAddress(object, symbol);
#else
      address = (void*)dlsym(object, symbol);
#endif
      if (!address) {
        throw InputError("Failed to find symbol " + std::string(symbol) + " in Plugin " + std::string(name) + "\n");
        return false;
      }

      function = (T)address;
      return true;
    }

    char name[1024];
#ifdef _WIN32
    typedef HMODULE DSOType;
#else // Unix systems
    typedef void* DSOType;
#endif
    DSOType object;
    T function;

  };
} // end namespace Manta

#endif // _MANTA_CORE_UTIL_PLUGIN_H_
