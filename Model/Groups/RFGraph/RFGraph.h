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

#ifndef RFGRAPH_H
#define RFGRAPH_H


typedef struct RF_ALIGN16
{
  /* 48 bytes */
  rff plane[4];
  rff edpu[4];
  rff edpv[4];
} rfTri;


typedef struct
{
  /* 40||44 bytes */
  rff edge[RF_EDGE_COUNT];
#if !RF_CONFIG_PACK_MEMORY
  uint32_t primcount;
#endif
  uint32_t flags;
  int16_t link[RF_EDGE_COUNT];
} rfSector16;

#define RF_SECTOR16_TRILIST(p) ((int16_t *)RF_ADDRESS(p,sizeof(rfSector16)))
#define RF_SECTOR16_TRI(p,t) (RF_SECTOR16_TRILIST(p)[t])


typedef struct
{
  /* 52||56 bytes */
  rff edge[RF_EDGE_COUNT];
#if !RF_CONFIG_PACK_MEMORY
  uint32_t primcount;
#endif
  uint32_t flags;
  int32_t link[RF_EDGE_COUNT];
} rfSector32;

#define RF_SECTOR32_TRILIST(p) ((int32_t *)RF_ADDRESS(p,sizeof(rfSector32)))
#define RF_SECTOR32_TRI(p,t) (RF_SECTOR32_TRILIST(p)[t])


typedef struct
{
  /* 76||80 bytes */
  rff edge[RF_EDGE_COUNT];
#if !RF_CONFIG_PACK_MEMORY
  uint32_t primcount;
#endif
  uint32_t flags;
  int64_t link[RF_EDGE_COUNT];
} rfSector64;

#define RF_SECTOR64_TRILIST(p) ((int64_t *)RF_ADDRESS(p,sizeof(rfSector64)))
#define RF_SECTOR64_TRI(p,t) (RF_SECTOR64_TRILIST(p)[t])

#if RF_CONFIG_PACK_MEMORY
 #define RF_SECTOR_LINKFLAGS_SHIFT (16)
 #define RF_SECTOR_GET_PRIMCOUNT(s) ((s)->flags&((1<<RF_SECTOR_LINKFLAGS_SHIFT)-1))
 #define RF_SECTOR_SET_FLAGSPRIMCOUNT(s,f,p) ((s)->flags=(f)|(p))
#else
 #define RF_SECTOR_LINKFLAGS_SHIFT (0)
 #define RF_SECTOR_GET_PRIMCOUNT(s) ((s)->primcount)
 #define RF_SECTOR_SET_FLAGSPRIMCOUNT(s,f,p) (s)->flags=(f);(s)->primcount=(p)
#endif


#define RF_NODE_CHILD_COUNT (2)

typedef struct
{
  /* 12 bytes */
  uint32_t flags;
  rff plane;
  int16_t link[RF_NODE_CHILD_COUNT];
} rfNode16;

typedef struct
{
  /* 16 bytes */
  uint32_t flags;
  rff plane;
  int32_t link[RF_NODE_CHILD_COUNT];
} rfNode32;

typedef struct
{
  /* 24 bytes */
  uint32_t flags;
  rff plane;
  int64_t link[RF_NODE_CHILD_COUNT];
} rfNode64;

#define RF_NODE_AXIS_SHIFT (0)
#define RF_NODE_AXIS_MASK (0x3)
#define RF_NODE_LINKFLAGS_SHIFT (2)
#define RF_NODE_LINKFLAGS_LESS_SHIFT (RF_NODE_LINKFLAGS_SHIFT+RF_NODE_LESS)
#define RF_NODE_LINKFLAGS_MORE_SHIFT (RF_NODE_LINKFLAGS_SHIFT+RF_NODE_MORE)
#define RF_NODE_AXISMASK_SHIFT (4)
#define RF_NODE_DUPLINKFLAGS_SHIFT (RF_NODE_AXISMASK_SHIFT+4)
#define RF_NODE_DUPLINKFLAGS_LESS_SHIFT (RF_NODE_DUPLINKFLAGS_SHIFT+RF_NODE_LESS)
#define RF_NODE_DUPLINKFLAGS_MORE_SHIFT (RF_NODE_DUPLINKFLAGS_SHIFT+RF_NODE_MORE)

#define RF_NODE_GET_AXIS(f) (((f)>>RF_NODE_AXIS_SHIFT)&RF_NODE_AXIS_MASK)
#define RF_NODE_SET_AXIS(f) (((f)<<RF_NODE_AXIS_SHIFT)|((1<<RF_NODE_AXISMASK_SHIFT)<<(f)))

