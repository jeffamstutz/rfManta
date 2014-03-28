
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

###############################################################################
# Example Manta Project.
# -- Links to Manta shared libraries.
# -- Shows how to write Manta components and place them in a separate 
#    shared library.
# -- Shows how to create python wrappers for new components.
# -- Shows how to write python scripts using the new components with Manta.
# 
# Abe Stephens
#
###############################################################################

###############################################################################
#
# STEP ONE: CMake Options
#

###############################################################################
#
# STEP TWO: Locate Manta.
#
# This script prompts the user to set the variable MANTA_SOURCE_DIR and 
# MANTA_BUILD_PREFIX. After this is done, the script sets variables:
#
# MANTA_INCLUDE               -- Paths containing Manta header files.
# MANTA_TARGET_LINK_LIBRARIES -- Path containing Manta shared libs.
#
# Additionally several .cmake scripts from the Manta build are executed to 
# insure a similar build environment will be used by the project.
#INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindManta.cmake)

# If certain SCIRun classes are used from Manta it is necessary to define
#FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS "-DSCI_NOPERSISTENT")
###############################################################################
#
# STEP THREE: Add new code in shared lib.
# 
# It's important that new Manta components be compiled into a shared library 
# so that python bindings may be created for them.

# # Initialize Python/SWIG.
# SET(CMAKE_SWIG_OUTDIR ${LIBRARY_OUTPUT_PATH})
# FIND_PATH(SWIG_DIR swig)
# FIND_PACKAGE(SWIG)

# # Important: Must use Manta's copy of UseSWIG.cmake
# INCLUDE(${MANTA_SOURCE_DIR}/CMake/MantaUseSWIG.cmake)

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(
  ${PYTHON_INCLUDE_PATH}
  )

# Include this project's source directory
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

# ADD YOUR OWN SOURCE CODE AND CHANGE LIBRARY NAME HERE.
ADD_LIBRARY(CSAFE
  src/CDSWIGIFY.h
  src/CDTest.h
  src/CDGridSpheres.cc
  src/CDGridSpheres.h
  src/CSAFEObject.cc
  src/CSAFEObject.h
  )


# CHANGE THE NAME OF YOUR LIBRARY HERE.
TARGET_LINK_LIBRARIES(CSAFE
  ${MANTA_TARGET_LINK_LIBRARIES}
  )


###############################################################################
#
# STEP FOUR: Create Python bindings for shared library.
#
# See swig/CMakeLists.txt
#  

SUBDIRS(
  swig
  python
  )
