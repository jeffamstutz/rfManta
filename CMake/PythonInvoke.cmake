#
# The contents of this archive are governed by the following license:
#
#  For more information, please see: http://software.sci.utah.edu
#
#  The MIT License
#
#  Copyright (c) 2008
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
#
# This macro creates an invocation shortcut for the specified python
# script.
# 
#   For example: 
#
#   PYTHON_ADD_EXECUTABLE(cuda_test
#     ${CMAKE_CURRENT_SOURCE_DIR}/cuda_test.py
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${LIBRARY_OUTPUT_PATH}
#     )
# 
#   Creates either an executable or script (depending on platform)
#   bin/cuda_test that executes the specified python file and sets the
#   python path to include the last two arguments.
#
###############################################################################



MACRO(PYTHON_ADD_EXECUTABLE python_target python_source)

  SET(PY_SOURCE_FILE ${python_source})

  # Create a helper invocation script or executable.
  IF (NOT WIN32)

    ###########################################################################
    # Apple: create a script (pythonw workaround).

    IF (APPLE)
      SET(PY_COMMAND pythonw)
    ELSE(APPLE)
      SET(PY_COMMAND python)
    ENDIF(APPLE)

    # Construct the python search path.
    SET(PY_SEARCH_PATH "")
    FOREACH(path ${ARGN})
	    IF (WIN32)
	      STRING(REPLACE "/" "\\\\" path ${path}) # Convert slashes to back slashes on windows.
	    ENDIF(WIN32)
      SET(PY_SEARCH_PATH "${PY_SEARCH_PATH}\nsetenv PYTHONPATH \"$PYTHONPATH\":${path}")
    ENDFOREACH(path)
    SET(PY_SEARCH_PATH "${PY_SEARCH_PATH}

if ( $?WXPATH ) then
  setenv PYTHONPATH $WXPATH\\:$PYTHONPATH
endif

if ( $?WXLIB ) then
  if ( $?LD_LIBRARY_PATH ) then
    setenv LD_LIBRARY_PATH $WXLIB\\:$LD_LIBRARY_PATH
  else
    setenv LD_LIBRARY_PATH $WXLIB
  endif
endif
\n"
)
    CONFIGURE_FILE (
      ${CMAKE_SOURCE_DIR}/CMake/python_invoke.csh.CMakeTemplate
      ${CMAKE_BINARY_DIR}/bin/${python_target}
      )

  ELSE (NOT WIN32) 

    ###########################################################################
    # Windows and Linux: create an executable (appears as target in visual studio).

    # Construct the python search path.
    SET(PY_SEARCH_PATH "")
    FOREACH(path ${ARGN})
	    IF (WIN32)
	      STRING(REPLACE "/" "\\\\" path ${path}) # Convert slashes to back slashes on windows.
	    ENDIF(WIN32)
      SET(PY_SEARCH_PATH "${PY_SEARCH_PATH}\n  PyRun_SimpleString(\"sys.path.append(r'${path}')\\n\");\n")
    ENDFOREACH(path)

    SET(target_source "${CMAKE_BINARY_DIR}/src/${python_target}-generated.cc")

    # Configure the invoker source file.
    CONFIGURE_FILE(
      ${CMAKE_SOURCE_DIR}/CMake/python_invoke.cc.CMakeTemplate
      ${target_source}
      )

    # Add python include directory.
    INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

    # Add the executable.
    ADD_EXECUTABLE(${python_target}
      ${target_source}
      )
    TARGET_LINK_LIBRARIES(${python_target}
      ${PYTHON_LIBRARY}
      )

  ENDIF (NOT WIN32)

ENDMACRO(PYTHON_ADD_EXECUTABLE)
