
SET (Manta_Display_SRCS
  Display/FileDisplay.cc
  Display/FileDisplay.h
  Display/MultiDisplay.cc
  Display/MultiDisplay.h
  Display/NullDisplay.cc
  Display/NullDisplay.h
  Display/PureOpenGLDisplay.cc
  Display/PureOpenGLDisplay.h
  Display/SyncDisplay.cc
  Display/SyncDisplay.h
  )

IF(MANTA_ENABLE_X11)
  SET (Manta_Display_SRCS ${Manta_Display_SRCS}
    Display/OpenGLDisplay.cc
    Display/OpenGLDisplay.h
    )
ENDIF(MANTA_ENABLE_X11)
