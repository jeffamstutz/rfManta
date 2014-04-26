/* *****************************************************************************
 *
 * Copyright (c) 2007-2013 Alexis Naveros.
 * Portions developed under contract to the SURVICE Engineering Company.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 *
 * *****************************************************************************
 */

#ifndef RFDEFS_H
#define RFDEFS_H

#ifdef __cplusplus
#  include <stdint.h>
#  include <stddef.h>
#endif // __cplusplus

#ifdef __cplusplus
 #include <stdint.h>
 #include <stddef.h>
#endif /* __cplusplus */


/* Rayforce config */

#ifndef RF_CONFIG_DOUBLE_PRECISION
 #define RF_CONFIG_DOUBLE_PRECISION (0)
#endif

#ifndef RF_CONFIG_NUMA_SUPPORT
 #define RF_CONFIG_NUMA_SUPPORT (1)
#endif

#ifndef RF_CONFIG_CUDA_SUPPORT
 #define RF_CONFIG_CUDA_SUPPORT (1)
#endif

#ifndef RF_CONFIG_OPENCL_SUPPORT
 #define RF_CONFIG_OPENCL_SUPPORT (0)
#endif

#ifndef RF_CONFIG_BIG_GRAPH_SUPPORT
 #define RF_CONFIG_BIG_GRAPH_SUPPORT (0)
#endif

#ifndef RF_CONFIG_PACK_MEMORY
 #define RF_CONFIG_PACK_MEMORY (0)
#endif



/* EXPERIMENTAL STUFF */
#define RF_CONFIG_ROOT_FORMAT (1)
/* EXPERIMENTAL STUFF */



/* Rayforce config */



#if !RF_CONFIG_DOUBLE_PRECISION
 #define RF_USE_FLOAT (1)
 #define RF_USE_DOUBLE (0)
typedef float rff;
 #define RF_RFF RF_FLOAT
 #define RFF_MAX FLT_MAX
 #define RFF_MIN FLT_MIN
 #define rffabs(x) fabsf(x)
 #define rffmax(x,y) fmaxf(x,y)
 #define rffmin(x,y) fminf(x,y)
 #define rffloor(x) floorf(x)
 #define rfsqrt(x) sqrtf(x)
 #define rfpow(x,y) powf(x,y)
 #define rflog(x) logf(x)
 #define rflog2(x) log2f(x)
 #define rfnextafter(x,y) nextafterf(x,y)
 #define rfMathMatrixIdentity(x) rfMathMatrixIdentityf(x);
#else
 #define RF_USE_FLOAT (0)
 #define RF_USE_DOUBLE (1)
typedef double rff;
 #define RF_RFF RF_DOUBLE
 #define RFF_MAX DBL_MAX
 #define RFF_MIN DBL_MIN
 #define rffabs(x) fabs(x)
 #define rffmax(x,y) fmax(x,y)
 #define rffmin(x,y) fmin(x,y)
 #define rffloor(x) floor(x)
 #define rfsqrt(x) sqrt(x)
 #define rfpow(x,y) pow(x,y)
 #define rflog(x) log(x)
 #define rflog2(x) log2(x)
 #define rfnextafter(x,y) nextafter(x,y)
 #define rfMathMatrixIdentity(x) rfMathMatrixIdentityd(x);
#endif


typedef int rfint;
typedef unsigned int rfuint;
typedef struct _rfContext rfContext;
typedef struct _rfScene rfScene;
typedef struct _rfHandle rfHandle;
typedef struct _rfPipeline rfPipeline;
typedef struct _rfModel rfModel;
typedef struct _rfObject rfObject;
#if ( __WORDSIZE == 64 ) || defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_WIN64)
 #define RF_ARCH_64_BITS
typedef uint64_t rfsize;
typedef int64_t rfssize;
#elif ( __WORDSIZE == 32 ) || defined(__i386__) || defined(__i386) || defined(i386) || defined(_WIN32)
 #define RF_ARCH_32_BITS
typedef uint32_t rfsize;
typedef int32_t rfssize;
#else
 #define RF_ARCH_64_BITS
#warning Unknown Arch, assuming 64 bits addressing
typedef uint64_t rfsize;
typedef int64_t rfssize;
#endif
typedef uint64_t rforigin;


