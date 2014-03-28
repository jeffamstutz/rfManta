#include <Model/Groups/BSP/BSH.h>
#include <Model/Primitives/MeshTriangle.h>
#include <assert.h>
#include <cstdlib>
using namespace Manta;

//initializing static data members
vector<Vector> BSH::triCentroid;
vector<int> BSH::tmpTriID1, BSH::tmpTriID2;
vector<BSH::ClippedTriangle> BSH::tmpCTri;

void BSH::build(Mesh *mesh, const BBox &bounds)
{
  this->mesh = mesh;
  nodes.resize(mesh->size()*2);

  tmpTriID1.resize(mesh->size());
  tmpTriID2.resize(mesh->size());

  triCentroid.resize(mesh->size());
  for (size_t i=0; i < mesh->size(); ++i) {
    tmpTriID1[i] = i;
    triCentroid[i] = mesh->getBBox(i).center();
  }

  int nextFree = 1;
  nodes[0].parent = -1;
  //spatialMedianBuild(0, nextFree, 0, mesh->size(), bounds);
  localMedianBuild(0, nextFree, 0, mesh->size());

  triID.resize(mesh->size());
  for (size_t i=0; i < mesh->size(); ++i) {
    const MeshTriangle *triangle = mesh->get(tmpTriID1[i]);
    triID[i] = ClippedTriangle(tmpTriID1[i],
                               triangle->getVertex(0),
                               triangle->getVertex(1),
                               triangle->getVertex(2));
  }

  refit(0);

  tmpTriID1.clear();
  tmpTriID2.clear();
  triCentroid.clear();
}

void BSH::buildSubset(Mesh *mesh, const vector<int> &subsetObjects,
                      const BBox &bounds)
{
  this->mesh = mesh;
  nodes.resize(subsetObjects.size()*2);
  tmpTriID1.resize(subsetObjects.size());
  tmpTriID2.resize(subsetObjects.size());

  //TODO: try and build bvh over subsetObjects.size() things, not the
  //entire mesh.
  triCentroid.resize(mesh->size());
  for (size_t i=0; i < subsetObjects.size(); ++i) {
    tmpTriID1[i] = subsetObjects[i];
    triCentroid[subsetObjects[i]] = mesh->getBBox(subsetObjects[i]).center();
  }

  int nextFree = 1;
  nodes[0].parent = -1;
  //spatialMedianBuild(0, nextFree, 0, subsetObjects.size(), bounds);
  localMedianBuild(0, nextFree, 0, subsetObjects.size());

  triID.resize(subsetObjects.size());
  for (size_t i=0; i < subsetObjects.size(); ++i) {
    const MeshTriangle *triangle = mesh->get(tmpTriID1[i]);
    triID[i] = ClippedTriangle(tmpTriID1[i],
                               triangle->getVertex(0),
                               triangle->getVertex(1),
                               triangle->getVertex(2));
  }

  refit(0);

  tmpTriID1.clear();
  tmpTriID2.clear();
  triCentroid.clear();

}

void BSH::localMedianBuild(int nodeID, int &nextFree,
                           const size_t begin, const size_t end)
{
  BBox box;
  for (size_t i=begin; i < end; ++i)
    box.extendByPoint(triCentroid[tmpTriID1[i]]);

  if (end - begin < 5 || box[0] == box[1]) {
    nodes[nodeID].makeLeaf(begin, end);
    return;
  }

  const int dim = box.longestAxis();
  const Real split = 0.5f*(box[1][dim] + box[0][dim]);

  for (size_t i=begin; i < end; ++i)
    tmpTriID2[i] = tmpTriID1[i];
  int *l = &tmpTriID1[begin];
  int *r = &tmpTriID1[end - 1];

  for (size_t i=begin;i<end;i++) {
    int t = tmpTriID2[i];
    if (triCentroid[t][dim] >= split)
      *r-- = t;
    else
      *l++ = t;
  }
  int bestSplit = l - &tmpTriID1[begin];

  if (bestSplit == 0) {
    nodes[nodeID].makeLeaf(begin, end);
    return;
  }
  if (begin + bestSplit == end) {
    nodes[nodeID].makeLeaf(begin, end);
    return;
  }

  nodes[nodeID].makeInternal(nextFree);
  nodes[nextFree  ].parent = nodeID;
  nodes[nextFree+1].parent = nodeID;

  nextFree+=2;

  localMedianBuild(nodes[nodeID].children, nextFree,
                   begin, begin+bestSplit);
  localMedianBuild(nodes[nodeID].children+1, nextFree,
                   begin+bestSplit, end);
}

