

#ifndef Manta_Model_ThinLensCamera_h
#define Manta_Model_ThinLensCamera_h

#include <Interface/Camera.h>
#include <Core/Geometry/Vector.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;

  class ThinLensCamera : public Camera {
  public:
    ThinLensCamera( const Vector& eye_, const Vector& lookat_,
                          const Vector &up_, Real hfov_, Real vfov_, 
                          Real focalDistance_, Real aperture_);
    ThinLensCamera(const vector<string>& args);
    virtual ~ThinLensCamera();
    virtual void preprocess(const PreprocessContext& context);
    virtual void makeRays(const RenderContext& context, RayPacket&) const;

    // Camera manipulation
    virtual void scaleFOV(Real);
    virtual void translate(Vector);
    virtual void dolly(Real);
    virtual void transform(AffineTransform t, TransformCenter);
    virtual void autoview(const BBox bbox);
    virtual void output( std::ostream& os );

    virtual void getBasicCameraData(BasicCameraData& cam) const;
    // This is not pass by reference because it needs to be used in a transaction
    virtual void setBasicCameraData(BasicCameraData cam);

    static Camera* create(const vector<string>& args);

    virtual Vector getPosition() const { return eye; }
    virtual Vector getLookAt()   const { return lookat; };
    virtual Vector getUp()       const { return up; };

    virtual void reset( const Vector& eye_, const Vector& up_,
                        const Vector& lookat_ );

    void setStereoOffset( Real stereo_offset_ ) { stereo_offset = stereo_offset_; }
    Real getStereoOffset() const { return stereo_offset; }

    void setAperture( Real aperture_ ) { aperture = aperture_; radius = aperture/2.0; }
    Real getAperture() const { return aperture; }

    void setFocalLength( Real focal_length_ ) { focal_length = focal_length_; }
    Real getFocalLength() const { return focal_length; }

  protected:

    Vector direction;
    Vector u,v;
    Vector eye;
    Vector lookat;
    Vector up;
    Real  stereo_offset;
    Real  hfov, vfov, width, height; // x and y field of view,
                                            // width and height of image plane
                                            // distance from eye to image plane
    Real focal_length, aperture, radius;

    void setup();
    bool haveCamera;
  };
}


#endif
