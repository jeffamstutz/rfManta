
###############################################################################
# This script saves svn revision info in a source file which may be used
# by the application to determine which revision has been compiled.
#
# The script creates the following advanced variable:
# ${ABOUT_SRC} -- Location of the source file containing the text.
#
# -- Abe Stephens
###############################################################################

SET(ABOUT_SRC ${CMAKE_BINARY_DIR}/src/About.cc)

# Configure the cmake script which will execute svn.
CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/CMake/ConfigureAbout.cmake.CMakeTemplate
  ${CMAKE_BINARY_DIR}/CMake/src/ConfigureAbout.cmake
  @ONLY
   )

# This custom command executes a cmake script which runs svn and parses output.
ADD_CUSTOM_COMMAND(
  OUTPUT  ${ABOUT_SRC}
  DEPENDS ${CMAKE_SOURCE_DIR}/.svn/entries ${CMAKE_CURRENT_SOURCE_DIR}/CMake/ConfigureAbout.cmake.CMakeTemplate
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/CMake/src/ConfigureAbout.cmake
  )  

# Since this is a generated file, mark it as such
SET_SOURCE_FILES_PROPERTIES(${ABOUT_SRC}
  PROPERTIES GENERATED TRUE
  )
  
# Explicitly add the -fPIC flag.
IF(NOT WIN32)
  SET_SOURCE_FILES_PROPERTIES(${ABOUT_SRC} PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF(NOT WIN32)

# Add a static library.
ADD_LIBRARY(About STATIC
  ${ABOUT_SRC}
  )