void BSH::spatialMedianBuild(int nodeID, int &nextFree,
                             const size_t begin, const size_t end,
                             const BBox &bounds)
{
  //TODO: try a build that might be slower but produces something more
  //suited to our needs. local median might be better quality, but
  //slower to build.

  BBox box;
  for (size_t i=begin; i < end; ++i)
    box.extendByPoint(triCentroid[tmpTriID1[i]]);

  if (end - begin < 5 || box[0] == box[1]) {
    nodes[nodeID].makeLeaf(begin, end);
    return;
  }

  const int dim = bounds.longestAxis();
  const Real split = 0.5f*(bounds[1][dim] + bounds[0][dim]);

  for (size_t i=begin; i < end; ++i)
    tmpTriID2[i] = tmpTriID1[i];
  int *l = &tmpTriID1[begin];
  int *r = &tmpTriID1[end - 1];

  for (size_t i=begin;i<end;i++) {
    int t = tmpTriID2[i];
    if (triCentroid[t][dim] >= split)
      *r-- = t;
    else
      *l++ = t;
  }

  BBox lBounds = bounds;
  BBox rBounds = bounds;
  lBounds[1][dim] = rBounds[0][dim] = split;

  int bestSplit = l - &tmpTriID1[begin];

  if (bestSplit == 0) {
    spatialMedianBuild(nodeID, nextFree, begin, end, rBounds);
    return;
  }
  if (begin + bestSplit == end) {
    spatialMedianBuild(nodeID, nextFree, begin, end, lBounds);
    return;
  }

  nodes[nodeID].makeInternal(nextFree);
  nodes[nextFree  ].parent = nodeID;
  nodes[nextFree+1].parent = nodeID;

  nextFree+=2;

  spatialMedianBuild(nodes[nodeID].children, nextFree,
                     begin, begin+bestSplit, lBounds);
  spatialMedianBuild(nodes[nodeID].children+1, nextFree,
                     begin+bestSplit, end, rBounds);
}

void BSH::refit(size_t nodeID)
{
  BSHNode &node = nodes[nodeID];
  if (node.isLeaf()) {
    for (int i=0; i < node.size; ++i) {
      const int tri = triID[node.objectsIndex+i].originalTriID;
      node.bounds.extendByBox(mesh->getBBox(tri));
    }
  }
  else {
    refit(node.children);
    refit(node.children+1);

    node.size = nodes[node.children].size + nodes[node.children+1].size;

    if (nodes[node.children].size > 0 && nodes[node.children+1].size == 0) {
      int parent = node.parent;
      node = nodes[node.children];
      node.parent = parent;
    }
    else if (nodes[node.children].size == 0 && nodes[node.children+1].size > 0) {
      int parent = node.parent;
      node = nodes[node.children+1];
      node.parent = parent;
    }

    node.bounds.extendByBox(nodes[node.children].bounds);
    node.bounds.extendByBox(nodes[node.children+1].bounds);
  }

  //compute a loose bound.
  //TODO: compute a tighter bound
  node.center = node.bounds.center();
  node.radius = 0.5f * node.bounds.diagonal().length();
}


