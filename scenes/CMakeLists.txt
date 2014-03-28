
SET(BUILD_SHARED_LIBS ON)

############################################################
# Initial test scenes.

SET(MANTA_SCENE_LINK 
  ${MANTA_TARGET_LINK_LIBRARIES}
  )

# Basic test scene.
SET(SCENE_0 TRUE CACHE BOOL "Scene 0")
IF(SCENE_0)
   ADD_LIBRARY(scene_0 0.cc)
   TARGET_LINK_LIBRARIES(scene_0 ${MANTA_SCENE_LINK})
ENDIF(SCENE_0)

# Test different primitives.
SET(SCENE_PRIMTEST TRUE CACHE BOOL "Primitive Test")
IF(SCENE_PRIMTEST)
   ADD_LIBRARY(scene_primtest primtest.cc)
   TARGET_LINK_LIBRARIES(scene_primtest ${MANTA_SCENE_LINK})
ENDIF(SCENE_PRIMTEST)

# Test acceleration structure efficiency through random objects
SET(SCENE_COMPLEXITYTEST TRUE CACHE BOOL "Complexity Test")
IF(SCENE_COMPLEXITYTEST)
   ADD_LIBRARY(scene_complexitytest complexitytest.cc)
   TARGET_LINK_LIBRARIES(scene_complexitytest ${MANTA_SCENE_LINK})
ENDIF(SCENE_COMPLEXITYTEST)

# Old RTRT teapot scene
SET(SCENE_TEAPOT_ROOM TRUE CACHE BOOL "Old RTRT teapot scene")
IF(SCENE_TEAPOT_ROOM)
   ADD_LIBRARY(scene_teapotRoom teapotRoom.cc)
   TARGET_LINK_LIBRARIES(scene_teapotRoom ${MANTA_SCENE_LINK})
ENDIF(SCENE_TEAPOT_ROOM)

# softshadow via many point sources
SET(SCENE_SOFT_SHADOW TRUE CACHE BOOL "Soft Shadow test")
IF(SCENE_SOFT_SHADOW)
   ADD_LIBRARY(scene_softshadow softshadow.cc)
   TARGET_LINK_LIBRARIES(scene_softshadow ${MANTA_SCENE_LINK})
ENDIF(SCENE_SOFT_SHADOW)

# instances of fences
SET(SCENE_FENCE TRUE CACHE BOOL "fence test")
IF(SCENE_FENCE)
   ADD_LIBRARY(scene_fence fence.cc)
   TARGET_LINK_LIBRARIES(scene_fence ${MANTA_SCENE_LINK})
ENDIF(SCENE_FENCE)

# hdri lit scene 
SET(SCENE_HDRI TRUE CACHE BOOL "HDRI test")
IF(SCENE_HDRI)
   ADD_LIBRARY(scene_hdri hdritest.cc)
   TARGET_LINK_LIBRARIES(scene_hdri ${MANTA_SCENE_LINK})
ENDIF(SCENE_HDRI)

IF(FOUND_TEEM AND MANTA_SSE)
SET(SCENE_VOLUMETEST TRUE CACHE BOOL "volume test")
IF(SCENE_VOLUMETEST)
   INCLUDE_DIRECTORIES(${TEEM_INCLUDE_DIRS})
   ADD_LIBRARY(scene_volumeTest volumeTest.cc)
   TARGET_LINK_LIBRARIES(scene_volumeTest ${MANTA_SCENE_LINK})
ENDIF(SCENE_VOLUMETEST)
ENDIF(FOUND_TEEM AND MANTA_SSE)

IF(FOUND_TEEM AND MANTA_SSE AND BUILD_SWIG_INTERFACE)
SET(SCENE_CSAFE TRUE CACHE BOOL "csafe demo")
IF(SCENE_CSAFE)
  # The CSAFE stuff requires nrrd particles.
  SET(BUILD_NRRDPARTICLES TRUE CACHE BOOL "Build NRRD particle data reader/scene" FORCE)
  INCLUDE_DIRECTORIES(${TEEM_INCLUDE_DIRS})
   SUBDIRS(csafe)
#   ADD_LIBRARY(scene_volumeTest volumeTest.cc)
#   TARGET_LINK_LIBRARIES
ENDIF(SCENE_CSAFE)
ENDIF(FOUND_TEEM AND MANTA_SSE AND BUILD_SWIG_INTERFACE)

# octree isosurface
SET(SCENE_OCTISOVOL TRUE CACHE BOOL "octree isosurface")
IF(SCENE_OCTISOVOL)
   ADD_LIBRARY(scene_octisovol octisovol.cc)
   TARGET_LINK_LIBRARIES(scene_octisovol ${MANTA_SCENE_LINK})
