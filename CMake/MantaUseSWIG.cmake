#
# SWIG module for CMake
# 
# Defines the following macros:
#
#   SWIG_ADD_MODULE(name language [ files ])
#     - Define swig module with given name and specified language
#
#   SWIG_LINK_LIBRARIES(name [ libraries ])
#     - Link libraries to swig module
#
# All other macros are for internal use only.
#
# To get the actual name of the swig module, use: ${SWIG_MODULE_name_REAL_NAME}.
# Set Source files propertis such as CPLUSPLUS and SWIG_FLAGS to specify
# special behavior of SWIG. Also global CMAKE_SWIG_FLAGS can be used to add
# special flags to all swig calls.
# Another special variable is CMAKE_SWIG_OUTDIR, it allows one to specify 
# where to write all the swig generated module (swig -outdir option)

#######################################################################
# This was taken and adapted from the CMake distribution.

# cmake version 2.2-patch 3
# Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  * The names of Kitware, Inc., the Insight Consortium, or the names of any
#     consortium members, or of any contributors, may not be used to endorse or
#     promote products derived from this software without specific prior
#     written permission.
#  * Modified source versions must be plainly marked as such, and must not be
#     misrepresented as being the original software.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Generate the empty.depend.in file necessary for
# dependency checking.
#

# Output the empty depend file.
# FILE(WRITE ${CMAKE_SOURCE_DIR}/CMake/empty.depend.in "")


# We need python to do some dependency checks
FIND_PACKAGE(PythonInterp)
IF(NOT PYTHONINTERP_FOUND)
  MESSAGE(FATAL_ERROR "Could not find python interpreter")
ENDIF(NOT PYTHONINTERP_FOUND)

SET(SWIG_CXX_EXTENSION "cxx")
SET(SWIG_EXTRA_LIBRARIES "")

SET(SWIG_PYTHON_EXTRA_FILE_EXTENSION "py")

#
# For given swig module initialize variables associated with it
#
MACRO(SWIG_MODULE_INITIALIZE name language)
  STRING(TOUPPER "${language}" swig_uppercase_language)
  STRING(TOLOWER "${language}" swig_lowercase_language)
  SET(SWIG_MODULE_${name}_LANGUAGE "${swig_uppercase_language}")
  SET(SWIG_MODULE_${name}_SWIG_LANGUAGE_FLAG "${swig_lowercase_language}")

  IF("x${SWIG_MODULE_${name}_LANGUAGE}x" MATCHES "^xUNKNOWNx$")
    MESSAGE(FATAL_ERROR "SWIG Error: Language \"${language}\" not found")
  ENDIF("x${SWIG_MODULE_${name}_LANGUAGE}x" MATCHES "^xUNKNOWNx$")

  SET(SWIG_MODULE_${name}_REAL_NAME "${name}")
  IF("x${SWIG_MODULE_${name}_LANGUAGE}x" MATCHES "^xPYTHONx$")
    SET(SWIG_MODULE_${name}_REAL_NAME "_${name}")
  ENDIF("x${SWIG_MODULE_${name}_LANGUAGE}x" MATCHES "^xPYTHONx$")
  IF("x${SWIG_MODULE_${name}_LANGUAGE}x" MATCHES "^xPERLx$")
    SET(SWIG_MODULE_${name}_EXTRA_FLAGS "-shadow")
  ENDIF("x${SWIG_MODULE_${name}_LANGUAGE}x" MATCHES "^xPERLx$")
ENDMACRO(SWIG_MODULE_INITIALIZE)

#
# For a given language, input file, and output file, determine extra files that
# will be generated. This is internal swig macro.
#

MACRO(SWIG_GET_EXTRA_OUTPUT_FILES language outfiles generatedpath infile)
  FOREACH(it ${SWIG_PYTHON_EXTRA_FILE_EXTENSION})
    SET(outfiles ${outfiles}
      "${generatedpath}/${infile}.${it}")
  ENDFOREACH(it)
ENDMACRO(SWIG_GET_EXTRA_OUTPUT_FILES)

#####################################################################
## MANTA_INCLUDE_SWIG_DEPENDENCIES
##

# So we want to try and include the dependency file if it exists.  If
# it doesn't exist then we need to create an empty one, so we can
# include it.

# If it does exist, then we need to check to see if all the files it
# depends on exist.  If they don't then we should clear the dependency
# file and regenerate it later.  This covers the case where a header
# file has disappeared or moved.

