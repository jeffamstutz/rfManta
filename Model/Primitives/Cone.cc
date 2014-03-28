
#include <Model/Primitives/Cone.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/Expon.h>

using namespace Manta;
using namespace std;

Cone::Cone(Material* mat, Real radius, Real height)
  : PrimitiveCommon(mat, this), r(radius), h(height)
{
}

Cone::~Cone()
{
}

Cone* Cone::clone(CloneDepth depth, Clonable* incoming)
{
  Cone *copy;
  if (incoming)
    copy = dynamic_cast<Cone*>(incoming);
  else
    copy = new Cone();

  PrimitiveCommon::clone(depth, copy);
  copy->r = r;
  copy->h = h;
  return copy;
}

Interpolable::InterpErr Cone::serialInterpolate(const std::vector<keyframe_t> &keyframes)
{
  PrimitiveCommon::interpolate(keyframes);
  h = 0.0f;
  r = 0.0f;

  for (size_t i=0; i < keyframes.size(); ++i) {
    Cone *cone = dynamic_cast<Cone*>(keyframes[i].keyframe);
    if (cone == NULL)
      return notInterpolable;
    r += cone->r * keyframes[i].t;
    h += cone->h * keyframes[i].t;
  }
  return success;
}


void Cone::computeBounds(const PreprocessContext&, BBox& bbox) const
{
  bbox.extendByBox(BBox(Vector(-r,-r,0), Vector(r,r,h)));
}

void Cone::intersect(const RenderContext&, RayPacket& rays) const
{
  //Equation of cone is x^2+y^2 == (r/h)^2 * (z-h)^2
  //h is height of cone and cone base is located on z=0, xy plane
  //(cone is oriented along z-axis).

  // In[7]:= Expand[x^2+y^2==(r/h)^2 * (z-h)^2]
  //
  //           2     2                             2  2     2  2
  // Out[7]= Ox  + Oy  + 2 Dx Ox t + 2 Dy Oy t + Dx  t  + Dy  t  == 
  // 
  //                 2     2  2         2              2       2  2  2
  //       2   2 Oz r    Oz  r    2 Dz r  t   2 Dz Oz r  t   Dz  r  t
  // >    r  - ------- + ------ - --------- + ------------ + ---------
  //              h         2         h             2            2
  //                       h                       h            h

  //Solve quadratic equation in t.
  //Note that the code below looks different because we've factored
  //terms out, and done other algebraic optimizations.

  const Real r_invh = r/h;
  const Real r_invh2 = r_invh*r_invh;

  for(int i=rays.begin(); i<rays.end(); i++) {
    const Vector O = rays.getOrigin(i);
    const Vector D = rays.getDirection(i);
    const Vector O2 = O*O;
    const Vector D2 = D*D;
    const Vector DO = D*O;

    const Real h_Oz = h-O.z();
    const Real h_Oz_r_invh2 = h_Oz*r_invh2;

    const Real a = D2.x() + D2.y() - D2.z()*r_invh2;
    const Real b = 2*(DO.x() + DO.y() + D.z()*h_Oz_r_invh2);
    const Real c = O2.x() + O2.y() - (h_Oz*h_Oz_r_invh2);

    const Real d2 = b*b-4*a*c;
    if (d2 >= 0) {
      const Real d = Sqrt(d2);
      const Real inv2a = 1/(2*a);
      Real t1 = (-b+d)*inv2a;
      Real t2 = (-b-d)*inv2a;

      if(t1 > t2) {
        swap(t1, t2);
      }

      Real z1 = O.z() + t1*D.z();
      Real z2 = O.z() + t2*D.z();

      if (z1 >= 0 && z1 <= h)
        rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
      else if (z2 >= 0 && z2 <= h)
        rays.hit(i, t2, getMaterial(), this, getTexCoordMapper());
    }
  }
}


void Cone::computeNormal(const RenderContext&, RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i=rays.begin(); i<rays.end(); i++) {
    Vector xn = rays.getHitPosition(i);
    const Real r_prime = Sqrt(xn.x()*xn.x() + xn.y()*xn.y());
    xn[2] = r/h*r_prime;
    rays.setNormal(i, xn);
  }
}

void Cone::computeTexCoords2(const RenderContext&,
                 RayPacket& rays) const
{
  for(int i=rays.begin();i<rays.end();i++)
    rays.setTexCoords(i, rays.scratchpad<Vector>(i));
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void Cone::computeTexCoords3(const RenderContext& context,
                      RayPacket& rays) const
{
  computeTexCoords2(context, rays);
}
