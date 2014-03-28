 
#ifndef Manta_Model_EnvironmentCamera_h
#define Manta_Model_EnvironmentCamera_h

#include <Interface/Camera.h>
#include <Core/Geometry/Vector.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;

  class EnvironmentCamera : public Camera {
  public:
    EnvironmentCamera(const Vector& eye_, const Vector& lookat_,
                      const Vector& up_ );
    EnvironmentCamera(const vector<string>& args);

    virtual ~EnvironmentCamera();
    virtual void preprocess(const PreprocessContext& context);
    virtual void makeRays(const RenderContext& context, RayPacket&) const;

    // Camera manipulation
    virtual void scaleFOV(Real);
    virtual void translate(Vector);
    virtual void dolly(Real);
    virtual void transform(AffineTransform t, TransformCenter);
    virtual void autoview(const BBox bbox);

    virtual void getBasicCameraData(BasicCameraData& cam) const;
    // This is not pass by reference because it needs to be used in a transaction
    virtual void setBasicCameraData(BasicCameraData cam);

    static Camera* create(const vector<string>& args);

    virtual Vector getPosition() const { return eye; }
    virtual Vector getLookAt()   const { return lookat; };
    virtual Vector getUp()       const { return up; };

    virtual void reset( const Vector& eye_, const Vector& up_,
                        const Vector& lookat_ );

    virtual void output( std::ostream& os );
  private:
    void setup();

    Vector eye;
    Vector lookat;
    Vector up;

    Vector direction;
    Vector u, v, n;
    bool normalizeRays;
    bool haveCamera;
  };
}

#endif