void BSH::splitTriangle(const ClippedTriangle &tri,
                        const BuildSplitPlane &plane,
                        ClippedTriangle negSide[2],
                        ClippedTriangle posSide[2])
{
  //This is the Reentrant Polygon Clipping algorithm by Sutherland
  //and Hodgman.

  int negCount=0;
  int posCount=0;

  Point s, f;
  for (size_t i=0; i < 3; ++i) {
    const Point &p = tri.tri[i];

    if (p != tri.tri[i]) {
      cout << p<<endl<<tri.tri[i]<<endl;
      exit(1);
    }

    double p_d = signedDistance(plane, p);
    if (i==0) { //first point?
      s = f = p;
    }
    else {
      double s_d = signedDistance(plane, s);

      if (s_d*p_d < 0) {  //does line sp cross dividing plane?
        //compute intersection I, of sp and plane.
        const double length1 = fabs(p_d);
        const double length2 = fabs(s_d);

        const double a = length1 / (length1 + length2);

        //We do the a>.5 test for numerical stability.
        Point I;
        if (a > .5)
          I = p + a*(s-p);
        else
          I = s + (1-a)*(p-s);

        if(plane.normal[0]==1)
          I[0]=-plane.d;
        else if(plane.normal[1]==1)
          I[1]=-plane.d;
        else if(plane.normal[2]==1)
          I[2]=-plane.d;

        assert(negCount < 3);
        assert(posCount < 3);
        negSide[0].tri[negCount++] = I;
        posSide[0].tri[posCount++] = I;
      }
    }

    //How does p relate to plane?
    if (p_d == 0) { //on the plane
      assert(negCount < 3);
      assert(posCount < 3);
      negSide[0].tri[negCount++] = p;
      posSide[0].tri[posCount++] = p;
    }
    else if (p_d < 0)
      if (negCount == 3) {
        negSide[1].tri[0] = negSide[0].tri[0];
        negSide[1].tri[1] = negSide[0].tri[2];
        negSide[1].tri[2] = p;
        ++negCount;
      }
      else
        negSide[0].tri[negCount++] = p;
    else if (p_d > 0) {
      if (posCount == 3) {
        posSide[1].tri[0] = posSide[0].tri[0];
        posSide[1].tri[1] = posSide[0].tri[2];
        posSide[1].tri[2] = p;
        ++posCount;
      }
      else
        posSide[0].tri[posCount++] = p;
    }
    s=p;
  }

  //now we need to close the two polygons.
  //TODO: once we know it works, do this closing as part of the for
  //loop.
  const Point &p = f;
  double p_d = signedDistance(plane, p);
  double s_d = signedDistance(plane, s);

  if (s_d*p_d < 0) {  //does line sp cross dividing plane?
    //compute intersection I, of sp and plane.
    const double length1 = fabs(p_d);
    const double length2 = fabs(s_d);

    const double a = length1 / (length1 + length2);
    Point I;
    if (a > .5)
      I = p + a*(s-p);
    else
      I = s + (1-a)*(p-s);

        if(plane.normal[0]==1)
          I[0]=-plane.d;
        else if(plane.normal[1]==1)
          I[1]=-plane.d;
        else if(plane.normal[2]==1)
          I[2]=-plane.d;

    if (negCount == 3) {
      negSide[1].tri[0] = negSide[0].tri[0];
      negSide[1].tri[1] = negSide[0].tri[2];
      negSide[1].tri[2] = I;
      ++negCount;
    }
    else
      negSide[0].tri[negCount++] = I;

    if (posCount == 3) {
      posSide[1].tri[0] = posSide[0].tri[0];
      posSide[1].tri[1] = posSide[0].tri[2];
      posSide[1].tri[2] = I;
      ++posCount;
    }
    else
      posSide[0].tri[posCount++] = I;
  }

  assert(negCount > 0 && negCount < 5);
  assert(posCount > 0 && posCount < 5);

  if (negCount < 4)
    negSide[1].originalTriID = -1;
  if (posCount < 4)
    posSide[1].originalTriID = -1;
}


void BSH::split(const BuildSplitPlane &plane, BSH &positiveBSH,
                const Location coplanar_primitiveSide)
{
  size_t oldSize = size();
  tmpCTri.clear();
  positiveBSH.triID.clear();

  split(plane, positiveBSH, coplanar_primitiveSide, 0);

  triID = tmpCTri;

  if (oldSize > 1) {
    size_t i=0;
    if (size()-numSplits() > 0 && size()-numSplits() < oldSize)
      compact(i);
    i=0;
    if (positiveBSH.size()-positiveBSH.numSplits() > 0 &&
        positiveBSH.size()-positiveBSH.numSplits() < oldSize)
      positiveBSH.compact(i);
  }
}

void BSH::nodeTriIDCopy(size_t nodeID, const vector<ClippedTriangle> &triID,
                        vector<ClippedTriangle> &tmpCTri)
{
  BSHNode &node = nodes[nodeID];
  if (node.isLeaf()) {
    tmpCTri.insert(tmpCTri.end(), triID.begin()+node.objectsIndex,
                   triID.begin()+node.objectsIndex+node.size);
    node.objectsIndex = tmpCTri.size()-node.size;
  }
  else {
    nodeTriIDCopy(node.children, triID, tmpCTri);
    nodeTriIDCopy(node.children+1, triID, tmpCTri);
  }
}

