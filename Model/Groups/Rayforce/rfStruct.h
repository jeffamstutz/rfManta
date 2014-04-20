
#ifndef RFSTRUCT_H
#define RFSTRUCT_H

#include <RF/rf.h>
#include <math.h>


typedef unsigned char      uchar;
typedef unsigned int       uint;
typedef unsigned long int  ulong;
typedef unsigned short int ushort;

// Useful constants ///////////////////////////////////////////////////////////

#define Pi M_PI
#define DegreesToRadians M_PI/180.f

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  // Per-triangle data
  typedef struct
  {
    size_t triID;
    int    matID;
  } rfTriangleData;

  // Per-ray data (primary rays)
  typedef struct
  {
    int    hit;
    float  minT;
    size_t triID;
    int    matID;
    float  normal[3];
  } rfRayData;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // RFSTRUCT_H