MACRO(MANTA_INCLUDE_SWIG_DEPENDENCIES dependency_file)
  SET(MANTA_SWIG_DEPEND)
  SET(MANTA_SWIG_DEPEND_REGENERATE)

  # Include the dependency file.  Create it first if it doesn't exist
  # for make files except for IDEs (see below).  The INCLUDE puts a
  # dependency that will force CMake to rerun and bring in the new info
  # when it changes.  DO NOT REMOVE THIS (as I did and spent a few hours
  # figuring out why it didn't work.
  IF(${CMAKE_MAKE_PROGRAM} MATCHES "make")
    IF(NOT EXISTS ${dependency_file})
      CONFIGURE_FILE(
        ${CMAKE_SOURCE_DIR}/CMake/empty.depend.in
        ${dependency_file} IMMEDIATE)
    ENDIF(NOT EXISTS ${dependency_file})
    # Always include this file to force CMake to run again next
    # invocation and rebuild the dependencies.
    INCLUDE(${dependency_file})
  ELSE(${CMAKE_MAKE_PROGRAM} MATCHES "make")
    # for IDE generators like MS dev only include the depend files
    # if they exist.   This is to prevent ecessive reloading of
    # workspaces after each build.   This also means
    # that the depends will not be correct until cmake
    # is run once after the build has completed once.
    # the depend files are created in the wrap tcl/python sections
    # when the .xml file is parsed.
    INCLUDE(${dependency_file} OPTIONAL)
  ENDIF(${CMAKE_MAKE_PROGRAM} MATCHES "make")

  # Now we need to verify the existence of all the included files
  # here.  If they aren't there we need to just blank this variable and
  # make the file regenerate again.
  IF(MANTA_SWIG_DEPEND)
    FOREACH(f ${MANTA_SWIG_DEPEND})
      IF(EXISTS ${f})
      ELSE(EXISTS ${f})
        SET(MANTA_SWIG_DEPEND_REGENERATE 1)
      ENDIF(EXISTS ${f})
    ENDFOREACH(f)
  ELSE(MANTA_SWIG_DEPEND)
    # No dependencies, so regenerate the file.
    SET(CABLE_SWIG_DEPEND_REGENERATE 1)
  ENDIF(MANTA_SWIG_DEPEND)

  # No incoming dependencies, so we need to generate them.  Make the
  # output depend on the dependency file itself, which should cause the
  # rule to re-run.
  IF(MANTA_SWIG_DEPEND_REGENERATE)
    SET(MANTA_SWIG_DEPEND ${dependency_file})
    # Force CMake to run again next build
    CONFIGURE_FILE(
      ${CMAKE_SOURCE_DIR}/CMake/empty.depend.in
      ${dependency_file} IMMEDIATE)
  ENDIF(MANTA_SWIG_DEPEND_REGENERATE)
      
ENDMACRO(MANTA_INCLUDE_SWIG_DEPENDENCIES)

