# This will create a set of default compiler flags based on the system
# and compiler supplied.

##############################################################
## Compiler libraries
##############################################################

# Some compilers in non default locations have libraries they need in
# order to run properly.  You could change your LD_LIBRARY_PATH or you
# could add the path toth to the rpath of the library or executable.
# This helps with that.

IF(NOT COMPILER_LIBRARY_PATH)
  # Try and guess it
  IF(USING_ICC)
    SET(compiler_test_compiler ${CMAKE_C_COMPILER})
  ENDIF(USING_ICC)
  IF(USING_ICPC)
    SET(compiler_test_compiler ${CMAKE_CXX_COMPILER})
  ENDIF(USING_ICPC)
  IF(compiler_test_compiler)
    # TODO(bigler): get the full path of the compiler and guess the
    # library from that.
  ENDIF(compiler_test_compiler)
ENDIF(NOT COMPILER_LIBRARY_PATH)

SET(COMPILER_LIBRARY_PATH "${COMPILER_LIBRARY_PATH}" CACHE PATH "Path to compiler libraries" FORCE)

IF(EXISTS "${COMPILER_LIBRARY_PATH}")
  SET(rpath_arg "-Wl,-rpath,\"${COMPILER_LIBRARY_PATH}\"")
  # TODO(bigler): remove the old path if there is one
  FORCE_ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS ${rpath_arg})
  FORCE_ADD_FLAGS(CMAKE_MODULE_LINKER_FLAGS ${rpath_arg})
  FORCE_ADD_FLAGS(CMAKE_SHARED_LINKER_FLAGS ${rpath_arg})
ELSE(EXISTS "${COMPILER_LIBRARY_PATH}")
  IF(COMPILER_LIBRARY_PATH)
    MESSAGE(FATAL_ERROR "COMPILER_LIBRARY_PATH is set, but the path does not exist:\n${COMPILER_LIBRARY_PATH}")
  ENDIF(COMPILER_LIBRARY_PATH)
ENDIF(EXISTS "${COMPILER_LIBRARY_PATH}")

##############################################################
## System independent
##############################################################

# Initialize these parameters
SET(C_FLAGS "")
SET(C_FLAGS_DEBUG "")
SET(C_FLAGS_RELEASE "")

SET(CXX_FLAGS "")
SET(CXX_FLAGS_DEBUG "")
SET(CXX_FLAGS_RELEASE "")

SET(INTEL_OPT " ")
SET(GCC_OPT " ")
SET(CL_OPT " ")

# Set some defaults.  CMake provides some defaults in the INIT
# versions of the variables.
APPEND_TO_STRING(C_FLAGS "${CMAKE_C_FLAGS_INIT}")
#
APPEND_TO_STRING(CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT}")

# SSE flags
APPEND_TO_STRING(C_FLAGS "${MANTA_SSE_FLAGS}")
APPEND_TO_STRING(CXX_FLAGS "${MANTA_SSE_FLAGS}")

# Set the default warning levels for each compiler

####################
IF (APPLE_X86)
  #Something is broken on macs using icc where disabling warnings breaks
  #the compile. Hopefully this can be fixed!!
  SET(WARNING_FLAGS "-Wall")
ELSE (APPLE_X86)
  SET(WARNING_FLAGS "-Wall -wd193,383,424,981,1419,1572") #-Wcheck may also work
  # Solomon also uses these warning flags -wd1188,1476,1505
ENDIF(APPLE_X86)

SET(DEBUG_FLAGS "-O0 -g -restrict")
SET(RELEASE_FLAGS "-O3 -DNDEBUG -g -restrict")
IF   (USING_ICC)
  APPEND_TO_STRING(C_FLAGS         ${WARNING_FLAGS})
  APPEND_TO_STRING(C_FLAGS_DEBUG   ${DEBUG_FLAGS})
  APPEND_TO_STRING(C_FLAGS_RELEASE ${RELEASE_FLAGS})
ENDIF(USING_ICC)

IF   (USING_ICPC)
  APPEND_TO_STRING(CXX_FLAGS         ${WARNING_FLAGS})
  APPEND_TO_STRING(CXX_FLAGS_DEBUG   "${DEBUG_FLAGS}")
  APPEND_TO_STRING(CXX_FLAGS_RELEASE ${RELEASE_FLAGS})