void BSH::split(const BuildSplitPlane &plane, BSH &positiveBSH,
                const Location coplanar_primitiveSide, size_t nodeID)
{
  //assume positiveBSH is a copy of *this.
  //negative is us.

  BSHNode &posNode = positiveBSH.nodes[nodeID];
  BSHNode &negNode = nodes[nodeID];

  Location position = posNode.whichSide(plane);
  if (position == negativeSide) {
    posNode.makeLeaf(0,0);
    posNode.bounds.reset();
    nodeTriIDCopy(nodeID, triID, tmpCTri);
    return;
  }
  else if (position == positiveSide) {
    negNode.makeLeaf(0,0);
    negNode.bounds.reset();
    positiveBSH.nodeTriIDCopy(nodeID, triID, positiveBSH.triID);
    return;
  }
  else if (posNode.isLeaf()) {

    negNode.bounds.reset();
    posNode.bounds.reset();

    unsigned int size = posNode.size;
    posNode.size=0;
    negNode.size=0;

    for (size_t i=0; i < size; ++i) {
      ClippedTriangle &tri = triID[posNode.objectsIndex+i];
      const Location pos = getLocation(tri, plane);
      switch(pos) {
      case negativeSide:
        tmpCTri.push_back(tri);
        negNode.size++;
        break;
      case positiveSide:
        positiveBSH.triID.push_back(tri);
        posNode.size++;
        break;
      case bothSides:
        //need to clip the triangle
        tmpCTri.push_back(tri);
        negNode.size++;

        positiveBSH.triID.push_back(tri);
        posNode.size++;

#if 1 //do triangle clipping
        tmpCTri.push_back(tri);
        positiveBSH.triID.push_back(tri);

        splitTriangle(tri, plane, &tmpCTri[tmpCTri.size()-2],
                      &positiveBSH.triID[positiveBSH.triID.size()-2]);

        if (tmpCTri.back().originalTriID == -1) {
          tmpCTri.pop_back();
        }
        else {
          negNode.size++;
        }
        if (positiveBSH.triID.back().originalTriID == -1) {
          positiveBSH.triID.pop_back();
        }
        else {
          posNode.size++;
        }
#endif
      break;
      case eitherSide:
        if (!tri.fixedPlane && true){ //set to false to not use fixedPlane
          tri.fixedPlane = true;
          tri.plane = plane;
        }
        if (coplanar_primitiveSide==negativeSide) {
          tmpCTri.push_back(tri);
          negNode.size++;
        }
        else {
          positiveBSH.triID.push_back(tri);
          posNode.size++;
        }
        break;
      }
    }

    negNode.objectsIndex = tmpCTri.size() - negNode.size;
    posNode.objectsIndex = positiveBSH.triID.size() - posNode.size;

    //need to find out how many split triangles there are now.  We
    //know that split triangles will always be next to each other.
    negNode.numSplits=0;
    for (int i=0; i < negNode.size; ++i) {
      if (i!=0 &&
          tmpCTri[negNode.objectsIndex+i-1].originalTriID ==
          tmpCTri[negNode.objectsIndex+i].originalTriID)
        negNode.numSplits++;

      negNode.bounds.extendByPoint(tmpCTri[negNode.objectsIndex+i].tri[0]);
      negNode.bounds.extendByPoint(tmpCTri[negNode.objectsIndex+i].tri[1]);
      negNode.bounds.extendByPoint(tmpCTri[negNode.objectsIndex+i].tri[2]);
    }
    posNode.numSplits=0;
    for (int i=0; i < posNode.size; ++i) {
      if (i!=0 &&
          positiveBSH.triID[posNode.objectsIndex+i-1].originalTriID ==
          positiveBSH.triID[posNode.objectsIndex+i].originalTriID)
        posNode.numSplits++;

      posNode.bounds.extendByPoint(positiveBSH.triID[posNode.objectsIndex+i].tri[0]);
      posNode.bounds.extendByPoint(positiveBSH.triID[posNode.objectsIndex+i].tri[1]);
      posNode.bounds.extendByPoint(positiveBSH.triID[posNode.objectsIndex+i].tri[2]);
    }
  }
  else {
    split(plane, positiveBSH, coplanar_primitiveSide, negNode.children);
    split(plane, positiveBSH, coplanar_primitiveSide, negNode.children+1);

    posNode.size = positiveBSH.nodes[posNode.children].size +
                   positiveBSH.nodes[posNode.children+1].size;
    negNode.size = nodes[negNode.children].size +
                   nodes[negNode.children+1].size;
    posNode.numSplits = positiveBSH.nodes[posNode.children].numSplits +
                        positiveBSH.nodes[posNode.children+1].numSplits;
    negNode.numSplits = nodes[negNode.children].numSplits +
                        nodes[negNode.children+1].numSplits;

    //If only left child has items, make left child become child of
    //its grand parent (i.e. remove current node).
    if (nodes[negNode.children].size > 0 && nodes[negNode.children+1].size == 0) {
      int parent = negNode.parent;
      negNode = nodes[negNode.children];
      negNode.parent = parent;
    }
    else if (nodes[negNode.children].size == 0 && nodes[negNode.children+1].size > 0) {
      int parent = negNode.parent;
      negNode = nodes[negNode.children+1];
      negNode.parent = parent;
    }
    else if (negNode.size == 0)
      negNode.makeLeaf(0,0);

    if (positiveBSH.nodes[posNode.children].size > 0 && positiveBSH.nodes[posNode.children+1].size == 0) {
      int parent = posNode.parent;
      posNode = positiveBSH.nodes[posNode.children];
      posNode.parent = parent;
    }
    else if (positiveBSH.nodes[posNode.children].size == 0 && positiveBSH.nodes[posNode.children+1].size > 0) {
      int parent = posNode.parent;
      posNode = positiveBSH.nodes[posNode.children+1];
      posNode.parent = parent;
    }
    else if (posNode.size == 0)
      posNode.makeLeaf(0,0);

    if (!posNode.isLeaf()) {
      posNode.bounds.extendByBox(positiveBSH.nodes[posNode.children].bounds);
      posNode.bounds.extendByBox(positiveBSH.nodes[posNode.children+1].bounds);
      //TODO: compute a tighter bound
      //compute a loose bound.
      posNode.center = posNode.bounds.center();
      posNode.radius = 0.5f * posNode.bounds.diagonal().length();
    }

    if (!negNode.isLeaf()) {
      negNode.bounds.extendByBox(nodes[negNode.children+0].bounds);
      negNode.bounds.extendByBox(nodes[negNode.children+1].bounds);

      negNode.center = negNode.bounds.center();
      negNode.radius = 0.5f * negNode.bounds.diagonal().length();
    }
  }
}

