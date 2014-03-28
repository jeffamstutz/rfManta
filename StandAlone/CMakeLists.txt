
ADD_EXECUTABLE(manta manta.cc)
TARGET_LINK_LIBRARIES(manta ${MANTA_TARGET_LINK_LIBRARIES})

IF(BUILD_TESTING)
  ADD_NP_TEST(1 DefaultSceneNoDisplayBench_NP1 ${CMAKE_BINARY_DIR}/bin/manta -np 1 -nodisplaybench-dart 200 20)
  ADD_NP_TEST(2 DefaultSceneNoDisplayBench_NP2 ${CMAKE_BINARY_DIR}/bin/manta -np 2 -nodisplaybench-dart 400 40)
  ADD_NP_TEST(4 DefaultSceneNoDisplayBench_NP4 ${CMAKE_BINARY_DIR}/bin/manta -np 4 -nodisplaybench-dart 800 80)

  # Search for the file
  FIND_FILE(SCI_MODEL_BUNNY bun_zipper.ply
    PATHS /usr/sci/data/Geometry/Stanford_Sculptures
    NO_DEFAULT_PATH)
  IF( SCI_MODEL_BUNNY )
	SET(IMAGE_TRAVERSER_ARG -imagetraverser "tiled(-square)")
	SET(CAMERA_ARG -camera "pinhole( -eye -0.0308032 0.257053 -0.0461074 -lookat -0.0396542 0.111762 -0.0132095 -up -0.072059 0.224437 0.971821 -hfov 60 -vfov 60 -normalizeRays -createCornerRays )")
	MACRO(ADD_BUNNY_TEST acceleration_structure NP)
    MATH(EXPR N_BENCH_FRAMES  "${NP} * 100")
    MATH(EXPR N_WARMUP_FRAMES "${NP} * 10")
	  ADD_NP_TEST(${NP} Bunny_${acceleration_structure}_NP${NP} ${CMAKE_BINARY_DIR}/bin/manta -np ${NP} -scene "${CMAKE_BINARY_DIR}/lib/libscene_triangleSceneViewer${CMAKE_SHARED_LIBRARY_SUFFIX}(-${acceleration_structure} -model ${SCI_MODEL_BUNNY} -triangleType Wald_tri)" ${IMAGE_TRAVERSER_ARG} ${CAMERA_ARG} -nodisplaybench-dart ${N_BENCH_FRAMES} ${N_WARMUP_FRAMES})
	ENDMACRO(ADD_BUNNY_TEST)

	ADD_BUNNY_TEST("DynBVH" 1)
	ADD_BUNNY_TEST("DynBVH" 2)
	ADD_BUNNY_TEST("KDTree" 1)
	ADD_BUNNY_TEST("KDTree" 2)
	IF(MANTA_SSE)
		ADD_BUNNY_TEST("CGT" 1)
		ADD_BUNNY_TEST("CGT" 2)
	ENDIF(MANTA_SSE)
  ENDIF( SCI_MODEL_BUNNY )

  SET(IMAGE_TRAVERSER_ARG  -imagetraverser "deadline (-dart_benchmark)")
  # We set nodisplaybench-dart to do a lot of frames so that the
  # deadline image traverser can finish before the benchmark finishes.
  MACRO(ADD_CORNELL_BOX_PATH_TRACED NP)
    # We could changed the resolution to try and make each benchmark take the
    # same amount of time per test.
    ADD_NP_TEST(${NP} CornellBoxPathTraced_NP${NP} ${CMAKE_BINARY_DIR}/bin/manta -np ${NP} -res 128x128 -maxdepth 2 -scene "${CMAKE_SOURCE_DIR}/scenes/cornell_box.rtml" -renderer pathtracer ${IMAGE_TRAVERSER_ARG} -nodisplaybench-dart 10000 10)
  ENDMACRO(ADD_CORNELL_BOX_PATH_TRACED)
  ADD_CORNELL_BOX_PATH_TRACED(1)
  ADD_CORNELL_BOX_PATH_TRACED(2)
  ADD_CORNELL_BOX_PATH_TRACED(4)

ENDIF(BUILD_TESTING)

SET (BUILD_OCTVOL_BUILD 0 CACHE BOOL "Include .v3c1 tools")
IF (BUILD_OCTVOL_BUILD) 
  ADD_EXECUTABLE(octvol_build octvol_build.cc)
  TARGET_LINK_LIBRARIES(octvol_build Manta_Model Manta_Engine ${SYSTEM_MATH_LIB})
ENDIF(BUILD_OCTVOL_BUILD) 
                                  
SET (BUILD_SAVESCENE FALSE CACHE BOOL "Build the scene read/write program (savescene)")
IF (BUILD_SAVESCENE)
  ADD_EXECUTABLE(savescene savescene.cc)
  TARGET_LINK_LIBRARIES(savescene ${MANTA_TARGET_LINK_LIBRARIES})
ENDIF (BUILD_SAVESCENE)

SET (BUILD_DISPLAY_TEST FALSE CACHE BOOL "Build the Display Test program (displaytest)")
IF (BUILD_DISPLAY_TEST)
  # Check for GLUT
  INCLUDE (${CMAKE_ROOT}/Modules/FindGLUT.cmake)
  IF (GLUT_FOUND)
    # Found glut
    INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
    ADD_EXECUTABLE(displaytest displaytest.cc)
    TARGET_LINK_LIBRARIES(displaytest ${MANTA_TARGET_LINK_LIBRARIES}
                                      ${GLUT_LIBRARIES}
                                      )
  ELSE (GLUT_FOUND)
    # Couldn't find it.
    MESSAGE("Need GLUT for BUILD_DISPLAY_TEST.  Turning off.")
    SET(BUILD_DISPLAY_TEST FALSE CACHE BOOL "Couldn't Find GLUT." FORCE)
  ENDIF (GLUT_FOUND)
ENDIF (BUILD_DISPLAY_TEST)
IF (BUILD_DISPLAY_TEST)
ENDIF(BUILD_DISPLAY_TEST)

