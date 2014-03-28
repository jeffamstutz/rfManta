
# Default to building release.  I can't tell you how many times folks
# cry because Manta suddenly got slower.  This will hopefully
# alieviate it some.

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

###############################################################################
# Set SCI_ASSERTION_LEVEL based on build type

IF(CMAKE_BUILD_TYPE MATCHES "Release") 
  SET(SCI_ASSERTION_LEVEL 0 CACHE INT "Level for assertions [0-3]" FORCE)
ENDIF(CMAKE_BUILD_TYPE MATCHES "Release")

IF(CMAKE_BUILD_TYPE MATCHES "Debug") 
  IF (SCI_ASSERTION_LEVEL MATCHES 0)
     SET(SCI_ASSERTION_LEVEL 3 CACHE INT "Level for assertions [0-3]" FORCE)
  ENDIF (SCI_ASSERTION_LEVEL MATCHES 0)
ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")
