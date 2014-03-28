#include <Model/Groups/TreeTraversalProbability.h>

void Manta::TreeTraversalProb::computeProbs(const BBox& parent, const BBox& bL,
                                            const BBox& bR, float& lr, float& jl,
                                            float& jr) const {
  const Vector diagL = bL.diagonal();
  const bool bLIsInfinitesimal[3] = { diagL[1] == 0 || diagL[2] == 0,
                                      diagL[0] == 0 || diagL[2] == 0,
                                      diagL[0] == 0 || diagL[1] == 0 };
  const Vector diagR = bR.diagonal();
  const bool bRIsInfinitesimal[3] = { diagR[1] == 0 || diagR[2] == 0,
                                      diagR[0] == 0 || diagR[2] == 0,
                                      diagR[0] == 0 || diagR[1] == 0 };
  lr = 0;

  // Test all patch combinations
  // +x, +y, +z, -x, -y, and -z are the 6 face normals.
  for (int i=0; i < 6; ++i) {

    const int ii=i%3;

    // infinitesimal face cannot emit any energy
    if (bLIsInfinitesimal[ii]) continue;

    // Check to see if face i has anything it can pair with.
    if (i < 3) {  // doing positive direction normals
      if (bL[1][ii] >= bR[1][ii])
        continue;
    }
    else { // doing negative direction normals
      if (bL[0][ii] <= bR[0][ii])
        continue;
    }

    for (int j=0; j < 6; ++j) {
      if (i == j) continue;

      const int jj = j%3;

      if (bRIsInfinitesimal[jj]) continue;

      BBox patchL = bL;
      if (i < 3)
        patchL[0][ii] = patchL[1][ii]; // flatten bbox to a patch.
      else
        patchL[1][ii] = patchL[0][ii]; // flatten bbox to a patch.

      BBox patchR = bR;
      if (j < 3)
        patchR[0][jj] = patchR[1][jj]; // flatten bbox to a patch.
      else
        patchR[1][jj] = patchR[0][jj]; // flatten bbox to a patch.

      if (ii == jj) { // have opposing faces (i!=j from above)
        if (i<3) {
          if (patchL[1][ii] >= patchR[0][ii]) continue; // boxes can't overlap
          lr += computeParF(patchL, patchR, ii);
        }
        else {
          if (patchL[0][ii] <= patchR[1][ii]) continue; // boxes can't overlap
          lr += computeParF(patchL, patchR, ii);
        }
      }
      else { // now checking for perpendicular faces
        if (j < 3) {
          if (patchL[1][jj] <= patchR[1][jj]) continue;
          patchL[0][jj] = Max(patchL[0][jj], patchR[1][jj]);
        }
        else {
          if (patchL[0][jj] >= patchR[0][jj]) continue;
          patchL[1][jj] = Min(patchL[1][jj], patchR[0][jj]);
        }

        if (i < 3) { //check perp faces
          patchR[0][ii] = Max(patchR[0][ii], patchL[1][ii]);
          lr += computePerpF(patchL, patchR, ii, jj);
        }
        else {
          patchR[1][ii] = Min(patchR[1][ii], patchL[0][ii]);
          lr += computePerpF(patchL, patchR, ii, jj);
        }
      }

    }
  }

  const float parentArea_inv = 1.0f / parent.computeArea();

  // find prob for overlap section
  lr += overlap(bL, bR);

  lr *= parentArea_inv;
  jl = bL.computeArea() * parentArea_inv - lr;
  jr = bR.computeArea() * parentArea_inv - lr;
}
