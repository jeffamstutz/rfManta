// This program is designed to test the FrustumKDTree code piece by
// piece, so that we can have some confidence as to the correctness of
// the code without having to run some giant model.
//
// Author: James Bigler
// Date: Sep. 14, 2005
//


#include <Model/Groups/FrustumKDTree.h>
#include <Interface/Context.h>

#include <iostream>

using namespace Manta;
using namespace Kdtree;
using namespace std;

// These are the two main functions we will be testing
#if 0
// Intersect the specified frustum with an internal node.
IntersectCase frustum_node_intersect(
                                     const KDTreeNode *node,
                                     const PacketFrustum &frustum,
                                     const BBox &node_bounds );

// Intersect the specified frustum with a leaf.
bool frustum_leaf_intersect(const RenderContext& context,
                            PacketFrustum &frustum,
                            const BBox& leaf_bounds );
#endif

int main(int argc, char* argv[]) {

  if (argc > 1) {
    // print help message
    cerr << "Usage: "<<argv[0]<<" no args\n";
    return 1;
  }

  // Here's the bounding box for the node
  Vector bbox_min(-10,-5,-4), bbox_max(-2,7,12);
  BBox bbox(bbox_min, bbox_max);
  // OK, create my first KDTreeNode
  KDTreeNode node;
  node.internal.split = 2; // split position
  node.internal.flags = 2; // z axis
  node.internal.flags |= KDNODE_INTERNAL_MASK; // This is an internal node
  // Now to create a RenderContext, but we don't actually use it for
  // the frustum intersect functions, so fill it as much as you need to.
  RenderContext render_context(NULL,
                               0, 0, 1,
                               NULL,
                               NULL, NULL,
                               NULL, NULL,
                               NULL,NULL,NULL );
  // Ah, the ray packet
  Vector ray_origin(20, 0, 20);
  Vector Pn(19,1,2), Pf(2,1,2);
  cout << "Pn-ray_origin = "<<Pn-ray_origin<<"\n";
  cout << "Pf-ray_origin = "<<Pf-ray_origin<<"\n";

  // Now for the PacketFrustum
  PacketFrustum pfrustum(ray_origin,
                         Pn-ray_origin, Pn-ray_origin,
                         Pf-ray_origin, Pf-ray_origin,
                         1);

  // Now to try a test
  FrustumKDTree kdtree(NULL);
  kdtree.debug_level = 1;

  FrustumKDTree::IntersectCase result =
    kdtree.frustum_node_intersect(/*render_context,*/ &node, pfrustum, bbox);
  switch(result) {
  case FrustumKDTree::INTERSECT_NONE:
    cout << "INTERSECT_NONE\n";
    break;
  case FrustumKDTree::INTERSECT_MIN:
    cout << "INTERSECT_MIN\n";
    break;
  case FrustumKDTree::INTERSECT_MAX:
    cout << "INTERSECT_MAX\n";
    break;
  case FrustumKDTree::INTERSECT_BOTH:
    cout << "INTERSECT_BOTH\n";
    break;
  }


  return 0;
}


