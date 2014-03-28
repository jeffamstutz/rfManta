# Where the source code lives
SET (CTEST_SOURCE_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/..")
SET (CTEST_BINARY_DIRECTORY "${CTEST_SOURCE_DIRECTORY}/build-ctest-nosse-float")

# Make sure we always reconfigure cmake stuff from scratch and don't
# rely on previously built libraries
SET (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)

SET (CTEST_CMAKE_COMMAND "cmake")
SET (CTEST_CVS_COMMAND "svn")

# A smoke test only builds the code and doesn't run any tests, so we
# exclude all tests here
SET (CTEST_COMMAND "ctest -D Nightly")
#SET (CTEST_COMMAND "ctest -D Experimental")

SET(CTEST_INITIAL_CACHE "
  MAKECOMMAND:STRING=/usr/bin/make -j16
  BUILDNAME:STRING=NoSSE-Float
  SCENE_GALILEO:BOOL=ON
  SCENE_GRIDISOVOL:BOOL=ON
  SCENE_OCTISOVOL:BOOL=ON
  SCENE_VORPAL:BOOL=ON
  SCENE_VOLUMETEST:BOOL=ON
  BUILD_SWIG_INTERFACE:BOOL=ON
  BUILD_TESTING:BOOL=ON
  MANTA_SSE:BOOL=OFF
  MANTA_SSE_GCC:BOOL=OFF
  MANTA_REAL:STRING=float
")

# SITE:STRING=cibc-rd1.sci.utah.edu
#   COMPARE_IMAGES:BOOL=ON
#   BASELINE_IMAGES_DIR:PATH=/home/sci/scirun-tester/Data/CIBCData/BaselineImages/Linux
#   RUN_CLASS_TESTS:BOOL=ON
#   RUN_SAMPLE_TESTS:BOOL=ON
#   RUN_UNIT_TESTS:BOOL=ON

# SET(CTEST_ENVIRONMENT
#   "SCIRUN_DATA=/home/sci/scirun-tester/Data/CIBCData/SCIRunData"
# )
