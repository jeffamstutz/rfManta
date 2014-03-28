
###############################################################################
###############################################################################
# Manta root CMakeLists.txt file.
###############################################################################
###############################################################################

# You need at least CMake version 2.4
cmake_minimum_required(VERSION 2.4)

# As of CMake 2.6 Policies were introduced in order to provide a mechanism for
# adding backwards compatibility one feature at a time.

# Run cmake --help-policy CMP<num> to see documentation.
if(COMMAND cmake_policy)
  # Library paths (/path/to/libmy.so not translated to -L/path/to -lmy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

PROJECT (Manta)

INCLUDE(CTest)
IF(BUILD_TESTING)
  ENABLE_TESTING()
  SET(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
  MARK_AS_ADVANCED(BUILDNAME)
  # Default this to 2 since most machines are now at least dual core.  You can
  # bump it up as needed.
  SET(TEST_NUM_PROCS 2 CACHE INTEGER "Maximum number of threads to use when running tests")
ENDIF(BUILD_TESTING)

SET(DEFAULT_BUILD_SHARED_LIBS ON)
IF (CYGWIN OR WIN32)
  # I can't get Manta to run with shared libraries.  This, at least,
  # let's me run for now.  I'm not sure how this will work for the
  # wxPython version.
  #
  # For now the windows version is built statically.
  SET(DEFAULT_BUILD_SHARED_LIBS OFF)
ENDIF (CYGWIN OR WIN32)
SET(BUILD_SHARED_LIBS ${DEFAULT_BUILD_SHARED_LIBS} CACHE BOOL "Building shared or static libraries")

# Turn off chatty makefiles by default.  To make them chatty again,
# call make with VERBOSE=1 like this:
#
# make VERBOSE=1

SET(CMAKE_VERBOSE_MAKEFILE OFF)

############################################################
# Set default search directory prefixes for third party libraries.

SET(THIRD_PARTY_LIBRARY_PATH "" CACHE PATH "Default search path for third-party libraries")
SET(THIRD_PARTY_INCLUDE_PATH "" CACHE PATH "Default search path for third-party headers")

###############################################################################
###############################################################################
# Configure build behavior
###############################################################################
###############################################################################

# Include helper cmake scripts

# Include all Manta CMake Macros.
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/Macros.cmake)
# Set flags based on the architecture we are on (no compiler stuff)
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/ConfigArchitecture.cmake)
# Determine information about the compiler
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/CompilerInfo.cmake)
# Set various options based on the build type.
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/BuildType.cmake)
# Check if the build supports SSE
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/CheckSSE.cmake)
# Add a static library build info and license.
#INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/About.cmake)
# Check for specific machine/compiler options.
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMake/ConfigCompilerFlags.cmake)

###############################################
# Search for common dependencies.

# Find X11 and GL, specialized on OSX and Cygwin
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindX11andGL.cmake)
# Locate Teem.
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindTeem.cmake)
# Locate pabst
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindPabst.cmake)
# Locate libxml2
INCLUDE (${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindLibXML2.cmake)
# Find Threads
INCLUDE (${CMAKE_ROOT}/Modules/FindThreads.cmake)
# Find ZLib
INCLUDE (${CMAKE_ROOT}/Modules/FindZLIB.cmake)

###############################################################################
###############################################################################
# Set some system level stuff
###############################################################################
###############################################################################

# Only unix systems need the math library
IF(UNIX)
  SET(SYSTEM_MATH_LIB -lm)
ELSE(UNIX)
  SET(SYSTEM_MATH_LIB)
ENDIF(UNIX)

SET(USE_STATS_COLLECTOR 0 CACHE BOOL "Enable stats collection")

SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "Single output directory for building all libraries.")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}/include
  ${OPENGL_INCLUDE_PATH}
  )

IF (FOUND_TEEM) 
  INCLUDE_DIRECTORIES(${Teem_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Teem_LIBRARY_DIRS})
ENDIF (FOUND_TEEM)

SET(MANTA_TARGET_LINK_LIBRARIES
  Manta_Factory
  Manta_UserInterface
  Manta_Engine
  Manta_Model
  Manta_Image
  Manta_Interface
  Manta_Core
  #About
  ${SYSTEM_MATH_LIB}
  )

IF(MANTA_ENABLE_X11)
 SET(MANTA_TARGET_LINK_LIBRARIES 
     ${MANTA_TARGET_LINK_LIBRARIES} 
     Manta_Core_XUtils)
ENDIF(MANTA_ENABLE_X11)


install (DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION include)
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION include)
install (DIRECTORY ${CMAKE_BINARY_DIR}/lib/  DESTINATION lib)

SUBDIRS(
  Core
  include
  Interface
  Image
  Model
  UserInterface
  Engine
  StandAlone
  scenes
  tests
  )


###############################################################################
###############################################################################
# OPTIONAL PROGRAMS AND LIBRARIES
###############################################################################
###############################################################################

# Optional Sub-directories
SUBDIRS_IF(BUILD_SWIG_INTERFACE "Check for swig/python" SwigInterface)

SET(BUILD_NRRDPARTICLES 0 CACHE BOOL "Build NRRD particle data reader/scene")

###############################################################################
###############################################################################
# Export some information on the libraries
###############################################################################
###############################################################################

EXPORT_LIBRARY_DEPENDENCIES(MantaConfigure.cmake)

###############################################################################
###############################################################################
# Now that everything is done indicate that we have finished
# configuring at least once.
SET(PASSED_FIRST_CONFIGURE ON CACHE INTERNAL "Already Configured once?")
###############################################################################
###############################################################################