void BSH::compact(size_t &highestSeen, size_t node)
{

  BSHNode &currNode = nodes[node];

  if (highestSeen < node) { //this is only true for left children
    highestSeen = node+1;//+1 for right child.
  }

  if (currNode.isLeaf())
    return;

  if (currNode.children > highestSeen+2) {
    //looks like we have some nodes that went unused.
    nodes[++highestSeen] = nodes[currNode.children];
    nodes[highestSeen].parent = node;
    nodes[++highestSeen] = nodes[currNode.children+1];
    nodes[highestSeen].parent = node;
    currNode.children = highestSeen-1;
  }
  compact(highestSeen, currNode.children);
  compact(highestSeen, currNode.children+1);

  if (node == 0) {
    nodes.resize(highestSeen+1);
  }
}

BSH::Iterator BSH::begin() const
{
  Iterator iter(*this, 0);
  while (!nodes[iter.currNode].isLeaf()) {
    iter.currNode = nodes[iter.currNode].children;
  }
  return iter.next();
}


BSH::Iterator &BSH::Iterator::next()
{
  if (bsh.nodes[currNode].isLeaf()) {
    if (++i < bsh.nodes[currNode].size)
      return *this;
    else if (currNode == 0) {
      i=-1;
      return *this;
    }
  }
  bool retreating = true;
  unsigned int prevNode = currNode;
  currNode = bsh.nodes[currNode].parent;

  while(true) {
    if (!bsh.nodes[currNode].isLeaf()) {
      if (retreating) {
        if (bsh.nodes[currNode].children == prevNode) {
          retreating = false;
          currNode = bsh.nodes[currNode].children+1;
        }
        else if (currNode == 0) {
          i=-1;
          return *this;
        }
        else {
          prevNode = currNode;
          currNode = bsh.nodes[currNode].parent;
        }
      }
      else
        currNode = bsh.nodes[currNode].children; //go left.
    }
    else { //we're at a leaf
      if (bsh.nodes[currNode].size > 0) {
        i = 0;
        return *this;
      }
      else {
        if (currNode == 0) {
          i=-1;
          return *this;
        }
        retreating = true;
        prevNode = currNode;
        currNode = bsh.nodes[currNode].parent;
      }
    }
  }
}