ENDIF(USING_ICPC)

####################
# NOTE(boulos): Using SSE loads on float arrays causes a lot of these warnings.  We don't
# want to get rid of -fstrict-aliasing though, so I'm disabling the warning (19-Jul-2007)
###################
SET(WARNING_FLAGS "-Wall -Wno-strict-aliasing")
SET(DEBUG_FLAGS "-O0 -g3")

####################
# NOTE(boulos): We have traditionally included
# -ffast-math in our optimization line. However, for Manta r2158 this
# only makes a 1.5 percent difference in framerate on bin/manta and
# can drastically affect the performance of the DynBVH code by
# producing an incredibly variable output BVH. I'm removing it as of (1-Apr-2008)
####################

SET(RELEASE_FLAGS "-O3 -DNDEBUG -g3 -fgcse-sm -funroll-loops -fstrict-aliasing -fsched-interblock -freorder-blocks")
IF   (USING_GCC)
  APPEND_TO_STRING(C_FLAGS         ${WARNING_FLAGS})
  APPEND_TO_STRING(C_FLAGS_DEBUG   ${DEBUG_FLAGS})
  APPEND_TO_STRING(C_FLAGS_RELEASE ${RELEASE_FLAGS})
ENDIF(USING_GCC)

IF   (USING_GPP)
  APPEND_TO_STRING(CXX_FLAGS         ${WARNING_FLAGS})
  APPEND_TO_STRING(CXX_FLAGS_DEBUG   "${DEBUG_FLAGS}")
  APPEND_TO_STRING(CXX_FLAGS_RELEASE ${RELEASE_FLAGS})
ENDIF(USING_GPP)

SET(WARNING_FLAGS "/wd4305 /wd4996")
IF (USING_WINDOWS_CL)
  # These are the warnings
  APPEND_TO_STRING(C_FLAGS         ${WARNING_FLAGS})
  APPEND_TO_STRING(CXX_FLAGS         ${WARNING_FLAGS})
ENDIF(USING_WINDOWS_CL)

SET(WARNING_FLAGS "/D_CRT_SECURE_NO_DEPRECATE=1 /Qstd=c99")
IF (USING_WINDOWS_ICL)
  # These are the warnings
  APPEND_TO_STRING(C_FLAGS         ${WARNING_FLAGS})
  APPEND_TO_STRING(CXX_FLAGS         ${WARNING_FLAGS})
ENDIF(USING_WINDOWS_ICL)

##############################################################
## IA64
##############################################################
IF   (CMAKE_SYSTEM_PROCESSOR MATCHES "ia64")
  APPEND_TO_STRING(INTEL_OPT "")
ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "ia64")

##############################################################
## Apple
##############################################################
IF(APPLE)
  IF (APPLE_G4)
    APPEND_TO_STRING(GCC_OPT "-falign-loops=16 -falign-jumps=16 -falign-functions=16 -falign-jumps-max-skip=15 -falign-loops-max-skip=15 -force_cpusubtype_ALL -mtune=G4 -mcpu=G4 -faltivec -mabi=altivec -mpowerpc-gfxopt")
  ENDIF (APPLE_G4)

  # G5 Workstation.
  IF (APPLE_G5)
    APPEND_TO_STRING(GCC_OPT "-falign-loops=16 -falign-jumps=16 -falign-functions=16 -falign-jumps-max-skip=15 -falign-loops-max-skip=15 -mpowerpc-gpopt -force_cpusubtype_ALL -mtune=G5 -mcpu=G5 -mpowerpc64 -faltivec -mabi=altivec -mpowerpc-gfxopt" STRING "G5 Optimized Flags")
  ENDIF (APPLE_G5)

  # Macintel
  IF (APPLE_X86)
    APPEND_TO_STRING(GCC_ARCH "nocona")
    APPEND_TO_STRING(GCC_ARCH "prescott")
    APPEND_TO_STRING(GCC_OPT "-msse -msse2 -msse3 -mfpmath=sse")
  ENDIF (APPLE_X86)
ENDIF(APPLE)

##############################################################
## X86
##############################################################

# On apple machines CMAKE_SYSTEM_PROCESSOR return i386.