#ifdef RF_CONFIG_BIG_GRAPH_SUPPORT
typedef uint64_t rfindex;
typedef int64_t rfsindex;
#else
typedef uint32_t rfindex;
typedef int32_t rfsindex;
#endif



#if RF_CONFIG_ROOT_FORMAT == 0
typedef uint32_t rfRoot;
 #define RF_SIZEOF_ROOT (4)
#elif RF_CONFIG_ROOT_FORMAT == 1
typedef uint64_t rfRoot;
 #define RF_SIZEOF_ROOT (8)
#elif RF_CONFIG_ROOT_FORMAT == 2
typedef struct
{
  uint32_t graphoffset;
  uint32_t extra;
} rfRoot;
 #define RF_SIZEOF_ROOT (8)
#elif RF_CONFIG_ROOT_FORMAT == 3
typedef struct
{
  uint64_t graphoffset;
  uint64_t extra;
} rfRoot;
 #define RF_SIZEOF_ROOT (16)
#endif



#define RF_ADDRESS(p,o) ((void *)(((char *)p)+(o)))
#define RF_ADDRESSDIFF(a,b) (((char *)a)-((char *)b))


#if defined(__CUDACC__)
 #define RF_ALIGN128 __align__(128)
 #define RF_ALIGN64 __align__(64)
 #define RF_ALIGN32 __align__(32)
 #define RF_ALIGN16 __align__(16)
 #define RF_ALIGN8 __align__(8)
 #define RF_ALWAYS_INLINE __forceinline__
 #define RF_HIDDEN
#elif defined(__GNUC__)
 #define RF_ALIGN128 __attribute__((aligned(128)))
 #define RF_ALIGN64 __attribute__((aligned(64)))
 #define RF_ALIGN32 __attribute__((aligned(32)))
 #define RF_ALIGN16 __attribute__((aligned(16)))
 #define RF_ALIGN8 __attribute__((aligned(8)))
 #define RF_ALWAYS_INLINE __attribute__((always_inline))
 #define RF_HIDDEN __attribute__((visibility("hidden")))
#elif defined(_MSC_VER)
 #define RF_ALIGN128 __declspec(align(128))
 #define RF_ALIGN64 __declspec(align(64))
 #define RF_ALIGN32 __declspec(align(32))
 #define RF_ALIGN16 __declspec(align(16))
 #define RF_ALIGN8 __declspec(align(8))
 #define RF_ALWAYS_INLINE inline
 #define RF_HIDDEN
#else
 #define RF_ALIGN128
 #define RF_ALIGN64
 #define RF_ALIGN32
 #define RF_ALIGN16
 #define RF_ALIGN8
 #define RF_ALWAYS_INLINE inline
 #define RF_HIDDEN
 #warning WARNING: rfdefs.h, unknown compiler!
#endif


#ifndef RF_INLINE
 #if defined(__GNUC__)
  #define RF_INLINE static inline __attribute__((always_inline))
 #else
  #define RF_INLINE static inline
 #endif
#endif


#ifndef RF_RESTRICT
 #if defined(__CUDACC__)
  #define RF_RESTRICT __restrict__
 #else
  #ifdef __cplusplus
   #if defined(__GNUC__)
    #define RF_RESTRICT __restrict__
   #elif defined(_MSC_VER)
    #define RF_RESTRICT __restrict
   #else
    #define RF_RESTRICT
   #endif
  #else
   #define RF_RESTRICT restrict
  #endif
 #endif
#endif


#define RF_SIZE_ROUND8(x) (((x)+0x7)&~0x7)
#define RF_SIZE_ROUND16(x) (((x)+0xf)&~0xf)
#define RF_SIZE_ROUND64(x) (((x)+0x3f)&~0x3f)


/**/


typedef struct
{
  /* Storage for packet parts */
  rff raysrc[4*3] RF_ALIGN16;
  rff vectinv[4*3] RF_ALIGN16;
  rff clipdist[4] RF_ALIGN16;

  /* Packet result */
  uint32_t hitmask;
  void *modeldata[4] RF_ALIGN16;
  void *objectdata[4] RF_ALIGN16;
  rff hitpt[4*3] RF_ALIGN16;
  rff hituv[4*3] RF_ALIGN16;
  void *hittri[4] RF_ALIGN16;
  rff hitdist[4] RF_ALIGN16;
  rff hitplane[4*4] RF_ALIGN16;
  uint32_t hitsidemask RF_ALIGN16;
  rfRoot root[4] RF_ALIGN16;
  rff *matrix[4] RF_ALIGN16;
} rfResult4;