#define RF_NODE_TEST_AXIS_X(f) (!((f)&(3<<RF_NODE_AXIS_SHIFT)))
#define RF_NODE_TEST_AXIS_Y(f) (((f)&(1<<RF_NODE_AXIS_SHIFT)))
#define RF_NODE_TEST_AXIS_Z(f) (((f)&(2<<RF_NODE_AXIS_SHIFT)))


/**/


#define RF_NODE16_SIZE RF_SIZE_ROUND16(sizeof(rfNode16))
#define RF_NODE32_SIZE RF_SIZE_ROUND16(sizeof(rfNode32))
#define RF_NODE64_SIZE RF_SIZE_ROUND16(sizeof(rfNode64))


/**/


typedef struct
{
  int32_t cullmode;
  int32_t datasize;
} rfObjectHeader;

#define RF_OBJECTHEADER_SIZE RF_SIZE_ROUND64(sizeof(rfObjectHeader))

#define RF_OBJECT_DATA(x) (RF_ADDRESS(x,RF_OBJECTHEADER_SIZE))


/**/


#define RF_GRAPH_HEADER_IDENTIFIER_LENGTH (16)

typedef struct
{
  char identifier[RF_GRAPH_HEADER_IDENTIFIER_LENGTH];
  uint32_t graphtype;
  uint32_t cacheversion;
  uint32_t rfversion;
  uint32_t headersize;
  uint32_t sectoraddrbits;
  uint32_t nodeaddrbits;
  uint32_t trirefaddrbits;
  uint32_t addrshift;
  uint32_t alignment;
  uint32_t primdatasize;
  uint32_t rffwidth;
  uint32_t graphflags;
  uint32_t sectorprimitivemax;

  float edge[6];
  float mediansizef;
  double mediansized;

  uint64_t sectorcount;
  uint64_t nodecount;
  uint64_t primitivecount;
  uint64_t graphdatasize;
  uint64_t modeldataoffset;
  uint64_t modeldatasize;

} rfGraphHeader;

#define RF_GRAPHHEADER_SIZE RF_SIZE_ROUND64(sizeof(rfGraphHeader))

enum
{
  RF_GRAPHEADER_TYPE_TRIANGLES,
};

typedef struct
{
  rff graphedge[RF_EDGE_COUNT];
  rff edge[RF_EDGE_COUNT];
  rff spacing[RF_AXIS_COUNT];
  rff spacinginv[RF_AXIS_COUNT];
  uint64_t width;
  uint64_t max;
  uint64_t totalcount;
  uint32_t factory, factorz;
} rfOriginTable;

#define RF_ORIGINTABLE_LIST(x) ((rforigin *)(RF_ADDRESS(x,sizeof(rfOriginTable))))

#define RF_GRAPH_LINK16_SHIFT (4)
#define RF_GRAPH_LINK32_SHIFT (4)
#define RF_GRAPH_LINK64_SHIFT (0)

#define RF_GRAPH_ROUND_SIZE(x) ((x+0xfff)&(~0xfff))


/**/


struct _rfHandle
{
  int32_t handletype;
  void *objectgraph;
  void *objectdata;
  void *scenegraph;
  void *rfscene;
  int32_t addressmode;
  int32_t cullmode;
};

enum
{
  RF_HANDLETYPE_OBJECT,
  RF_HANDLETYPE_SCENE
};


/**/


typedef struct
{
  int32_t objectcount;
  int32_t rootoffset;
  rff diagonalsize;
} rfSceneHeader;

#define RF_SCENEHEADER_SIZE RF_SIZE_ROUND64(sizeof(rfSceneHeader))

#define RF_SCENEHEADER_OBJECTTABLE(x) ((rfSceneObjectEntry *)(RF_ADDRESS(x,RF_SCENEHEADER_SIZE)))

typedef struct
{
  rff edge[RF_EDGE_COUNT];
  rff matrix[16];
  rff matrixinv[16];
  /* TODO: Store some flag for identity matrix? */
  int32_t hitsideskip;
  void *objectgraph;
  void *objectdata;
} rfSceneObjectEntry;

typedef struct
{
  /* Offsets to parent and children */
  int32_t parent;
  int32_t objectcount;

  int32_t childleft;
  int32_t childright;
  rff leftedge[RF_EDGE_COUNT];
  rff rightedge[RF_EDGE_COUNT];
} rfRegion;

#define RF_REGION_SIZE RF_SIZE_ROUND16(offsetof(rfRegion,childleft))
#define RF_REGIONLEAF_SIZE (offsetof(rfRegion,childleft))
#define RF_REGIONLEAF_OBJECTLIST(x) ((int32_t *)(RF_ADDRESS(x,offsetof(rfRegion,childleft))))


/**/


#endif /* RFGRAPH_H */


