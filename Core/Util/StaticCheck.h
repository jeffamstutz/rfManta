#define USE_LOKI_STATIC_CHECK 1

#if USE_LOKI_STATIC_CHECK
// This version came from Loki.

// http://loki-lib.svn.sourceforge.net/viewvc/loki-lib/trunk/include/loki/static_check.h?revision=752&view=markup

////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any
//     purpose is hereby granted without fee, provided that the above copyright
//     notice appear in all copies and that both that copyright notice and this
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the
//     suitability of this software for any purpose. It is provided "as is"
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

namespace Manta {
  
  template<int> struct CompileTimeChecker;
  template<> struct CompileTimeChecker<true> { };

};

#define MANTA_STATIC_CHECK(expr, msg) \
    { Manta::CompileTimeChecker<((expr) != 0)> ERROR_##msg; (void)ERROR_##msg; }

#else

namespace Manta {
  // This version came from Modern C++ Design
  template<bool> struct CompileTimeChecker
  {
    CompileTimeChecker(...);
  };
  template<> struct CompileTimeChecker<false> {};
};

#define MANTA_STATIC_CHECK(expr, msg) \
    {\
        class ERROR_##msg {}; \
        (void)sizeof((Manta::CompileTimeChecker<(expr) != 0>(ERROR_##msg())));\
    }
#endif