ENDIF(SCENE_OCTISOVOL)

# grid isosurface
SET(SCENE_GRIDISOVOL TRUE CACHE BOOL "grid isosurface")
IF(SCENE_GRIDISOVOL)
   ADD_LIBRARY(scene_gridisovol gridisovol.cc)
   TARGET_LINK_LIBRARIES(scene_gridisovol ${MANTA_SCENE_LINK})
ENDIF(SCENE_GRIDISOVOL)

# triangle scene viewer loads obj, ply, and animations made of those files.
SET(SCENE_TRIANGLESCENEVIEWER TRUE CACHE BOOL "triangle scene viewer (ply, obj, animations...)")
IF(SCENE_TRIANGLESCENEVIEWER)
   ADD_LIBRARY(scene_triangleSceneViewer triangleSceneViewer.cc)
   TARGET_LINK_LIBRARIES(scene_triangleSceneViewer ${MANTA_SCENE_LINK})
ENDIF(SCENE_TRIANGLESCENEVIEWER)

# Tylenol (acetaminophen aka paracetamol) molecule with CPK coloring.
# Test silhouette edge and intersection edge rendering.
SET(SCENE_TYLENOL TRUE CACHE BOOL "tylenol molecule")
IF(SCENE_TYLENOL)
   ADD_LIBRARY(scene_tylenol tylenol.cc)
   TARGET_LINK_LIBRARIES(scene_tylenol ${MANTA_SCENE_LINK})
ENDIF(SCENE_TYLENOL)

# Value primitive and colormap testing scene.
SET(SCENE_VALUE_COLORMAP TRUE CACHE BOOL "value colormap test")
IF(SCENE_VALUE_COLORMAP)
   ADD_LIBRARY(scene_value_colormap_test valuecolormaptest.cc)
   TARGET_LINK_LIBRARIES(scene_value_colormap_test ${MANTA_SCENE_LINK})
ENDIF(SCENE_VALUE_COLORMAP)

# Lazily evaluated LTs for NRRD particle datasets
IF(BUILD_DYNLT)
   ADD_LIBRARY(scene_dynlt dynlt.cc)
   TARGET_LINK_LIBRARIES(scene_dynlt Manta_DynLT ${MANTA_SCENE_LINK})
ENDIF(BUILD_DYNLT)

# Recurse into galileo directory
SUBDIRS(galileo)

SET(SCENE_VORPAL TRUE CACHE BOOL "VORPAL fusion datasets")
IF(SCENE_VORPAL)
   ADD_LIBRARY(scene_vorpal vorpal.cc)
   TARGET_LINK_LIBRARIES(scene_vorpal ${MANTA_SCENE_LINK})
ENDIF(SCENE_VORPAL)

SET(SCENE_AREA_LIGHT TRUE CACHE BOOL "An area light test scene")
IF(SCENE_AREA_LIGHT)
   ADD_LIBRARY(scene_area_light area_light.cc)
   TARGET_LINK_LIBRARIES(scene_area_light ${MANTA_SCENE_LINK})
ENDIF(SCENE_AREA_LIGHT)

# macbeth color checker scene
SET(SCENE_MACBETH TRUE CACHE BOOL "Macbeth test")
IF(SCENE_MACBETH)
   ADD_LIBRARY(scene_macbeth macbeth.cc)
   TARGET_LINK_LIBRARIES(scene_macbeth ${MANTA_SCENE_LINK})
ENDIF(SCENE_MACBETH)

# BEACH color checker scene
SET(SCENE_BEACH FALSE CACHE BOOL "Beach Scene by Patricia Hendricks")
IF(SCENE_BEACH)
   ADD_LIBRARY(scene_beach beach.cc)
   TARGET_LINK_LIBRARIES(scene_beach ${MANTA_SCENE_LINK})
ENDIF(SCENE_BEACH)

SET(SCENE_EXTERNAL_OBJECT TRUE CACHE BOOL "Externally derived object unit test")
IF(SCENE_EXTERNAL_OBJECT)
   ADD_LIBRARY(scene_externalObject externalObject.cc)
   TARGET_LINK_LIBRARIES(scene_externalObject ${MANTA_SCENE_LINK})
ENDIF(SCENE_EXTERNAL_OBJECT)

# Loads oriented boxes to test acceleration structures (box are indivisible)
SET(SCENE_CUBE_WORLD TRUE CACHE BOOL "Loads sets of oriented boxes")
IF(SCENE_CUBE_WORLD)
   ADD_LIBRARY(scene_cubeWorld cubeWorld.cc)
   TARGET_LINK_LIBRARIES(scene_cubeWorld ${MANTA_SCENE_LINK})
ENDIF(SCENE_CUBE_WORLD)