typedef struct
{
  /* Storage for packet parts */
  rff raysrc[8*3] RF_ALIGN16;
  rff vectinv[8*3] RF_ALIGN16;
  rff clipdist[8] RF_ALIGN16;

  /* Packet result */
  uint32_t hitmask;
  void *modeldata[8] RF_ALIGN16;
  void *objectdata[8] RF_ALIGN16;
  rff hitpt[8*3] RF_ALIGN16;
  rff hituv[8*3] RF_ALIGN16;
  void *hittri[8] RF_ALIGN16;
  rff hitdist[8] RF_ALIGN16;
  rff hitplane[8*4] RF_ALIGN16;
  uint32_t hitsidemask RF_ALIGN16;
  rfRoot root[8] RF_ALIGN16;
  rff *matrix[8] RF_ALIGN16;
} rfResult8;

typedef struct
{
  /* Storage for packet parts */
  rff raysrc[16*3] RF_ALIGN16;
  rff vectinv[16*3] RF_ALIGN16;
  rff clipdist[16] RF_ALIGN16;

  /* Packet result */
  uint32_t hitmask;
  void *modeldata[16] RF_ALIGN16;
  void *objectdata[16] RF_ALIGN16;
  rff hitpt[16*3] RF_ALIGN16;
  rff hituv[16*3] RF_ALIGN16;
  void *hittri[16] RF_ALIGN16;
  rff hitdist[16] RF_ALIGN16;
  rff hitplane[16*4] RF_ALIGN16;
  uint32_t hitsidemask RF_ALIGN16;
  rfRoot root[16] RF_ALIGN16;
  rff *matrix[16] RF_ALIGN16;
} rfResult16;

typedef struct
{
  /* Storage for packet parts */
  rff raysrc[32*3] RF_ALIGN16;
  rff vectinv[32*3] RF_ALIGN16;
  rff clipdist[32] RF_ALIGN16;

  /* Packet result */
  uint32_t hitmask;
  void *modeldata[32] RF_ALIGN16;
  void *objectdata[32] RF_ALIGN16;
  rff hitpt[32*3] RF_ALIGN16;
  rff hituv[32*3] RF_ALIGN16;
  void *hittri[32] RF_ALIGN16;
  rff hitdist[32] RF_ALIGN16;
  rff hitplane[32*4] RF_ALIGN16;
  uint32_t hitsidemask RF_ALIGN16;
  rfRoot root[32] RF_ALIGN16;
  rff *matrix[32] RF_ALIGN16;
} rfResult32;


/**/


#define RF_LINK_NODE (0)
#define RF_LINK_SECTOR (1)

#define RF_NODE_LESS (0)
#define RF_NODE_MORE (1)
#define RF_NODE_DEPTH_SHIFT (4)

#define RF_AXIS_X (0)
#define RF_AXIS_Y (1)
#define RF_AXIS_Z (2)
#define RF_AXIS_COUNT (3)

#define RF_EDGE_MIN (0)
#define RF_EDGE_MAX (1)

#define RF_EDGE_AXIS_SHIFT (1)
#define RF_EDGE_X (RF_AXIS_X<<RF_EDGE_AXIS_SHIFT)
#define RF_EDGE_Y (RF_AXIS_Y<<RF_EDGE_AXIS_SHIFT)
#define RF_EDGE_Z (RF_AXIS_Z<<RF_EDGE_AXIS_SHIFT)

#define RF_EDGE_MINX (RF_EDGE_X|RF_EDGE_MIN)
#define RF_EDGE_MAXX (RF_EDGE_X|RF_EDGE_MAX)
#define RF_EDGE_MINY (RF_EDGE_Y|RF_EDGE_MIN)
#define RF_EDGE_MAXY (RF_EDGE_Y|RF_EDGE_MAX)
#define RF_EDGE_MINZ (RF_EDGE_Z|RF_EDGE_MIN)
#define RF_EDGE_MAXZ (RF_EDGE_Z|RF_EDGE_MAX)
#define RF_EDGE_COUNT (6)

