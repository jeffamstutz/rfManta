
# Since SWIG support wasn't added to CMake until version 2.0, only do
# the check if we have a new enough version.
SET(BUILD_SWIG_INTERFACE 0 CACHE BOOL "Check for swig/python")

IF(BUILD_SWIG_INTERFACE)

  IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.9)

    # This will dump all the interface files in the library path.  Note
    # that this needs to be set before you try to find swig.
    SET(CMAKE_SWIG_OUTDIR ${LIBRARY_OUTPUT_PATH})
    # Only process the swig directory if we have swig stuff

    # Search for swig in the system path.
    FIND_PATH(SWIG_DIR swig)
    FIND_PACKAGE(SWIG)
    IF (SWIG_FOUND)

      EXECUTE_PROCESS(COMMAND ${SWIG_EXECUTABLE} -version
        OUTPUT_VARIABLE swig-output ERROR_VARIABLE swig-output
        )
      # string looks like: SWIG Version 1.3.31
      STRING(REGEX MATCH "SWIG Version ${MANTA_THREE_PART_VERSION_REGEX}" version-string ${swig-output})
      STRING(REGEX MATCH ${MANTA_THREE_PART_VERSION_REGEX} version-string ${version-string})

      COMPARE_VERSION_STRINGS(${version-string} "1.3.23" swig-version-compare)
      IF(swig-version-compare LESS 0)
        # version < 1.3.23
        MESSAGE( FATAL_ERROR "Swig must be at least version 1.3.23. You have version ${version-string}." )
      ENDIF(swig-version-compare LESS 0)

      COMPARE_VERSION_STRINGS(${version-string} "1.3.31" swig-version-compare)
      IF(swig-version-compare GREATER 0)
        # version > 1.3.31
        SET(warning_extra "-Wextra")
      ENDIF(swig-version-compare GREATER 0)

      ###############################################################################
      FIND_PACKAGE(PythonLibs)
      INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

      # Use our local copy instead of the system one
      INCLUDE(${CMAKE_SOURCE_DIR}/CMake/MantaUseSWIG.cmake)

      INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

      SET_SOURCE_FILES_PROPERTIES(example.i PROPERTIES CPLUSPLUS ON)

      # OK, some versions of CMAKE need you to specify the SWIG_FLAGS
      # parameter.  You can't use a variable set to "", because that will
      # get replaced with a null string and the SET_SOURCE_FILES_PROPERTIES
      # macro will barf.

      # SET_SOURCE_FILES_PROPERTIES(example.i PROPERTIES SWIG_FLAGS "`echo \"\"`")
      # SWIG_ADD_MODULE(example python example.i example.cc)
      # SWIG_LINK_LIBRARIES(example ${PYTHON_LIBRARIES})

      # X11 flag
      IF(MANTA_ENABLE_X11)
        SET(X11_FLAG "-DMANTA_ENABLE_X11=1")
      ELSE(MANTA_ENABLE_X11)
        SET(X11_FLAG "-DMANTA_ENABLE_X11=0")
      ENDIF(MANTA_ENABLE_X11)

      # -Wall : Turn on all the warnings.
      # -w512 : Turn off warning 512.  Overloaded declaration const ignored. Non-const method at file:line used.
      # -w312 : Turn off nested class warning.
      SET(MANTA_SWIG_FLAGS ${warning_extra} -w512 -w312 ${X11_FLAG}) # Don't put quotes around it.  It needs to be a list.

      ############################################################
      # Manta Interface.
      SET_SOURCE_FILES_PROPERTIES(mantainterface.i PROPERTIES CPLUSPLUS ON)
      SET_SOURCE_FILES_PROPERTIES(mantainterface.i PROPERTIES SWIG_FLAGS "${MANTA_SWIG_FLAGS}")

      SWIG_ADD_MODULE(mantainterface python mantainterface.i manta.cc manta.h)

      SWIG_LINK_LIBRARIES(mantainterface
        ${PYTHON_LIBRARIES}
        ${MANTA_TARGET_LINK_LIBRARIES}
      )
      ADD_DEPENDENCIES(_mantainterface ${MANTA_TARGET_LINK_LIBRARIES})

      ############################################################
      # Manta Runtime.
      SET_SOURCE_FILES_PROPERTIES(manta.i PROPERTIES CPLUSPLUS ON)
      SET_SOURCE_FILES_PROPERTIES(manta.i PROPERTIES SWIG_FLAGS "${MANTA_SWIG_FLAGS}")

      SWIG_ADD_MODULE(manta python manta.i manta.cc manta.h)

      # Squash some warnings
      SET(swig_generated_file_extra_flags)
      IF   (USING_GCC)
        SET(swig_generated_file_extra_flags ${swig_generated_file_extra_flags}
          -Wno-uninitialized)
      ENDIF(USING_GCC)

      IF(swig_generated_file_extra_flags)
        SET_SOURCE_FILES_PROPERTIES(${swig_generated_sources}
          COMPILE_FLAGS ${swig_generated_file_extra_flags}
          )
      ENDIF(swig_generated_file_extra_flags)

      SWIG_LINK_LIBRARIES(manta
        ${PYTHON_LIBRARIES}
        ${MANTA_TARGET_LINK_LIBRARIES}
        )
      ADD_DEPENDENCIES(_manta ${MANTA_TARGET_LINK_LIBRARIES})


      ############################################################
      # wxManta Swig/Python

      # SET_SOURCE_FILES_PROPERTIES(wxManta_helper.i PROPERTIES CPLUSPLUS ON)
      # SET_SOURCE_FILES_PROPERTIES(wxManta_helper.i PROPERTIES SWIG_FLAGS "-Wall;-DSCI_NOPERSISTENT")
      # SWIG_ADD_MODULE(wxmanta_helper python wxManta_helper.i wxManta.cc wxManta.h)

      ############################################################
      # Python callback code
      SET_SOURCE_FILES_PROPERTIES(pycallback.i PROPERTIES CPLUSPLUS ON)
      SET_SOURCE_FILES_PROPERTIES(pycallback.i PROPERTIES SWIG_FLAGS "${MANTA_SWIG_FLAGS}")
      SWIG_ADD_MODULE(pycallback python pycallback.i pycallback.cc pycallback.h)
      SWIG_LINK_LIBRARIES(pycallback ${PYTHON_LIBRARIES})

      ############################################################
      # Output a helper script for setting up path variables.
      INCLUDE(${CMAKE_SOURCE_DIR}/CMake/PythonInvoke.cmake)

      PYTHON_ADD_EXECUTABLE(runwxmanta
        ${CMAKE_CURRENT_SOURCE_DIR}/runwxmanta.py
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBRARY_OUTPUT_PATH}
        )

      PYTHON_ADD_EXECUTABLE(objviewer
        ${CMAKE_CURRENT_SOURCE_DIR}/objviewer.py
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBRARY_OUTPUT_PATH}
        )

      FILE(WRITE ${CMAKE_BINARY_DIR}/bin/pythonpath.csh 
        "setenv PYTHONPATH ${CMAKE_SOURCE_DIR}/SwigInterface:${CMAKE_BINARY_DIR}/lib\n"
        "\n"
        "if ( $?WXPATH ) then\n"
        "  setenv PYTHONPATH $WXPATH\\:$PYTHONPATH\n"
        "endif\n"
        "\n"
        "if ( $?WXLIB ) then\n"
        "  if ( $?LD_LIBRARY_PATH ) then\n"
        "    setenv LD_LIBRARY_PATH $WXLIB\\:$LD_LIBRARY_PATH\n"
        "  else\n"
        "    setenv LD_LIBRARY_PATH $WXLIB\n"
        "  endif\n"
        "endif\n"
        "\n"
        )
      FILE(WRITE ${CMAKE_BINARY_DIR}/bin/pythonpath.sh 
        "export PYTHONPATH=${CMAKE_SOURCE_DIR}/SwigInterface:${CMAKE_BINARY_DIR}/lib\n\n")

      ###############################################################################
    ENDIF (SWIG_FOUND)
  ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.9)
ENDIF(BUILD_SWIG_INTERFACE)
