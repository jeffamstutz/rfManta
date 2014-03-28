#ifndef _MANTA_TREE_TRAVERSAL_PROBABILITY_H_
#define _MANTA_TREE_TRAVERSAL_PROBABILITY_H_

#include <Core/Geometry/BBox.h>
#include <Core/Math/Trig.h>
#include <Core/Math/ExponSSE.h>

#ifndef M_1_PI // 1.0/PI
#define M_1_PI 0.318309886183790671538
#endif

namespace Manta {
  class TreeTraversalProb {
  public:

    // Returns probability of a randomly distributed ray that intersects the
    // parent box also intersects both boxes bL and bR (lr), just bL (jl) and
    // just bR (jr).  The probability of it hitting bL is (lr+jl) and bR is
    // (lr+jr). bL and bR must be fully contained in the parent bounds.
    void computeProbs(const BBox& parent, const BBox& bL, const BBox& bR,
                      float& lr, float& jl, float& jr) const;

  private:

    ICSI_log fastLog;

    // We have to use a modified log function that returns a number for log(0)
    // instead of -inf.  Since log(0) ends up being multiplied by a 0, it will
    // become 0 as long as log(0) is not -inf.  The alternative is to
    // specifically handle those cases with other formulas which is a pain.

    // Using the approximate versions of log and atan result in an overall 3X
    // speedup for computing the RTSAH.

    //#define ttp_Log Logf
    #define ttp_Log(x) (fastLog.log(x))
    //#define ttp_Log(x) (x==0.f ? -100000.f : logf(x));

    #define ttp_Atan2 fast_atan2f
    //#define ttp_Atan2 atan2f


    float alternatingSign(int i) const {
      return i%2 == 1 ? -1.0f : 1.0f;
    }

    float computePerpF_G(float v, float z, float x, float y, float z0, float x1) const {
      const float a = y-v;
      const float b = z0-z;
      const float c = x-x1;

      const float aa = a*a;
      const float cc = c*c;
      const float bb = b*b;

      const float sqrt_cc_bb = Sqrt(cc+bb);
      return a*sqrt_cc_bb*ttp_Atan2(a, sqrt_cc_bb) + .25f*(aa-bb-cc)*ttp_Log(aa+bb+cc);
    }

    int otherIndex(unsigned int index1, unsigned int index2) const {
      // If we have a Vector that can be indexed by element 0, 1, and 2, and we
      // have two unique indices and want to find out what the third index is,
      // then this will return it.  For instance, given 0, 1 we get 2.  0, 2
      // gives 1. 1, 2 gives 0.
      return (~(index1+index2)) & 3;
    }

    float computePerpF(const BBox& b0, const BBox& b1,
                       const int flatDim1, const int flatDim2) const {
      const int axis = otherIndex(flatDim1, flatDim2); // perpendicular about this axis

      const float v[2] = {b1[0][axis], b1[1][axis]};
      const float y[2] = {b0[0][axis], b0[1][axis]};

      const float x[2] = {b0[0][flatDim2], b0[1][flatDim2]};
      const float z[2] = {b1[0][flatDim1], b1[1][flatDim1]};

      const float x1 = b1[0][flatDim2];
      const float z0 = b0[0][flatDim1];

      float F01 = 0;
      for (int i=1; i <= 2; ++i)
        for (int j=1; j <= 2; ++j)
          for (int k=1; k <= 2; ++k)
            for (int m=1; m <= 2; ++m)
              F01 += computePerpF_G(v[i-1], z[j-1], x[k-1], y[m-1], z0, x1) *
                alternatingSign(i+j+k+m);
      F01 *= static_cast<float>(M_1_PI);
      return Abs(F01); // Gives negative answer if b1 is behind b0.
    }

    float computeParF_G(float u, float v, float x, float y, float z_inv) const {
      const float a = (x-u)*z_inv;
      const float b = (y-v)*z_inv;

      const float aa = a*a;
      const float bb = b*b;

      const float sqrt_1_aa = Sqrt(1+aa);
      const float sqrt_1_bb = Sqrt(1+bb);

      return b*sqrt_1_aa*ttp_Atan2(b, sqrt_1_aa) +
        a*sqrt_1_bb*ttp_Atan2(a, sqrt_1_bb) - 0.5f*ttp_Log(1 + aa + bb);
    }

    // returns Form factor from rectangle b0 to parallel rectangle b1, multiplied
    //  by area of b0 (we don't divide by b0.computeArea() as an optimization).
    float computeParF(const BBox& b0, const BBox& b1, const int flatDim) const {
      const int dim1 = (flatDim+1)%3;
      const int dim2 = (flatDim+2)%3;

      const float y[2] = {b0[0][dim1], b0[1][dim1]};
      const float v[2] = {b1[0][dim1], b1[1][dim1]};

      const float x[2] = {b0[0][dim2], b0[1][dim2]};
      const float u[2] = {b1[0][dim2], b1[1][dim2]};

      const float z = b0[0][flatDim] - b1[0][flatDim];
      const float z_inv = 1.0f / z;

      float F01 = 0;
      for (int i=1; i <= 2; ++i)
        for (int j=1; j <= 2; ++j)
          for (int k=1; k <= 2; ++k)
            for (int m=1; m <= 2; ++m)
              F01 += computeParF_G(u[i-1], v[j-1], x[k-1], y[m-1], z_inv) *
                alternatingSign(i+j+k+m);
      F01 *= z*z * static_cast<float>(M_1_PI); // later on we multiply by area
                                               // of b0, so we remove this
                                               // redundant divide by area..
      return Abs(F01); // Gives negative answer if b1 is behind b0.
    }

    float overlap(const BBox& b0, const BBox& b1) const{
      BBox overlapBBox = b0;
      overlapBBox.intersection(b1);
      const Vector diag = overlapBBox.diagonal();
      if (diag[0] < 0 || diag[1] < 0 || diag[2] < 0)
        return 0;
      else
        return overlapBBox.computeArea();
    }
  };
};

#endif

