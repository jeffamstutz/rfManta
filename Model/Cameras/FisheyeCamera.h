
#ifndef Manta_Model_FisheyeCamera_h
#define Manta_Model_FisheyeCamera_h

#include <Interface/Camera.h>
#include <Core/Geometry/Vector.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;

  class FisheyeCamera : public Camera {
  private:
    void setup();
  public:
    FisheyeCamera(const vector<string>& args);
    FisheyeCamera(const Vector& eye_, const Vector& lookat_, const Vector& up_,
                  Real hfov_ = 60, Real vfov_ = 60 );

    virtual ~FisheyeCamera();
    virtual void preprocess(const PreprocessContext& context);
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
    virtual Vector getLookAt()   const { return lookat; };
    virtual Vector getUp()       const { return up; };

    virtual void reset(const Vector& eye_, const Vector& up_,
                       const Vector& lookat_ );

  private:
    Vector  eye;
    Vector  lookat;
    Vector up;
    Real hfov, vfov, width, height, nearZ; // x and y field of view,
                                           // width and height of
                                           // image plane distance
                                           // from eye to image plane

    Vector direction;
    Vector u,v,n;
    bool haveCamera;
  };
}

#endif
