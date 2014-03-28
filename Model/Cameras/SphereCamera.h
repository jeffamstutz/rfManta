#ifndef Manta_Model_SphereCamera_h
#define Manta_Model_SphereCamera_h

#include <Interface/Camera.h>
#include <Core/Geometry/Vector.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;

  class SphereCamera : public Camera {
  public:
    SphereCamera(const vector<string>& args);
    SphereCamera(const Vector& eye);

    virtual ~SphereCamera();
    virtual void preprocess(const PreprocessContext& context);
    virtual void makeRays(const RenderContext& context, RayPacket&) const;

    // Camera manipulation: This camera can't be manipulated. Unless
    // you want to implement a method for this...
    virtual void scaleFOV(Real) { }
    virtual void translate(Vector) { }
    virtual void dolly(Real) { }
    virtual void transform(AffineTransform t, TransformCenter) { }
    virtual void autoview(const BBox bbox) { }
    virtual void output( std::ostream &os );
    static Camera* create(const vector<string>& args);

    virtual void getBasicCameraData(BasicCameraData& cam) const;
    // This is not pass by reference because it needs to be used in a transaction
    virtual void setBasicCameraData(BasicCameraData cam);

    virtual Vector getPosition() const { return eye; }
    virtual Vector getLookAt()   const { return Vector(0,0,0); };
    virtual Vector getUp()       const { return Vector(0,1,0); };

    virtual void reset(const Vector& eye_, const Vector& up_,
                       const Vector& lookat_ );

  private:
    Vector eye;
    bool haveCamera;
  };
}

#endif
