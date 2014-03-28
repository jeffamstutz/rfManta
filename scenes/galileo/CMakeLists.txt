
SET(SCENE_GALILEO 0 CACHE BOOL "Galileo scene reader")

IF(SCENE_GALILEO)
  # Only look for the parsers if the Galileo scene is specified
  INCLUDE(${CMAKE_SOURCE_DIR}/CMake/FindParsers.cmake)

  IF(PARSERS_FOUND)

    GENERATE_BISON_FLEX_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/grparse.y ""
      ${CMAKE_CURRENT_SOURCE_DIR}/grlex.l   "")

    INCLUDE_DIRECTORIES(
      ${CMAKE_CURRENT_BINARY_DIR}
      )

    # This will eliminate the yyunput function that isn't used.
    ADD_DEFINITIONS(-DYY_NO_UNPUT)
    
    # Stop warnings from these generated files
    IF   (USING_GCC)
      SET_SOURCE_FILES_PROPERTIES(${BF_SOURCES}
        PROPERTIES COMPILE_FLAGS "-w")
    ENDIF(USING_GCC)

    ADD_LIBRARY(scene_galileo galileo.cc ${BF_SOURCES})
    TARGET_LINK_LIBRARIES(scene_galileo ${MANTA_SCENE_LINK})
    
  ELSE(PARSERS_FOUND)
    # Opps, didn't find the parsers
    MESSAGE(SEND_ERROR "Galileo scene build is specified, but either CMake isn't new enough or the parsing tools can't be found.\n\nIf you can't get it configured turn off the SCENE_GALILEO variable.")
  ENDIF(PARSERS_FOUND)

ENDIF(SCENE_GALILEO)
