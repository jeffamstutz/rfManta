
#ifndef Manta_Model_OrthogonalCamera_h
#define Manta_Model_OrthogonalCamera_h

#include <Interface/Camera.h>
#include <Core/Geometry/Vector.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;

  class OrthogonalCamera : public Camera {
  public:
    OrthogonalCamera(const Vector& eye_, const Vector& lookat_,
                     const Vector& up_, Real hscale_, Real vscale_ );
    OrthogonalCamera(const vector<string>& args);
    virtual void preprocess(const PreprocessContext& context);
    virtual ~OrthogonalCamera();
    virtual void makeRays(const RenderContext& context, RayPacket&) const;

    // Camera manipulation
    virtual void scaleFOV(Real);
    virtual void translate(Vector);
    virtual void dolly(Real);
    virtual void transform(AffineTransform t, TransformCenter);
    virtual void autoview(const BBox bbox);
    virtual void output( std::ostream &os );
    static Camera* create(const vector<string>& args);

    virtual void getBasicCameraData(BasicCameraData& cam) const;
    // This is not pass by reference because it needs to be used in a transaction
    virtual void setBasicCameraData(BasicCameraData cam);

    virtual Vector getPosition() const { return eye; }
    virtual Vector getLookAt()   const { return lookat; }
    virtual Vector getUp()       const { return up; }

    virtual void reset(const Vector& eye_, const Vector& up_,
                       const Vector& lookat_ );

  private:
    void setup();
    Vector eye;
    Vector lookat;
    Vector up;
    Real hscale, vscale;

    Vector direction;
    Vector u,v;
    bool haveCamera;
  };
}

#endif
