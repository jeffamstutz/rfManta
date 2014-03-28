
SET(Manta_UserInterface_SRCS
  AutomatorUI.h
  AutomatorUI.cc
  CameraPathAutomator.h
  CameraPathAutomator.cc
  NullUI.h
  # PromptUI.h
  # PromptUI.cc
  SyncFrameAutomator.cc
  SyncFrameAutomator.h
  )

IF (FOUND_TEEM) 
  INCLUDE_DIRECTORIES(${Teem_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Teem_LIBRARY_DIRS})
ENDIF (FOUND_TEEM)

IF(MANTA_ENABLE_X11)
  SET(Manta_UserInterface_SRCS ${Manta_UserInterface_SRCS}
    XWindowUI.h
    XWindowUI.cc
    )
ENDIF(MANTA_ENABLE_X11)

ADD_LIBRARY( Manta_UserInterface ${Manta_UserInterface_SRCS} )

TARGET_LINK_LIBRARIES(Manta_UserInterface 
                      Manta_Engine 
                      Manta_Interface
                      Manta_Core)

IF(MANTA_ENABLE_X11)
  TARGET_LINK_LIBRARIES(Manta_UserInterface
                        Manta_Core_XUtils)
ENDIF(MANTA_ENABLE_X11)
