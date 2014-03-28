
SET (Manta_ImageTraversers_SRCS
     ImageTraversers/DeadlineImageTraverser.cc
     ImageTraversers/DeadlineImageTraverser.h
     ImageTraversers/DissolveImageTraverser.cc
     ImageTraversers/DissolveImageTraverser.h
     ImageTraversers/DissolveTiledImageTraverser.cc
     ImageTraversers/DissolveTiledImageTraverser.h
     ImageTraversers/FilteredImageTraverser.cc
     ImageTraversers/FilteredImageTraverser.h
     ImageTraversers/NullImageTraverser.cc
     ImageTraversers/NullImageTraverser.h
     ImageTraversers/TiledImageTraverser.cc
     ImageTraversers/TiledImageTraverser.h
     )

IF(ENABLE_MPI)
  SET (Manta_ImageTraversers_SRCS ${Manta_ImageTraversers_SRCS}
    ImageTraversers/MPI_ImageTraverser.cc
    ImageTraversers/MPI_ImageTraverser.h
    )
ENDIF(ENABLE_MPI)
