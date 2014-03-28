# This file will set architecture and system variables.  Compiler
# flags should go into ConfigComipilerFlags.cmake.

# Assume that if we are running on an Itanium (ia64), then it is an
# SGI linux system
IF   (CMAKE_SYSTEM_PROCESSOR MATCHES "ia64")

  SET(SGI_LINUX TRUE)

ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "ia64")

IF (APPLE)

  # Obtain output of /usr/bin/macine
  EXEC_PROGRAM("/usr/bin/machine" OUTPUT_VARIABLE APPLE_MACHINE)

  # Apple G4
  IF (APPLE_MACHINE MATCHES "ppc7450")
    SET(APPLE_G4 TRUE)
  ENDIF (APPLE_MACHINE MATCHES "ppc7450")

  # Apple G5
  IF (APPLE_MACHINE MATCHES "ppc970")
    SET(APPLE_G5 TRUE)
  ENDIF(APPLE_MACHINE MATCHES "ppc970")

  # Intel
  IF (APPLE_MACHINE MATCHES "i486")
    SET(APPLE_X86 TRUE)
  ENDIF (APPLE_MACHINE MATCHES "i486")
    
#   MESSAGE("APPLE_G4  = ${APPLE_G4}")
#   MESSAGE("APPLE_G5  = ${APPLE_G5}")
#   MESSAGE("APPLE_X86 = ${APPLE_X86}")
ENDIF(APPLE)
