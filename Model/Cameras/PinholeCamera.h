

#ifndef Manta_Model_PinholeCamera_h
#define Manta_Model_PinholeCamera_h

#include <Interface/Camera.h>
#include <Core/Geometry/Vector.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;

  class PinholeCamera : public Camera {
  public:
    PinholeCamera( const Vector& eye_, const Vector& lookat_,
                   const Vector &up_, Real hfov_, Real vfov_ );
    PinholeCamera(const vector<string>& args);
    virtual ~PinholeCamera();
    virtual void preprocess(const PreprocessContext& context);
    virtual void makeRays(const RenderContext& context, RayPacket&) const;

    // Camera manipulation
    virtual void scaleFOV(Real);
    virtual void translate(Vector);
    virtual void dolly(Real);
    virtual void transform(AffineTransform t, TransformCenter);
    virtual void autoview(const BBox bbox);
    virtual void output( std::ostream& os );

    virtual void setAspectRatio(float ratio);

    virtual void getBasicCameraData(BasicCameraData& cam) const;
    // This is not pass by reference because it needs to be used in a transaction
    virtual void setBasicCameraData(BasicCameraData cam);

    static Camera* create(const vector<string>& args);

    virtual Vector getPosition() const { return eye; }
    virtual Vector getLookAt()   const { return lookat; };
    virtual Vector getUp()       const { return up; };

    virtual void reset( const Vector& eye_, const Vector& up_,
                        const Vector& lookat_ );
    virtual void reset( const Vector& eye_, const Vector& up_,
                        const Vector& lookat_, Real hfov_, Real vfov_);

    void setStereoOffset( Real stereo_offset_ ) { stereo_offset = stereo_offset_; }
    Real getStereoOffset() const { return stereo_offset; }

  protected:
    template <bool NORMALIZE_RAYS, bool CREATE_CORNER_RAYS>
    void makeRaysSpecialized(RayPacket& rays) const;

    Vector direction;
    Vector u,v;
    Vector eye;
    Vector lookat;
    Vector up;
    Real  stereo_offset;
    Real  hfov, vfov, width, height, nearZ; // x and y field of view,
                                            // width and height of image plane
                                            // distance from eye to image plane
    void setup();
    bool normalizeRays;
    bool createCornerRays;
    bool haveCamera;

    // Choudhury: this variable is part of a hack to get
    // PinholeCameras to display non-square images properly at
    // startup.
    float savedRatio;
  };
}

#endif