Location BSH::getLocation(const ClippedTriangle &ctri,
                          const BuildSplitPlane &plane) const
{
  if (ctri.plane.normal == plane.normal && ctri.plane.d == plane.d) {
    return eitherSide;
  }

  double p1_dist = signedDistance(plane, ctri.tri[0]);
  double p2_dist = signedDistance(plane, ctri.tri[1]);
  double p3_dist = signedDistance(plane, ctri.tri[2]);

  const double MIN_DIST = 1e-6;
  if ( fabs(p1_dist) < MIN_DIST )
    p1_dist = 0;
  if ( fabs(p2_dist) < MIN_DIST )
    p2_dist = 0;
  if ( fabs(p3_dist) < MIN_DIST )
    p3_dist = 0;

  const double points1_2 = p1_dist * p2_dist;
  const double points1_3 = p1_dist * p3_dist;
  const double points2_3 = p2_dist * p3_dist;

  //Check to see if the triangle intersects the plane. It's ok if
  //the triangle lies on the plane, hence we allow distance to be 0,
  //but need to check all combinations of points since a 0 would
  //mask out the sign of the other point.
  if (points1_2 >= 0 && points1_3 >= 0 && points2_3 >= 0) {
    if (p1_dist > 0 || p2_dist > 0 || p3_dist > 0) {
      //positive distance places tri on right child.
      return positiveSide;
    }
    else if (p1_dist == 0 && p2_dist == 0 && p3_dist == 0) {
      //all zero distance places tri on split, so it can go to either side// on left child.
      return eitherSide;
    }
    else {
      //we have negative (and maybe zero) distances.
      return negativeSide;
    }
  }
  else {
    //need to place triangle in both children.
    return bothSides;
  }
}


void BSH::pointsNearPlaneDist(vector<Point> &points,
                          const BuildSplitPlane &plane,
                          const int nodeID) const
{
  const BSHNode &node = nodes[nodeID];

  const double MAX_DIST = 2.5e-6;

  bool nearPlane = node.nearPlaneDist(plane, MAX_DIST);
  if (!nearPlane)
    return;

  if (node.isLeaf()) {
    for (int i=0; i < node.size; ++i) {

      const Point &p0 = triID[node.objectsIndex+i].tri[0];
      const Point &p1 = triID[node.objectsIndex+i].tri[1];
      const Point &p2 = triID[node.objectsIndex+i].tri[2];

      double dist[3] = {signedDistance(plane, p0),
                        signedDistance(plane, p1),
                        signedDistance(plane, p2)};

      int pointOnPlaneID=-1;
      for (int k=0; k < 3; ++k)
        if (fabs(dist[k]) <= MAX_DIST) {
          dist[k]=0;
          pointOnPlaneID=k;
        }

      for (int k=0; k<3; ++k) {
        if (pointOnPlaneID>=0) {
          if (dist[k] == 0)
            points.push_back(triID[node.objectsIndex+i].tri[k]);
          else {
            //if one of the points was close enough to be considered
            //on the plane, but this point isn't, it's possible it
            //should be on the plane, but was just slightly too far
            //away. So instead we check to see if the edge formed by
            //this point and the point known to be on the plane is
            //orthogonal to the plane normal, and so must be on the
            //plane.
            const Point &p = triID[node.objectsIndex+i].tri[pointOnPlaneID];
            Point edge = (p - triID[node.objectsIndex+i].tri[k]).normal();
            double costheta = 1 - fabs(Dot(edge, plane.normal));
            if (costheta > 0.999999) {
#ifdef BUILD_DEBUG
              cout <<"angle: " <<costheta<<endl;
              cout <<"got one! it had a distance of: "
                   <<signedDistance(plane, triID[node.objectsIndex+i].tri[k])
                   <<"     " <<dist[k]<<endl
                   <<acos(costheta)<<"  with d of " <<plane.d<< endl
                   <<Dot(plane.normal, triID[node.objectsIndex+i].tri[k])<<endl;
#endif
              points.push_back(triID[node.objectsIndex+i].tri[k]);
              // exit(1);
            }
          }
        }
      }
    }
  }
  else {
    pointsNearPlaneDist(points, plane, node.children);
    pointsNearPlaneDist(points, plane, node.children+1);
  }
}