#
# Take swig (*.i) file and add proper custom commands for it
#
MACRO(SWIG_ADD_SOURCE_TO_MODULE name outfiles infile)
  SET(swig_full_infile ${infile})
  GET_FILENAME_COMPONENT(swig_source_file_path "${infile}" PATH)
  GET_FILENAME_COMPONENT(swig_source_file_name_we "${infile}" NAME_WE)
  GET_SOURCE_FILE_PROPERTY(swig_source_file_generated ${infile} GENERATED)
  GET_SOURCE_FILE_PROPERTY(swig_source_file_cplusplus ${infile} CPLUSPLUS)
  GET_SOURCE_FILE_PROPERTY(swig_source_file_flags ${infile} SWIG_FLAGS)
  IF("${swig_source_file_flags}" STREQUAL "NOTFOUND")
    SET(swig_source_file_flags "")
  ENDIF("${swig_source_file_flags}" STREQUAL "NOTFOUND")
  SET(swig_source_file_fullname "${infile}")
  IF(${swig_source_file_path} MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}")
    STRING(REGEX REPLACE 
      "^${CMAKE_CURRENT_SOURCE_DIR}" ""
      swig_source_file_relative_path
      "${swig_source_file_path}")
  ELSE(${swig_source_file_path} MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}")
    IF(${swig_source_file_path} MATCHES "^${CMAKE_CURRENT_BINARY_DIR}")
      STRING(REGEX REPLACE 
        "^${CMAKE_CURRENT_BINARY_DIR}" ""
        swig_source_file_relative_path
        "${swig_source_file_path}")
      SET(swig_source_file_generated 1)
    ELSE(${swig_source_file_path} MATCHES "^${CMAKE_CURRENT_BINARY_DIR}")
      SET(swig_source_file_relative_path "${swig_source_file_path}")
      IF(swig_source_file_generated)
        SET(swig_source_file_fullname "${CMAKE_CURRENT_BINARY_DIR}/${infile}")
      ELSE(swig_source_file_generated)
        SET(swig_source_file_fullname "${CMAKE_CURRENT_SOURCE_DIR}/${infile}")
      ENDIF(swig_source_file_generated)
    ENDIF(${swig_source_file_path} MATCHES "^${CMAKE_CURRENT_BINARY_DIR}")
  ENDIF(${swig_source_file_path} MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}")

  SET(swig_generated_file_fullname
    "${CMAKE_CURRENT_BINARY_DIR}")
  IF(swig_source_file_relative_path)
    SET(swig_generated_file_fullname
      "${swig_generated_file_fullname}/${swig_source_file_relative_path}")
  ENDIF(swig_source_file_relative_path)
  SWIG_GET_EXTRA_OUTPUT_FILES(${SWIG_MODULE_${name}_LANGUAGE}
    swig_extra_generated_files
    "${swig_generated_file_fullname}"
    "${swig_source_file_name_we}")
  SET(swig_generated_file_fullname
    "${swig_generated_file_fullname}/${swig_source_file_name_we}")
  # add the language into the name of the file (i.e. TCL_wrap)
  # this allows for the same .i file to be wrapped into different languages
  SET(swig_generated_file_fullname
    "${swig_generated_file_fullname}${SWIG_MODULE_${name}_LANGUAGE}_wrap")

  IF(swig_source_file_cplusplus)
    SET(swig_generated_file_fullname
      "${swig_generated_file_fullname}.${SWIG_CXX_EXTENSION}")
  ELSE(swig_source_file_cplusplus)
    SET(swig_generated_file_fullname
      "${swig_generated_file_fullname}.c")
  ENDIF(swig_source_file_cplusplus)

  #MESSAGE("Full path to source file: ${swig_source_file_fullname}")
  #MESSAGE("Full path to the output file: ${swig_generated_file_fullname}")
  GET_DIRECTORY_PROPERTY(cmake_include_directories INCLUDE_DIRECTORIES)
  SET(swig_include_dirs)
  FOREACH(it ${cmake_include_directories})
    SET(swig_include_dirs ${swig_include_dirs} "-I${it}")
  ENDFOREACH(it)

  SET(swig_special_flags)
  # default is c, so add c++ flag if it is c++
  IF(swig_source_file_cplusplus)
    SET(swig_special_flags ${swig_special_flags} "-c++")
  ENDIF(swig_source_file_cplusplus)
  SET(swig_extra_flags)
  IF(SWIG_MODULE_${name}_EXTRA_FLAGS)
    SET(swig_extra_flags ${swig_extra_flags} ${SWIG_MODULE_${name}_EXTRA_FLAGS})
  ENDIF(SWIG_MODULE_${name}_EXTRA_FLAGS)

  # Bring in the dependencies.  Creates a variable MANTA_SWIG_DEPEND
  SET(cmake_dependency_file "${swig_generated_file_fullname}.depend")
  MANTA_INCLUDE_SWIG_DEPENDENCIES(${cmake_dependency_file})
  SET(swig_generated_dependency_file
    "${swig_generated_file_fullname}.swig-depend")

  # Add this argument if we need to.
  IF(CMAKE_SWIG_OUTDIR)
    SET(OUTDIR_ARG "-outdir" "${CMAKE_SWIG_OUTDIR}")
  ELSE(CMAKE_SWIG_OUTDIR)
    SET(OUTDIR_ARG "")
  ENDIF(CMAKE_SWIG_OUTDIR)

  # Put all the arguments in a single variable, so we can have a
  # centralized location.
  SET(ALL_SWIG_COMMAND_FLAGS
    "-${SWIG_MODULE_${name}_SWIG_LANGUAGE_FLAG}"
    ${swig_source_file_flags}
    ${CMAKE_SWIG_FLAGS}
    ${OUTDIR_ARG}
    ${swig_special_flags}
    ${swig_extra_flags}
    ${swig_include_dirs}
    -o ${swig_generated_file_fullname}
    )

  # Build the swig made dependency file
  ADD_CUSTOM_COMMAND(
    OUTPUT ${swig_generated_dependency_file}
    COMMAND ${SWIG_EXECUTABLE}
    ARGS ${ALL_SWIG_COMMAND_FLAGS}
    -M -MF ${swig_generated_dependency_file}
    ${swig_source_file_fullname}
    MAIN_DEPENDENCY ${swig_source_file_fullname}
    DEPENDS ${MANTA_SWIG_DEPEND}
    COMMENT "Swig generated dependency file (${swig_generated_dependency_file})"
    )

  # Build the CMake readible dependency file
  ADD_CUSTOM_COMMAND(
    OUTPUT ${cmake_dependency_file}
    COMMAND ${PYTHONINTERP}
    ARGS "${CMAKE_SOURCE_DIR}/CMake/make2cmake.py" ${swig_generated_dependency_file} ${cmake_dependency_file}
    MAIN_DEPENDENCY ${swig_generated_dependency_file}
    COMMENT "Converting swig dependency to CMake (${cmake_dependency_file})"
    )

  # Build the wrapper file.  This depends on the
  # cmake_dependency_file, so that stuff will get built too.
  ADD_CUSTOM_COMMAND(
    OUTPUT ${swig_generated_file_fullname}
    COMMAND "${SWIG_EXECUTABLE}"
    ARGS ${ALL_SWIG_COMMAND_FLAGS} ${swig_source_file_fullname}
    MAIN_DEPENDENCY "${swig_source_file_fullname}"
    DEPENDS ${MANTA_SWIG_DEPEND}
    DEPENDS ${cmake_dependency_file}
    COMMENT "Swig source (${swig_generated_file_fullname})"
    )
  
  SET_SOURCE_FILES_PROPERTIES("${swig_generated_file_fullname}"
    PROPERTIES GENERATED 1)
  SET(${outfiles} "${swig_generated_file_fullname}")
