
SET (Manta_Renderers_SRCS
     Renderers/KajiyaPathtracer.h
     Renderers/KajiyaPathtracer.cc
     Renderers/Moire.h
     Renderers/Moire.cc
     Renderers/Noise.h
     Renderers/Noise.cc
     Renderers/NPREdges.h
     Renderers/NPREdges.cc
     Renderers/NullRenderer.h
     Renderers/NullRenderer.cc
     Renderers/RayGen.h
     Renderers/RayGen.cc
     Renderers/Raytracer.h
     Renderers/Raytracer.cc
     )

IF (PABST_FOUND)
SET(Manta_Renderers_SRCS ${Manta_Renderers_SRCS}
			 Renderers/Raydumper.h
			 Renderers/Raydumper.cc
			 )
ENDIF (PABST_FOUND)
