
#include <Interface/Context.h>
#include <Model/MiscObjects/CuttingPlane.h>
#include <Model/Intersections/AxisAlignedBox.h>
#include <Model/Intersections/Plane.h>
#include <iostream>


using namespace Manta;

CuttingPlane::CuttingPlane( const Vector &point_, const Vector &normal_,
                            Object *internal_object_ )
  : internal_object( internal_object_ ),
    movement_scale( 1 ),
    initial_point( point_ ),
    plane_point( point_ ),
    plane_normal( normal_ )
{
}

void CuttingPlane::movePlaneAlongNormal( Real distance )
{
  plane_point = initial_point + (plane_normal * distance * 100.0);
}

// Preprocess the internal object and compute its bounds.
void CuttingPlane::preprocess( const PreprocessContext &context )
{
  // Call preprocess on the object.
  internal_object->preprocess( context );

  // Compute its bounds.
  internal_object->computeBounds( context, bounds );

  // Determine how far away the edges of the bounding box are from the
  // initial point.
  if (context.proc == 0)
    movement_scale = bounds.diagonal().length() * (Real)0.5;

  context.done();
}

void CuttingPlane::intersect(const RenderContext& context, RayPacket& rays) const {

  // Send a new ray packet with new ray origins.
  RayPacketData new_data;
  RayPacket     new_rays( new_data, RayPacket::UnknownShape, rays.begin(), rays.end(),
                          rays.getDepth(), rays.getAllFlags());

  rays.normalizeDirections();
  rays.computeInverseDirections();
  rays.computeSigns();

  // Map between rays in original packet and new packet in case some rays are skipped.
  int packet_map[RayPacket::MaxSize];

  // Keep track of which new rays intersect the front face of the plane.
  bool front_facing[RayPacket::MaxSize];

  // Keep track of the plane_t offset for each ray.
  Real plane_t[RayPacket::MaxSize];

  /////////////////////////////////////////////////////////////////////////////
  // Create the new rays.
  int new_i = 0;
  for (int i=rays.begin();i<rays.end();++i) {
    Real box_min, box_max;

    Ray ray = rays.getRay(i);
    // Check to see if the ray intersects the bounding box.
    if (intersectAaBox( bounds, box_min, box_max,
                                      ray, rays.getSigns(i),
                                      rays.getInverseDirection(i) )) {

      // Intersect the ray with the plane.
      intersectPlane( plane_point, plane_normal, plane_t[new_i],
                                    ray );

      // Record which original ray this new ray maps to.
      packet_map[new_i] = i;

      // Check to see if the ray origin is on the front or back facing side of
      // the cutting plane (the front facing side is cut away).
      front_facing[new_i] = Dot(plane_normal, (ray.origin()-plane_point)) > 0;
      if (front_facing[new_i]) {

        // If plane_t is negative, the ray can't possibly hit the cut model.
        // because the ray is pointing away from the plane.
        if (plane_t[new_i] > 0) {

          // If front facing, move the new ray to the cutting plane.
          new_rays.setRay(new_i, ray.origin()+(ray.direction()*plane_t[new_i]),
                          ray.direction());
          new_rays.resetFlag(RayPacket::ConstantOrigin);


          // Subtract the distance from the hit t.
          new_rays.resetHit( new_i, rays.getMinT(i) - plane_t[new_i] );

          ++new_i;
        }
      }
      else {

        // Otherwise if back facing, move the hit t in the hit record
        // to the plane (it will need to be moved back afterwards)
        new_rays.setRay(new_i, ray);
        new_rays.resetHit( new_i, rays.getMinT(i) );

        ++new_i;
      }


    }
  }

  // Check to see if all of the rays miss the bounds.
  if (new_i == 0) {
    return;
  }

  // Specify the number of new rays.
  new_rays.resize( new_i );

  /////////////////////////////////////////////////////////////////////////////
  // Intersect the new ray packet with the internal object.
  internal_object->intersect( context, new_rays );

  /////////////////////////////////////////////////////////////////////////////
  // Map the results back to the old rays.
  for (new_i=new_rays.begin(); new_i<new_rays.end(); ++new_i) {

    // Check to see if the new ray hit something.
    if (new_rays.wasHit(new_i)) {

      // Determine which old ray this maps to.
      int old_i = packet_map[new_i];

      // Check to see if the old ray is front or back facing.
      if (front_facing[new_i]) {
        // If so, then translate the hit t back into the old hit record.
        if (rays.hit( old_i, new_rays.getMinT(new_i)+plane_t[new_i],
                      new_rays.getHitMaterial(new_i),
                      new_rays.getHitPrimitive(new_i),
                      new_rays.getHitTexCoordMapper(new_i) ))
          rays.copyScratchpad( old_i, new_rays, new_i );
      }
      else {
        // Otherwise, if the original ray is back facing check to see
        // if the hit is closer then the cutting plane.
        if ((plane_t[new_i]<0) || (new_rays.getMinT(new_i) < plane_t[new_i])) {
          if (rays.hit( old_i, new_rays.getMinT(new_i),
                        new_rays.getHitMaterial(new_i),
                        new_rays.getHitPrimitive(new_i),
                        new_rays.getHitTexCoordMapper(new_i) ))
            rays.copyScratchpad( old_i, new_rays, new_i );
        }
      }
    }
  }
}