void BSH::pointNearPlaneAngle(const vector<Point> &points,
                              const BuildSplitPlane &plane,
                              Point &closePoint, double &minCostheta,
                              const int nodeID) const
{
  assert(points.size() == 2);

  const BSHNode &node = nodes[nodeID];

  bool nearPlane = node.nearPlaneAngle(plane, points[0], minCostheta);
  if (!nearPlane)
    return;

  if (node.isLeaf()) {
    const Point edge = points[1]-points[0];
    for (int i=0; i < node.size; ++i) {
      for (int k=0; k<3; ++k) {
        const Point &p = triID[node.objectsIndex+i].tri[k];
        if (p == points[0] || (points.size()>1 && p == points[1]))
          continue;
        //TODO: optimize this by passing in some of these vectors to the function.
        const Point normal2 = Cross(edge, p-points[0]);
        const float normal2_length2 = static_cast<float>(normal2.length2());
        if (normal2_length2 < 1e-12f)
          continue;

        const float inv_length = inverseSqrt(normal2_length2);
        const double costheta = 1-fabs(Dot(normal2, plane.normal)*inv_length);

        if (costheta==minCostheta) {
          if (closePoint == p)
            continue;
          if (normal2_length2 <= Cross(edge, closePoint-points[0]).length2())
            continue;
//           cout <<"still equal! " << costheta<<endl;
        }
        if (costheta <= minCostheta) {
          closePoint = p;
          minCostheta = costheta;
        }
      }
    }
  }
  else {
    pointNearPlaneAngle(points, plane, closePoint,
                        minCostheta, node.children);
    pointNearPlaneAngle(points, plane, closePoint,
                        minCostheta, node.children+1);
  }
}

void BSH::countLocations(const BuildSplitPlane &plane,
                         int &nLeft, int &nRight, int &nEither,
                         const int nodeID) const
{
  const BSHNode &node = nodes[nodeID];

  if (node.isLeaf()) {
    if (node.numSplits > 0) {
      Location location = getLocation(triID[node.objectsIndex],
                                      plane);
      switch (location) {
      case eitherSide: ++nEither;
        break;
      case negativeSide: ++nLeft;
        break;
      case bothSides: ++nLeft;
      case positiveSide: ++nRight;
        break;
      }

      for (int i=1; i < node.size; ++i) {
        if (triID[node.objectsIndex+i-1].originalTriID !=
            triID[node.objectsIndex+i].originalTriID) {
          Location location = getLocation(triID[node.objectsIndex+i],
                                          plane);
          switch (location) {
          case eitherSide: ++nEither;
            break;
          case negativeSide: ++nLeft;
            break;
          case bothSides: ++nLeft;
          case positiveSide: ++nRight;
            break;
          }
        }
      }
    }
    else {
      for (int i=0; i < node.size; ++i) {
        Location location = getLocation(triID[node.objectsIndex+i],
                                        plane);
        switch (location) {
        case eitherSide: ++nEither;
          break;
        case negativeSide: ++nLeft;
          break;
        case bothSides: ++nLeft;
        case positiveSide: ++nRight;
          break;
        }
      }
    }
  }
  else {
    Location location = node.whichSide(plane);

    switch (location) {
    case negativeSide: nLeft+=node.size-node.numSplits;
      break;
    case positiveSide: nRight+=node.size-node.numSplits;
      break;
    default:
      countLocations(plane, nLeft, nRight, nEither, node.children);
      countLocations(plane, nLeft, nRight, nEither, node.children+1);
      break;
    }
  }
}
