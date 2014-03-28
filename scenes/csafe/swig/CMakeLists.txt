
#
#  For more information, please see: http://software.sci.utah.edu
#
#  The MIT License
#
#  Copyright (c) 2005-2006
#  Scientific Computing and Imaging Institute, University of Utah
#
#  License for the specific language governing rights and limitations under
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#

# Initialize Python/SWIG.
SET(CMAKE_SWIG_OUTDIR ${LIBRARY_OUTPUT_PATH})
FIND_PATH(SWIG_DIR swig)
FIND_PACKAGE(SWIG REQUIRED)

EXECUTE_PROCESS(COMMAND ${SWIG_EXECUTABLE} -version
                RESULT_VARIABLE status ERROR_VARIABLE swig-output
                OUTPUT_VARIABLE swig-output )

IF(status EQUAL 0)
ELSE(status EQUAL 0)
  MESSAGE( FATAL_ERROR "Running SWIG command (${SWIG_EXECUTABLE} -version) failed with error: ${status}" )
ENDIF(status EQUAL 0)

STRING( LENGTH '${swig-output}' length )

IF(length LESS 3)
  MESSAGE(FATAL_ERROR "SWIG did not return a valid version string: '${swig-output}'")
ENDIF(length LESS 3)

# string looks like: SWIG Version 1.3.31
STRING(REGEX MATCH "SWIG Version ${MANTA_THREE_PART_VERSION_REGEX}" version-string '${swig-output}' )
STRING(REGEX MATCH ${MANTA_THREE_PART_VERSION_REGEX} version-string '${version-string}' )

COMPARE_VERSION_STRINGS( '${version-string}' "1.3.31" swig-version-compare)
IF(swig-version-compare GREATER 0)
  # version > 1.3.31
  SET(warning_extra "-Wextra")
ENDIF(swig-version-compare GREATER 0)

# Important: Must use Manta's copy of UseSWIG.cmake
INCLUDE(${CMAKE_SOURCE_DIR}/CMake/MantaUseSWIG.cmake)

# CHANGE NAME OF SWIG INCLUDE FILE HERE.
SET_SOURCE_FILES_PROPERTIES(csafe.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(csafe.i 
  PROPERTIES SWIG_FLAGS "-I${CMAKE_SOURCE_DIR}/SwigInterface;${warning_extra};-w512;-w312")

# Add the python module "csafe" corresponding to shared lib _csafe.so
# CHANGE NAME OF PYTHON MODULE HERE.
SWIG_ADD_MODULE(csafe python csafe.i)

# CHANGE NAME OF PYTHON MODULE HERE.
SWIG_LINK_LIBRARIES(csafe
  ${PYTHON_LIBRARIES}
  ${MANTA_TARGET_LINK_LIBRARIES}
  
  # ADD ALL PROJECT SHARED LIBS HERE
  CSAFE
  # swigpy
  )