#define RF_AXIS_TO_EDGE(axis,maxbit) (((axis)<<1)|(maxbit))
#define RF_EDGE_TO_AXIS(edge) ((edge)>>1)
#define RF_EDGE_TO_EDGEMIN(edge) ((edge)&~0x1)
#define RF_EDGE_TO_EDGEMAX(edge) ((edge)|0x1)
#define RF_OPPOSITE_EDGE(edge) ((edge)^0x1)
#define RF_EDGE_IS_MIN(edge) (!((edge)&0x1))
#define RF_EDGE_IS_MAX(edge) ((edge)&0x1)


/**/


#ifndef CHAR_BIT
 #define CHAR_BIT (8)
#endif

/*
rfRoot map
Bits  0...3 : Root type (sector, primitive, sectordisplace, node?)
Bits  4..57 : Root offset
Bits 58..63 : Primitive index
*/
#define RF_QUADTRACE_TYPEMASK ((size_t)0x3)

#define RF_QUADTRACE_TYPE_SECTOR ((size_t)0x0)
#define RF_QUADTRACE_TYPE_PRIMITIVE ((size_t)0x1)
#define RF_QUADTRACE_TYPE_SECTORDISPLACE ((size_t)0x2)

#define RF_QUADTRACE_PRIMINDEX_BITS (6)
#define RF_QUADTRACE_PRIMINDEX_MAX ((1<<RF_QUADTRACE_PRIMINDEX_BITS)-1)
#if RF_CONFIG_ROOT_FORMAT == 0
 #define RF_QUADTRACE_PRIMINDEX_SHIFT ((RF_SIZEOF_ROOT*CHAR_BIT)-RF_QUADTRACE_PRIMINDEX_BITS)
#elif RF_CONFIG_ROOT_FORMAT == 1
 #define RF_QUADTRACE_PRIMINDEX_SHIFT ((RF_SIZEOF_ROOT*CHAR_BIT)-RF_QUADTRACE_PRIMINDEX_BITS)
#elif RF_CONFIG_ROOT_FORMAT == 2
 #error FOO
#else
 #error FOO
#endif

#define RF_QUADTRACE_PRIMINDEXMASK (((size_t)RF_QUADTRACE_PRIMINDEX_MAX)<<RF_QUADTRACE_PRIMINDEX_SHIFT)


/**/


struct _rfPipeline
{
  uint32_t reqflags;
  void *object[3];
  void *scene[3];
};

/* Must not exceed 32 flags */
#define RF_PIPELINE_REQFLAG_SSE (1<<0)
#define RF_PIPELINE_REQFLAG_SSE2 (1<<1)
#define RF_PIPELINE_REQFLAG_SSE3 (1<<2)
#define RF_PIPELINE_REQFLAG_SSSE3 (1<<3)
#define RF_PIPELINE_REQFLAG_SSE4P1 (1<<4)
#define RF_PIPELINE_REQFLAG_SSE4P2 (1<<5)
#define RF_PIPELINE_REQFLAG_SSE4A (1<<6)
#define RF_PIPELINE_REQFLAG_AVX (1<<7)
#define RF_PIPELINE_REQFLAG_AVX2 (1<<8)
#define RF_PIPELINE_REQFLAG_XOP (1<<9)
#define RF_PIPELINE_REQFLAG_FMA3 (1<<10)
#define RF_PIPELINE_REQFLAG_FMA4 (1<<11)
#define RF_PIPELINE_REQFLAG_RDRND (1<<12)
#define RF_PIPELINE_REQFLAG_POPCNT (1<<13)
#define RF_PIPELINE_REQFLAG_LZCNT (1<<14)
#define RF_PIPELINE_REQFLAG_F16C (1<<15)
#define RF_PIPELINE_REQFLAG_BMI (1<<16)
#define RF_PIPELINE_REQFLAG_BMI2 (1<<17)
#define RF_PIPELINE_REQFLAG_TBM (1<<18)


/**/


#endif /* RFDEFS_H */

