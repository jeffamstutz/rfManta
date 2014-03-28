
#ifndef Manta_Model_CuttingPlane_h
#define Manta_Model_CuttingPlane_h

#include <Core/Geometry/Vector.h>
#include <Core/Geometry/BBox.h>
#include <Interface/Object.h>
#include <Interface/RayPacket.h>


// Cutting plane for intersections.
// Abe Stephens abe@sgi.com

namespace Manta {

  class CuttingPlane : public Object {
    BBox bounds;             // Bounds of the internal object.
    Object *internal_object; // Object to be cut.

    Real   movement_scale;
    Vector initial_point;

    Vector plane_point;
    Vector plane_normal;

  public:
    CuttingPlane( const Vector &point_, const Vector &normal_,
                  Object *internal_object_ );

    // Preprocess the internal object and compute its bounds.
    void preprocess( const PreprocessContext &context );

    // Return the bounds of the object.
    void computeBounds(const PreprocessContext& context, BBox& bbox) const
    {
      bbox.extendByBox( bounds );
    }

    // Intersection method.
    void intersect(const RenderContext& context, RayPacket& rays) const;

    // Accessors.
    const BBox &getBounds() { return bounds; };
    Object     *getObject() { return internal_object; };
    void setObject( Object *object_ ) { internal_object = object_; };

    void  getPlanePoint ( Vector& result ) { result = plane_point; };
    void  getPlaneNormal( Vector& result ) { result = plane_normal; };

    void setPlanePoint ( const Vector &plane_point_ )
    {
      plane_point = plane_point_;
    }
    void setPlaneNormal( const Vector &plane_normal_ )
    {
      plane_normal = plane_normal_;
    }

    // Move the plane point a signed distance along the plane normal.
    void movePlaneAlongNormal( Real distance );
  };
}

#endif