ENDMACRO(SWIG_ADD_SOURCE_TO_MODULE)

#
# Create Swig module
#
MACRO(SWIG_ADD_MODULE name language)
  SWIG_MODULE_INITIALIZE(${name} ${language})
  SET(swig_dot_i_sources)
  SET(swig_other_sources)
  FOREACH(it ${ARGN})
    IF(${it} MATCHES ".*\\.i$")
      SET(swig_dot_i_sources ${swig_dot_i_sources} "${it}")
    ELSE(${it} MATCHES ".*\\.i$")
      SET(swig_other_sources ${swig_other_sources} "${it}")
    ENDIF(${it} MATCHES ".*\\.i$")
  ENDFOREACH(it)

  SET(swig_generated_sources)
  FOREACH(it ${swig_dot_i_sources})
    SWIG_ADD_SOURCE_TO_MODULE(${name} swig_generated_source ${it})
    SET(swig_generated_sources ${swig_generated_sources} "${swig_generated_source}")
  ENDFOREACH(it)
  GET_DIRECTORY_PROPERTY(swig_extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "${swig_extra_clean_files};${swig_generated_sources}")
  ADD_LIBRARY(${SWIG_MODULE_${name}_REAL_NAME}
    MODULE
    ${swig_generated_sources}
    ${swig_other_sources})
  SET_TARGET_PROPERTIES(${SWIG_MODULE_${name}_REAL_NAME}
    PROPERTIES PREFIX "")
ENDMACRO(SWIG_ADD_MODULE)

#
# Like TARGET_LINK_LIBRARIES but for swig modules
#
MACRO(SWIG_LINK_LIBRARIES name)
  IF(SWIG_MODULE_${name}_REAL_NAME)
    TARGET_LINK_LIBRARIES(${SWIG_MODULE_${name}_REAL_NAME} ${ARGN})
  ELSE(SWIG_MODULE_${name}_REAL_NAME)
    MESSAGE(SEND_ERROR "Cannot find Swig library \"${name}\".")
  ENDIF(SWIG_MODULE_${name}_REAL_NAME)
ENDMACRO(SWIG_LINK_LIBRARIES name)



