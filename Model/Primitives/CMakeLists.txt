
SET (Manta_Primitives_SRCS
     Primitives/Cone.cc
     Primitives/Cone.h
     Primitives/ConvexQuad.cc
     Primitives/ConvexQuad.h
     Primitives/Cube.cc
     Primitives/Cube.h
     Primitives/Cylinder.cc
     Primitives/Cylinder.h
     Primitives/Disk.cc
     Primitives/Disk.h
     Primitives/GridSpheres.cc
     Primitives/GridSpheres.h
     Primitives/Heightfield.cc
     Primitives/Heightfield.h
     Primitives/Hemisphere.cc
     Primitives/Hemisphere.h
     Primitives/OctreeVolume.cc
     Primitives/OctreeVolume.h
     Primitives/IsosurfaceGridVolume.cc
     Primitives/IsosurfaceGridVolume.h
     Primitives/IsosurfaceOctreeVolume.cc
     Primitives/IsosurfaceOctreeVolume.h
     Primitives/KenslerShirleyTriangle.h
     Primitives/KenslerShirleyTriangle.cc
     Primitives/MeshTriangle.cc
     Primitives/MeshTriangle.h
     Primitives/MovingKSTriangle.h
     Primitives/MovingKSTriangle.cc
     Primitives/Parallelogram.cc
     Primitives/Parallelogram.h
     Primitives/Plane.cc
     Primitives/Plane.h
     Primitives/PrimaryRaysOnly.cc
     Primitives/PrimaryRaysOnly.h
     Primitives/PrimitiveCommon.cc
     Primitives/PrimitiveCommon.h
     Primitives/QuadFacedHexahedron.cc
     Primitives/QuadFacedHexahedron.h
     Primitives/Ring.cc
     Primitives/Ring.h
     Primitives/Sphere.cc
     Primitives/Sphere.h
     Primitives/SuperEllipsoid.cc
     Primitives/SuperEllipsoid.h
     Primitives/TessellatedCylinder.cc
     Primitives/TessellatedCylinder.h
     Primitives/Torus.cc
     Primitives/Torus.h
     Primitives/ValuePrimitive.h
     Primitives/TextureCoordinateCylinder.cc
     Primitives/TextureCoordinateCylinder.h
     Primitives/TextureCoordinateSphere.cc
     Primitives/TextureCoordinateSphere.h
     Primitives/WaldTriangle.h
     Primitives/WaldTriangle.cc
     Primitives/BumpPrimitive.h
     Primitives/BumpPrimitive.cc
)

# IsosurfaceOctreeVolume appears broken in icc 9.1
#IF(INTEL_COMPILER_VERSION LESS 9.1)
#  SET(Manta_Primitives_SRCS
#      ${Manta_Primitives_SRCS}
#      Primitives/IsosurfaceOctreeVolume.cc
#      Primitives/IsosurfaceOctreeVolume.h
#     )
#ENDIF(INTEL_COMPILER_VERSION LESS 9.1)

