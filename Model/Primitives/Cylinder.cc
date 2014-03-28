#include <Model/Primitives/Cylinder.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Exceptions/BadPrimitive.h>

#include <Core/Math/Expon.h>

using namespace Manta;
using namespace std;

Cylinder::Cylinder(Material* mat, const Vector& bottom, const Vector& top,
                   Real radius)
  : PrimitiveCommon(mat, this), bottom(bottom), top(top), radius(radius) 
{
  Vector axis(top-bottom);
  Real height = axis.normalize();
  // Set up unit transformation
  xform.initWithTranslation(-bottom);
  xform.rotate(axis, Vector(0,0,1));
  Real inv_radius = 1/radius;
  xform.scale(Vector(inv_radius, inv_radius, 1/height));
  // And the inverse for normals and what not
  ixform.initWithScale(Vector(radius, radius, height));
  ixform.rotate(Vector(0,0,1), axis);
  ixform.translate(bottom);
}

Cylinder::~Cylinder()
{
}

void Cylinder::computeBounds(const PreprocessContext&, BBox& bbox) const
{
  Vector axis(top-bottom);
  axis.normalize();
  bbox.extendByDisc(bottom, axis, radius);
  bbox.extendByDisc(top, axis, radius);
}

void Cylinder::intersect(const RenderContext&, RayPacket& rays) const
{
  for(int i=rays.begin(); i<rays.end(); i++) {
    Vector v(xform.multiply_vector(rays.getDirection(i)));
    Real dist_scale=v.normalize();
    Ray xray(xform.multiply_point(rays.getOrigin(i)), v);
    Real dx=xray.direction().x();
    Real dy=xray.direction().y();
    Real a=dx*dx+dy*dy;
    if(a >= T_EPSILON) {
      // Check sides...
      Real ox=xray.origin().x();
      Real oy=xray.origin().y();
      Real oz=xray.origin().z();
      Real dz=xray.direction().z();

      Real b=2*(ox*dx+oy*dy);
      Real c=ox*ox+oy*oy-1;
      Real d=b*b-4*a*c;
      if(d>T_EPSILON) {
	Real sd=Sqrt(d);
	Real t1=(-b+sd)/(2*a);
	Real t2=(-b-sd)/(2*a);
	
	if(t1>t2){
	  Real tmp=t1;
	  t1=t2;
	  t2=tmp;
	}
	Real z1=oz+t1*dz;
	Real z2=oz+t2*dz;
	if(t1 > T_EPSILON && z1 > 0 && z1 < 1){
	  rays.hit(i, t1/dist_scale, getMaterial(), this, getTexCoordMapper());
	} else if(t2 > T_EPSILON && z2 > 0 && z2 < 1){
	  rays.hit(i, t2/dist_scale, getMaterial(), this, getTexCoordMapper());
	}
      }
    }
  }
}

void Cylinder::computeNormal(const RenderContext&, RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i=rays.begin(); i < rays.end(); i++) {
    Vector xn(xform.multiply_point(rays.getHitPosition(i)));
    xn[2]=0.0;
    Vector v=ixform.multiply_vector(xn);
    v.normalize();
    rays.setNormal(i, v);
  }
  // We either need to set the flag here or don't normalize the normals.
  rays.setFlag(RayPacket::HaveUnitNormals);
}


void Cylinder::computeTexCoords2(const RenderContext&,
			     RayPacket& rays) const
{
  for(int i=rays.begin();i<rays.end();i++)
    rays.setTexCoords(i, rays.scratchpad<Vector>(i));
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void Cylinder::computeTexCoords3(const RenderContext&,
				      RayPacket& rays) const
{
  for(int i=rays.begin();i<rays.end();i++)
    rays.setTexCoords(i, rays.scratchpad<Vector>(i));
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}
