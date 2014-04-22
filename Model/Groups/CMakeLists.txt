SET (Manta_Groups_SRCS
     Groups/CellSkipper.h
     Groups/CellSkipper.cc
     Groups/Clip.cc
     Groups/DynBVH.h
     Groups/DynBVH.cc
     Groups/Embree
     Groups/Group.cc
     Groups/Group.h
     Groups/KDTree.h
     Groups/KDTree.cc
     Groups/Mesh.cc
     Groups/Mesh.h
     Groups/MovingMesh.cc
     Groups/MovingMesh.h
     Groups/ObjGroup.h
     Groups/ObjGroup.cc
     Groups/RecursiveGrid
     Groups/RFGraph
     Groups/TreeTraversalProbability.cc
     Groups/BSP/BSP
     Groups/BSP/BSH
     Groups/BSP/Geometry
     Groups/BSP/Polytope
     Groups/BSP/aip.h
     Groups/BSP/chainHull.h
     Groups/Rayforce/Rayforce
     Groups/Rayforce/rfPipeline.c
     Groups/Rayforce/rfStruct.h
)

set_source_files_properties(Groups/Rayforce/rfPipeline.c
  PROPERTIES COMPILE_FLAGS "-msse4.1 -std=gnu99"
)

# Include private code if available
IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Groups/private )
#  SUBDIRS (Groups/private)
  INCLUDE (Groups/private/CMakeLists.txt)
ELSE (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Groups/private)
  SET(USE_PRIVATE_CODE_DEF "0" CACHE INTERNAL "Disable use of private code")
ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Groups/private)

CONFIGURE_FILE(
  ${CMAKE_SOURCE_DIR}/include/UsePrivateCode.h.CMakeTemplate
  ${CMAKE_BINARY_DIR}/include/UsePrivateCode.h
)


IF(BUILD_NRRDPARTICLES)
  SET(Manta_Groups_SRCS
    ${Manta_Groups_SRCS}
    Groups/TimeSteppedParticles.cc
    Groups/TimeSteppedParticles.h
   )
INCLUDE_DIRECTORIES(${FOUND_TEEM_INCLUDE})
ENDIF(BUILD_NRRDPARTICLES)
