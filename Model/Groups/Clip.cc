#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>

namespace Manta {

  bool ClipTriangle(Vector poly[10], unsigned int &polySize, const BBox &bounds)
  {
    // I think allocating space for 7 vertices should be fine, but just in case
    // I'm being stupid, let's make it 10 to be safe.

    // Note: We use arrays instead of std::vector since this results in about a
    // 2x speedup in the build time since std::vector needs to allocate memory
    // on the heap which is very expensive.

    Vector out[10];
    unsigned int outSize=0;

    const Vector a = poly[0];
    const Vector b = poly[1];
    const Vector c = poly[2];

    for (int d=0; d<3; d++) {
      if (bounds[0][d] == bounds[1][d]) {
        // slabs is planar -- we're either in it, or fully out
        if (a[d] == b[d] && a[d] == c[d] && a[d] == bounds[0][d])
          continue; // no need to try clipping; we know it's all inside.
        else {
          // we're fully out.  Nothing to clip.
          return false;
        }
      }
      else
        for (int side = 0; side < 2; side++) {
          for (unsigned int edge = 0;edge < polySize; edge++) {
            unsigned int v0 = edge;
            unsigned int v1 = (edge+1);
            if (v1 >= polySize)
              v1 = v1-polySize; // cheaper than using mod as in: (edge+1)%polySize

            const bool v0in
              = (side==0)
              ? poly[v0][d] >= bounds[0][d]
              : poly[v0][d] <= bounds[1][d];
            const bool v1in
              = (side==0)
              ? poly[v1][d] >= bounds[0][d]
              : poly[v1][d] <= bounds[1][d];

            if (v0in && v1in) {
              // v0 was already added in the last step, then
              out[outSize++] = poly[v1];
            } else if (!v0in && !v1in) {
              // do nothing, both are out
            } else {
              const float f
                = (bounds[side][d] - poly[v0][d])
                / (    poly[v1][d] - poly[v0][d]);
              Vector newVtx;
              newVtx = poly[v0] + f * (poly[v1]-poly[v0]);//(1-f)*poly[v0] + f*poly[v1];
              newVtx[d] = bounds[side][d]; // make sure it's exactly _on_ the plane

              if (v0in) {
                // v0 was already pushed
                if (newVtx != poly[v0])
                  out[outSize++] = newVtx;
              } else {
                if (newVtx != poly[v0] && newVtx != poly[v1])
                  out[outSize++] = newVtx;
                out[outSize++] = poly[v1];
              }
            }
          }
          if (outSize < 3)
            return false;

          for (unsigned int i=0; i < outSize; ++i)
            poly[i] = out[i];

          polySize = outSize;
          outSize = 0;
        }
    }
    return true;
  }

  bool ClipTriangle(BBox &clipped,
                    const Vector &a,const Vector &b,const Vector&c,
                    const BBox &bounds)
  {
    BBox bb;
    bb.reset();
    bb.extendByPoint(a);
    bb.extendByPoint(b);
    bb.extendByPoint(c);

    // do sutherland-hodgeman clip

    clipped.reset();

    Vector poly[10] = {a, b, c};
    unsigned int polySize = 3;

    bool success = ClipTriangle(poly, polySize, bounds);
    if (!success)
      return false;

    for (unsigned int i=0;i<polySize;i++)
      clipped.extendByPoint(poly[i]);


#define CLIP_ACCURACY_FIX
#ifdef CLIP_ACCURACY_FIX
    // accuracy checker: *if* a triangle - in a certain dimension - is
    // very, very narrow (relative to its numerical expressability),
    // make sure the bounding box gets large enough to actually contain
    // the triangle.  Note, if the triangle is flat in that dimension
    // then both the clipped and original bbox will be the same (flat),
    // so no need to bother with which bbox to use in that case.

    for (int i=0;i<3;i++)
      if ((bb[1][i]-bb[0][i]) * float(1<<19) < std::max(fabsf(bb[1][i]),
                                                        fabsf(bb[0][i]))) {
        // This should be good enough since the original bbox is already almost
        // flat (or completely flat), so using it should give almost the same
        // results as the ideal value.
        clipped[0][i] = bb[0][i];
        clipped[1][i] = bb[1][i];
      }
//     clipped.intersection(bounds);
# endif

    return !(clipped[0] == clipped[1]);
  }
};
