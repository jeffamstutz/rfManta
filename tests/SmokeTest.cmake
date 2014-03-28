# Where the source code lives
SET (CTEST_SOURCE_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/..")
SET (CTEST_BINARY_DIRECTORY "${CTEST_SOURCE_DIRECTORY}/build-ctest")

# Make sure we always reconfigure cmake stuff from scratch and don't
# rely on previously built libraries
SET (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)

SET (CTEST_CMAKE_COMMAND "cmake")
SET (CTEST_CVS_COMMAND "svn")

# A smoke test only builds the code and doesn't run any tests, so we
# exclude all tests here
SET (CTEST_COMMAND "ctest -D Nightly -E \".*\"")
#SET (CTEST_COMMAND "ctest -D Experimental -E \".*\"")

SET(CTEST_INITIAL_CACHE "
  SCENE_GALILEO:BOOL=ON
  SCENE_GRIDISOVOL:BOOL=ON
  SCENE_OCTISOVOL:BOOL=ON
  SCENE_VORPAL:BOOL=ON
  BUILD_SWIG_INTERFACE:BOOL=ON
")
