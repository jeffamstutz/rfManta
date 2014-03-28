# MANTA_SSE_FLAGS contain flags needed for SSE compilation
#
# Cache variables:
# MANTA_SSE      - If SSE is enabled.  It will be forcibly disabled if
#                  no SSE support found.
# MANTA_SSE_GCC  - If the epi64x intrinsics are present
# MANTA_SSE_CAST - If the casting intrinsics are present
# MANTA_SSE_TEST_VERBOSE - Make the detection more chatty


##################################################################
# Test compilation.

SET(MANTA_SSE_TEST_RESULT 0)

# First try.
SET(SSE_TEST_FILE ${CMAKE_SOURCE_DIR}/CMake/sseTest.cc)
TRY_COMPILE(MANTA_SSE_TEST_RESULT ${CMAKE_BINARY_DIR}/testSSE ${SSE_TEST_FILE}
  CMAKE_FLAGS ${CMAKE_CXX_FLAGS}
  OUTPUT_VARIABLE MANTA_SSE_TEST_OUTPUT
  )

SET(MANTA_SSE_TEST_VERBOSE FALSE CACHE BOOL "Turn out verbose SSE checks")

IF(MANTA_SSE_TEST_VERBOSE)
  MESSAGE("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
  MESSAGE("MANTA_SSE_TEST_RESULT = ${MANTA_SSE_TEST_RESULT}")
  MESSAGE("MANTA_SSE_TEST_OUTPUT = ${MANTA_SSE_TEST_OUTPUT}")
ENDIF(MANTA_SSE_TEST_VERBOSE)

# If not, Try adding -msse flags and see if that makes it work.
IF (NOT MANTA_SSE_TEST_RESULT) 
  IF(MANTA_SSE_TEST_VERBOSE)
    MESSAGE("First sse build failed.  Trying to add some flags.")
  ENDIF(MANTA_SSE_TEST_VERBOSE)

  IF (USING_WINDOWS_CL)
    SET(MANTA_SSE_FLAGS "/arch:SSE2")
  ELSE(USING_WINDOWS_CL)
    SET(MANTA_SSE_FLAGS "-msse -msse2")
  ENDIF(USING_WINDOWS_CL)

  SET(SAFE_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MANTA_SSE_FLAGS}" CACHE STRING "" FORCE)

  # Test again.
  TRY_COMPILE(MANTA_SSE_TEST_RESULT ${CMAKE_BINARY_DIR}/testSSE ${SSE_TEST_FILE}
    OUTPUT_VARIABLE MANTA_SSE_TEST_OUTPUT
    )

  IF(MANTA_SSE_TEST_VERBOSE)
    MESSAGE("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
    MESSAGE("MANTA_SSE_TEST_RESULT = ${MANTA_SSE_TEST_RESULT}")
    MESSAGE("MANTA_SSE_TEST_OUTPUT = ${MANTA_SSE_TEST_OUTPUT}")
  ENDIF(MANTA_SSE_TEST_VERBOSE)

  # Reset the flags if they didn't help
  IF(NOT MANTA_SSE_TEST_RESULT)
    SET(CMAKE_CXX_FLAGS "${SAFE_CMAKE_CXX_FLAGS}" CACHE STRING "" FORCE)
  ENDIF(NOT MANTA_SSE_TEST_RESULT)

ENDIF(NOT MANTA_SSE_TEST_RESULT)

# Add additional try compile tests here...

##################################################################
# Check if any of the tests passed, and perform additional feature searches.

IF(MANTA_SSE_TEST_RESULT) # In other words, if any tests passed.

  IF(MANTA_SSE_TEST_VERBOSE)
    MESSAGE("We have SSE.  Now doing feature tests.")
  ENDIF(MANTA_SSE_TEST_VERBOSE)
  
  SET(MANTA_SSE TRUE CACHE BOOL "Compile SSE code.")

  # Check to see if the system is using gcc sse intrinsics.
  TRY_COMPILE(MANTA_SSE_TEST_RESULT ${CMAKE_BINARY_DIR}/testSSE ${SSE_TEST_FILE}
    COMPILE_DEFINITIONS -DMANTA_TEST_GCC
    OUTPUT_VARIABLE MANTA_SSE_TEST_OUTPUT
    )

  IF(MANTA_SSE_TEST_VERBOSE)
    MESSAGE("Test for GCC epi64 instrinsics")
    MESSAGE("MANTA_SSE_TEST_RESULT = ${MANTA_SSE_TEST_RESULT}")
    MESSAGE("MANTA_SSE_TEST_OUTPUT = ${MANTA_SSE_TEST_OUTPUT}")
  ENDIF(MANTA_SSE_TEST_VERBOSE)

  IF(MANTA_SSE_TEST_RESULT)
    SET(MANTA_SSE_GCC TRUE CACHE BOOL "Found *epi64x intrinsics")
  ELSE(MANTA_SSE_TEST_RESULT)
    SET(MANTA_SSE_GCC FALSE CACHE BOOL "Couldn't find *epi64x intrinsics" FORCE)
  ENDIF(MANTA_SSE_TEST_RESULT)

  # Check to see if the compiler has the cast intrinsics
  TRY_COMPILE(MANTA_SSE_TEST_RESULT ${CMAKE_BINARY_DIR}/testSSE ${SSE_TEST_FILE}
    COMPILE_DEFINITIONS -DMANTA_TEST_CAST
    OUTPUT_VARIABLE MANTA_SSE_TEST_OUTPUT
    )

  IF(MANTA_SSE_TEST_VERBOSE)
    MESSAGE("Test for casting instrinsics")
    MESSAGE("MANTA_SSE_TEST_RESULT = ${MANTA_SSE_TEST_RESULT}")
    MESSAGE("MANTA_SSE_TEST_OUTPUT = ${MANTA_SSE_TEST_OUTPUT}")
  ENDIF(MANTA_SSE_TEST_VERBOSE)

  IF(MANTA_SSE_TEST_RESULT)
    SET(MANTA_SSE_CAST TRUE CACHE BOOL "Found casting intrinsics")
  ELSE(MANTA_SSE_TEST_RESULT)
    SET(MANTA_SSE_CAST FALSE CACHE BOOL "Couldn't find casting intrinsics" FORCE)
  ENDIF(MANTA_SSE_TEST_RESULT)

ELSE(MANTA_SSE_TEST_RESULT) # All tests failed.

  IF(MANTA_SSE)
    MESSAGE("Couldn't compile with sse (already tried ${MANTA_SSE_FLAGS} options..).")
    MESSAGE("--debug-trycompile may be passed to CMake to avoid cleaning up temporary TRY_COMPILE files.")
    MESSAGE("Also try setting MANTA_SSE_TEST_VERBOSE to true in the advanced options to see compiler output.")
    MESSAGE("compiler:       " ${CMAKE_CXX_COMPILER})
    MESSAGE("compiler flags: " ${CMAKE_CXX_FLAGS})
    MESSAGE(${MANTA_SSE_TEST_OUTPUT})
  ENDIF(MANTA_SSE)

  # Force SSE off
  SET(MANTA_SSE FALSE CACHE BOOL "Couldn't compile SSE code." FORCE)

ENDIF(MANTA_SSE_TEST_RESULT)

IF (CYGWIN)
  IF (MANTA_SSE)
    MESSAGE("SSE is broken on Cygwin, because of global variable initialization and the lack of a GCC 4.x compiler.  SSE will be disabled for now.")
    SET(MANTA_SSE FALSE CACHE BOOL "SSE broken on Cygwin." FORCE)
  ENDIF (MANTA_SSE)
ENDIF (CYGWIN)

# Turn off other SSE variables if SSE isn't available
IF(NOT MANTA_SSE)
  SET(MANTA_SSE_GCC FALSE CACHE BOOL "Couldn't find *epi64x intrinsics." FORCE)
  SET(MANTA_SSE_CAST FALSE CACHE BOOL "Couldn't find casting intrinsics" FORCE)
ENDIF(NOT MANTA_SSE)

MARK_AS_ADVANCED(MANTA_SSE_GCC)
MARK_AS_ADVANCED(MANTA_SSE_CAST)
MARK_AS_ADVANCED(MANTA_SSE_TEST_VERBOSE)
