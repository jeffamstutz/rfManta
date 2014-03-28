# CTest is stupid in the sense that you can't pass in parameters to it from the
# command line.  I wish to do just this, so I will pass parameters to a .cmake
# script that will generate the cteat configuration and optionally run ctest.

# This will set the value of the variable only if it doesn't exist.  Setting the
# variable will overwrite any values passed in from the command line, so we need
# to check for its existence first.
MACRO(SET_PARAM VAR)
  IF(NOT DEFINED ${VAR})
    SET(${VAR} ${ARGN})
#   ELSE(NOT DEFINED ${VAR})
#     MESSAGE("Using command line value for <${VAR}> = ${${VAR}}")
  ENDIF(NOT DEFINED ${VAR})
ENDMACRO(SET_PARAM)

SET_PARAM("BUILD_DIR" "build-test")
SET_PARAM("CLEAN_BUILD" TRUE)
SET_PARAM("TEST_TYPE" Experimental) # also Nightly
SET_PARAM("NUM_CORES" 2)
SET_PARAM("ENABLE_SSE" TRUE)
SET_PARAM("REAL_TYPE" float)
SET_PARAM("SCRIPT_NAME" build-test.cmake)

# Do error checking
IF(NOT REAL_TYPE STREQUAL "float")
  IF(NOT REAL_TYPE STREQUAL "double")
    MESSAGE(FATAL_ERROR "Unknown REAL_TYPE ${REAL_TYPE}")
  ENDIF(NOT REAL_TYPE STREQUAL "double")
ENDIF(NOT REAL_TYPE STREQUAL "float")

FILE(WRITE "${SCRIPT_NAME}" "# This file was generated by the GenerateTest.cmake file, do not modify by hand unless you know what you are doing\n")

FILE(APPEND "${SCRIPT_NAME}" "SET (CTEST_SOURCE_DIRECTORY \"\${CTEST_SCRIPT_DIRECTORY}/..\")\n")
FILE(APPEND "${SCRIPT_NAME}" "SET (CTEST_BINARY_DIRECTORY \"\${CTEST_SOURCE_DIRECTORY}/${BUILD_DIR}\")\n")

IF(CLEAN_BUILD)
  FILE(APPEND "${SCRIPT_NAME}" "SET (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)\n")
ELSE(CLEAN_BUILD)
  FILE(APPEND "${SCRIPT_NAME}" "SET (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY FALSE)\n")
ENDIF(CLEAN_BUILD)

FILE(APPEND "${SCRIPT_NAME}" "SET (CTEST_CMAKE_COMMAND \"cmake\")\n")
FILE(APPEND "${SCRIPT_NAME}" "SET (CTEST_CVS_COMMAND \"svn\")\n")

# Based on the number of cores, we need to exclude some tests
MACRO(EXCLUDE_TEST_BY_NUM_CORES VAL)
  IF(NUM_CORES LESS ${VAL})
    LIST(APPEND np_exclude ${VAL})
  ENDIF(NUM_CORES LESS ${VAL})
ENDMACRO(EXCLUDE_TEST_BY_NUM_CORES)
EXCLUDE_TEST_BY_NUM_CORES(8)
EXCLUDE_TEST_BY_NUM_CORES(4)
EXCLUDE_TEST_BY_NUM_CORES(2)

LIST(LENGTH np_exclude num_excluded)
IF(num_excluded GREATER 0)
  LIST(GET np_exclude 0 np)
  SET(exclude "-E NP(${np}")
  LIST(REMOVE_AT np_exclude 0)
  FOREACH(np ${np_exclude})
    SET(exclude "${exclude}|${np}")
  ENDFOREACH(np)
  SET(exclude "${exclude})")
ENDIF(num_excluded GREATER 0)

FILE(APPEND "${SCRIPT_NAME}" "SET (CTEST_COMMAND \"ctest -D ${TEST_TYPE} ${exclude}\")\n")

FILE(APPEND "${SCRIPT_NAME}" "SET(CTEST_INITIAL_CACHE \"\n")

# Set options for parallel build.  If MAKE_PARALLEL is defined, then
# use that, otherwise use the number of cores.
IF(DEFINED MAKE_PARALLEL)
  SET(parallel ${MAKE_PARALLEL})
ELSE(DEFINED MAKE_PARALLEL)
  SET(parallel ${NUM_CORES})
ENDIF(DEFINED MAKE_PARALLEL)

IF(parallel GREATER 1)
  FILE(APPEND "${SCRIPT_NAME}" "  MAKECOMMAND:STRING=/usr/bin/make -j${parallel}\n")
ENDIF(parallel GREATER 1)

# Compute build name
IF(NOT BUILDNAME)
  IF(ENABLE_SSE)
    SET(BUILDNAME "SSE")
  ELSE(ENABLE_SSE)
    IF(REAL_TYPE STREQUAL "float")
      SET(BUILDNAME "NoSSE-Float")
    ELSE(REAL_TYPE STREQUAL "float")
      SET(BUILDNAME "NoSSE-Double")
    ENDIF(REAL_TYPE STREQUAL "float")
  ENDIF(ENABLE_SSE)
ENDIF(NOT BUILDNAME)
FILE(APPEND "${SCRIPT_NAME}" "  BUILDNAME:STRING=${BUILDNAME}\n")

# SSE stuff
IF(ENABLE_SSE)
  FILE(APPEND "${SCRIPT_NAME}" "  MANTA_SSE:BOOL=ON\n")
ELSE(ENABLE_SSE)
  FILE(APPEND "${SCRIPT_NAME}" "  MANTA_SSE:BOOL=OFF\n")
  FILE(APPEND "${SCRIPT_NAME}" "  MANTA_SSE_GCC:BOOL=OFF\n")
ENDIF(ENABLE_SSE)

# Real type
FILE(APPEND "${SCRIPT_NAME}" "  MANTA_REAL:STRING=${REAL_TYPE}\n")

# Scenes
FOREACH(scene GALILEO GRIDISOVOL OCTISOVOL VORPAL VOLUMETEST)
  FILE(APPEND "${SCRIPT_NAME}" "  SCENE_${scene}:BOOL=ON\n")
ENDFOREACH(scene)

# Options
FILE(APPEND "${SCRIPT_NAME}" "  BUILD_SWIG_INTERFACE:BOOL=ON\n")
FILE(APPEND "${SCRIPT_NAME}" "  BUILD_TESTING:BOOL=ON\n")
FILE(APPEND "${SCRIPT_NAME}" "  TEST_NUM_PROCS:INTEGER=${NUM_CORES}\n")

# End of cache variables
FILE(APPEND "${SCRIPT_NAME}" "\")\n")

# Environment variables
FILE(APPEND "${SCRIPT_NAME}" "SET(CTEST_ENVIRONMENT\n")
FILE(APPEND "${SCRIPT_NAME}" "  \"SCI_SIGNALMODE=exit\"\n") # Tells the thread library to exit when an exception is thrown
FILE(APPEND "${SCRIPT_NAME}" ")\n")

# Timeout
FILE(APPEND "${SCRIPT_NAME}" "SET(CTEST_TIMEOUT \"600\")\n")



