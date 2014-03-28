SET (Manta_Factory_SRCS
  Create.cc
  Create.h
  Factory.cc
  Factory.h
  RegisterKnownComponents.cc
  RegisterKnownComponents.h
  )

IF(MANTA_ENABLE_X11)
  SET(FLAG "-DMANTA_ENABLE_X11=1")
ELSE(MANTA_ENABLE_X11)
  SET(FLAG "-DMANTA_ENABLE_X11=0")
ENDIF(MANTA_ENABLE_X11)

SET_SOURCE_FILES_PROPERTIES(RegisterKnownComponents.cc
  PROPERTIES COMPILE_FLAGS ${FLAG}
  )

ADD_LIBRARY (Manta_Factory
  ${Manta_Factory_SRCS}
  )

TARGET_LINK_LIBRARIES(Manta_Factory
  Manta_UserInterface
  Manta_Engine
  Manta_Model
  Manta_Image
  Manta_Interface
  Manta_Core)
