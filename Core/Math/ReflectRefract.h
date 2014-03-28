#include <MantaTypes.h>
#include <Core/Math/MinMax.h>
#include <Core/Geometry/Vector.h>

namespace Manta {
  // Given eta         = eta_t/eta_i (or eta2/eta1)
  //       eta_inverse = eta_i/eta_t (or eta1/eta2)
  // r0 = (1 - eta_inverse) / (1 + eta_inverse);
  // r0 = (eta - 1)         / (1 + eta);
  // r0 = (eta_t - eta_i)   / (eta_t + eta_i);
  //
  // Note that because r0 is squared, eta_t and eta_i can be swapped and
  // get the same result.
  //
  // (eta_t - eta_i)^2 = eta_t^2 - 2*eta_t*eta_i + eta_i^2
  // (eta_i - eta_t)^2 = eta_i^2 - 2*eta_i*eta_t + eta_t^2
  //
  // We can conclude that the reflection coefficient is not dependent on
  // wether the ray is entering or leaving the dielectric, but rather
  // the relative angles.

  inline Real SchlickReflection(Real costheta, Real costheta2,
                                Real eta_inverse)
  {
    Real k = 1 - Min(costheta, costheta2);
    Real k5 = (k*k)*(k*k)*k;
    Real r0 = (1 - eta_inverse) / (1 + eta_inverse);
    r0 *= r0;
    return r0*(1-k5) + k5;
  }

  // Similarly to the Schlick approximation to the reflection, eta1 and
  // eta2 can be swapped and get the same result.
  inline Real FresnelReflection(Real costheta, Real costheta2,
                                Real eta1, Real eta2)
  {
    Real r_parallel = (((eta2 * costheta) - (eta1 * costheta2)) /
                       ((eta2 * costheta) + (eta1 * costheta2)));
    Real r_perp     = (((eta1 * costheta) - (eta2 * costheta2)) /
                       ((eta1 * costheta) + (eta2 * costheta2)));
    return (Real)(.5)*(r_parallel*r_parallel + r_perp*r_perp);
  }

  inline Vector ReflectRay(const Vector& v, const Vector& normal,
                           Real costheta)
  {
    return v + 2*costheta*normal;
  }

  inline Vector RefractRay(const Vector& v, const Vector& normal,
                           Real eta_inverse, Real costheta, Real costheta2)
  {
    return (eta_inverse * v +
            (eta_inverse * costheta - costheta2) * normal);
  }

} // end namespace Manta