IF (CMAKE_SYSTEM_PROCESSOR MATCHES "i686" OR
    CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  APPEND_TO_STRING(GCC_OPT "-msse -msse2 -msse3 -mfpmath=sse")

  # mtune options

  INCLUDE(${CMAKE_SOURCE_DIR}/CMake/LinuxCPUInfo.cmake)

  # AMD
  IF(VENDOR_ID MATCHES "AuthenticAMD")
    APPEND_TO_STRING(GCC_ARCH "opteron") # supports 64 bit instructions
    APPEND_TO_STRING(GCC_ARCH "athlon-xp") # no support for 64 bit instructions
    APPEND_TO_STRING(INTEL_OPT "-xW -unroll4")
  ENDIF(VENDOR_ID MATCHES "AuthenticAMD")

  # Intel
  IF(VENDOR_ID MATCHES "GenuineIntel")

    IF(CPU_FAMILY EQUAL 6)

      IF(MODEL EQUAL 15) # (F)

        # This is likely a Core 2
        # APPEND_TO_STRING(GCC_ARCH "kentsfield") # QX6700
        APPEND_TO_STRING(GCC_ARCH "nocona")
        APPEND_TO_STRING(GCC_ARCH "prescott")

        # -xT  Intel(R) Core(TM)2 Duo processors, Intel(R) Core(TM)2 Quad
        # processors, and Intel(R) Xeon(R) processors with SSSE3
        APPEND_TO_STRING(INTEL_OPT "-xT -unroll4")

      ENDIF(MODEL EQUAL 15)

      IF(MODEL EQUAL 14) # (E)
        # This is likely a Core Single or Core Duo.  This doesn't
        # support EM64T.
        APPEND_TO_STRING(GCC_ARCH "prescott")
      ENDIF(MODEL EQUAL 14)
      IF(MODEL LESS 14) #(0-D)
        # This is likely a Pentium3, Pentium M.  Some pentium 3s don't
        # support sse2, in that case fall back to the i686 code.
        APPEND_TO_STRING(GCC_ARCH "pentium-m")
        APPEND_TO_STRING(INTEL_OPT "-xB")
      ENDIF(MODEL LESS 14)
    ENDIF(CPU_FAMILY EQUAL 6)
    IF(CPU_FAMILY EQUAL 15)
      # These are your Pentium 4 and friends
      IF(FLAGS MATCHES "em64t")
        APPEND_TO_STRING(GCC_ARCH "nocona")
        APPEND_TO_STRING(GCC_ARCH "prescott")
      ENDIF(FLAGS MATCHES "em64t")
      APPEND_TO_STRING(GCC_ARCH "pentium4")
      APPEND_TO_STRING(INTEL_OPT "-xP -unroll4 -msse3")
    ENDIF(CPU_FAMILY EQUAL 15)
  ENDIF(VENDOR_ID MATCHES "GenuineIntel")
  APPEND_TO_STRING(GCC_ARCH "i686")

  ###########################################################
  # Some x86_64 specific stuff
  IF   (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    APPEND_TO_STRING(INTEL_OPT "")
  ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  ###########################################################

ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "i686" OR
       CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")

##############################################################
## Configure Architecture
##############################################################

# Cycle through the GCC_ARCH args and see which one will pass first.
# Guard this evaluation with PASSED_FIRST_CONFIGURE, to make sure it
# is only done the first time.
IF(USING_GCC OR USING_GPP AND NOT PASSED_FIRST_CONFIGURE)
  SEPARATE_ARGUMENTS(GCC_ARCH)
  # Change the extension based of if we are using both gcc and g++.
  IF(USING_GCC)
    SET(EXTENSION "c")
  ELSE(USING_GCC)
    SET(EXTENSION "cc")
  ENDIF(USING_GCC)
  SET(COMPILE_TEST_SOURCE ${CMAKE_BINARY_DIR}/test/compile-test.${EXTENSION})
  CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/CMake/testmain.c"
    ${COMPILE_TEST_SOURCE} IMMEDIATE COPYONLY)
  FOREACH(ARCH ${GCC_ARCH})
    IF(NOT GOOD_ARCH)
#       MESSAGE("Testing ARCH = ${ARCH}")
      SET(ARCH_FLAG "-march=${ARCH} -mtune=${ARCH}")
      SET(COMPILER_ARGS "${ARCH_FLAG} ${C_FLAGS_RELEASE} ${C_FLAGS} ${GCC_OPT}")
      TRY_RUN(RUN_RESULT_VAR COMPILE_RESULT_VAR
        ${CMAKE_BINARY_DIR}/test ${COMPILE_TEST_SOURCE}
        CMAKE_FLAGS
        -DCOMPILE_DEFINITIONS:STRING=${COMPILER_ARGS}
        OUTPUT_VARIABLE OUTPUT
        )
#       MESSAGE("OUTPUT             = ${OUTPUT}")
#       MESSAGE("COMPILER_ARGS      = ${COMPILER_ARGS}")
#       MESSAGE("RUN_RESULT_VAR     = ${RUN_RESULT_VAR}")
#       MESSAGE("COMPILE_RESULT_VAR = ${COMPILE_RESULT_VAR}")
      IF(RUN_RESULT_VAR EQUAL 0)
        SET(GOOD_ARCH ${ARCH})
      ENDIF(RUN_RESULT_VAR EQUAL 0)
    ENDIF(NOT GOOD_ARCH)
  ENDFOREACH(ARCH)
  IF(GOOD_ARCH)
    PREPEND_TO_STRING(GCC_OPT "-march=${GOOD_ARCH} -mtune=${GOOD_ARCH}")
  ENDIF(GOOD_ARCH)
#   MESSAGE("GOOD_ARCH = ${GOOD_ARCH}")
ENDIF(USING_GCC OR USING_GPP AND NOT PASSED_FIRST_CONFIGURE)

# MESSAGE("CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
# MESSAGE("APPLE = ${APPLE}")
# MESSAGE("LINUX = ${LINUX}")

##############################################################
## Set the defaults
##############################################################

# MESSAGE("CMAKE_C_COMPILER   = ${CMAKE_C_COMPILER}")
# MESSAGE("CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")

# MESSAGE("USING_GCC  = ${USING_GCC}")
# MESSAGE("USING_GPP  = ${USING_GPP}")
# MESSAGE("USING_ICC  = ${USING_ICC}")
# MESSAGE("USING_ICPC = ${USING_ICPC}")
# MESSAGE("CMAKE version = ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
# MESSAGE("CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
# MESSAGE("CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")

MACRO(ADD_COMPILER_FLAG COMPILER FLAGS NEW_FLAG)
  IF(${COMPILER})
    PREPEND_TO_STRING(${FLAGS} ${${NEW_FLAG}})
  ENDIF(${COMPILER})
ENDMACRO(ADD_COMPILER_FLAG)

ADD_COMPILER_FLAG(USING_ICC    C_FLAGS_RELEASE INTEL_OPT)
ADD_COMPILER_FLAG(USING_ICPC CXX_FLAGS_RELEASE INTEL_OPT)
ADD_COMPILER_FLAG(USING_GCC    C_FLAGS_RELEASE GCC_OPT)
ADD_COMPILER_FLAG(USING_GPP  CXX_FLAGS_RELEASE GCC_OPT)

MACRO(SET_FLAGS FLAG NEW_VALUE)
  IF(${NEW_VALUE})
#     FIRST_TIME_MESSAGE("Setting compiler flags:")
#     FIRST_TIME_MESSAGE("${NEW_VALUE} = ${${NEW_VALUE}}")
    FIRST_TIME_SET(${FLAG} "${${NEW_VALUE}}" CACHE STRING "Default compiler flags" FORCE)
  ENDIF(${NEW_VALUE})
ENDMACRO(SET_FLAGS)

IF(UNIX)
  APPEND_TO_STRING(C_FLAGS "-fPIC")
  APPEND_TO_STRING(CXX_FLAGS "-fPIC")
ENDIF(UNIX)
  
SET_FLAGS(CMAKE_C_FLAGS         C_FLAGS)
SET_FLAGS(CMAKE_C_FLAGS_DEBUG   C_FLAGS_DEBUG)
SET_FLAGS(CMAKE_C_FLAGS_RELEASE C_FLAGS_RELEASE)

SET_FLAGS(CMAKE_CXX_FLAGS         CXX_FLAGS)
SET_FLAGS(CMAKE_CXX_FLAGS_DEBUG   CXX_FLAGS_DEBUG)
SET_FLAGS(CMAKE_CXX_FLAGS_RELEASE CXX_FLAGS_RELEASE)



