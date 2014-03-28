
#ifndef Manta_Interface_Camera_h
#define Manta_Interface_Camera_h

#include <MantaTypes.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/AffineTransform.h>
#include <Interface/Interpolable.h>
#include <iosfwd>

namespace Manta {
  class PreprocessContext;
  class RenderContext;
  class RayPacket;

  class BasicCameraData {
  public:
    BasicCameraData() {}
    BasicCameraData(const Vector& eye,
                    const Vector& lookat,
                    const Vector& up,
                    double hfov,
                    double vfov);
    // The position of the camera
    Vector eye;

    // A point in the gaze direction
    Vector lookat;

    // This will be parallel to the y coordinate of the image

    Vector up;
    // For a pinhole camera, this will be the horizontal field of view.  For a
    // orthographic camera, it will be the effective field of view angle at the
    // lookat point
    double hfov;
    double vfov;

    // Someday, it may be nice to add extra space with an identifier for camera that
    // want to store other parameters here.

    void readwrite(ArchiveElement* archive);
  };

  MANTA_DECLARE_RTTI_BASECLASS(BasicCameraData, ConcreteClass, readwriteMethod);

  class Camera : public Interpolable {
  public:
    Camera();
    virtual ~Camera();
    virtual void preprocess(const PreprocessContext& context) {};
    virtual void makeRays(const RenderContext& context, RayPacket&) const = 0;

    // Camera manipulation
    virtual void scaleFOV(Real) = 0;
    virtual void translate(Vector v) = 0; // Translate in image plane coordinates.
    virtual void dolly(Real) = 0;

    // Accessors
    virtual void getBasicCameraData(BasicCameraData& cam) const = 0;
    virtual BasicCameraData getBasicCameraData() const { BasicCameraData c; getBasicCameraData(c); return c; }

    // Whether/how to maintain an aspect ratio.
    enum AspectRatioMode {
      KeepVertical,
      KeepHorizontal,
      KeepNone
    };
    void setAspectRatioMode(AspectRatioMode mode) { aspectRatioMode = mode; }
    AspectRatioMode getAspectRatioMode() const { return aspectRatioMode; }

    // NOTE(choudhury): This method is not pure virtual so as not to
    // break the other cameras.
    virtual void setAspectRatio(float ratio) { }

    // This is not pass by reference because it needs to be used in a transaction
    virtual void setBasicCameraData(BasicCameraData cam) = 0;

    virtual Vector getPosition() const = 0; // This method is called
                                            // to get the eye point by
                                            // HeadLight etc.

    virtual Vector getUp()       const = 0; // This method returns the
                                            // "horizontal center of
                                            // rotation" about the camera.

    virtual Vector getLookAt()   const = 0; // This method returns the
                                            // "center of rotation
                                            // point" about the look at
                                            // point.

    // Reset the camera.
    virtual void reset( const Vector& eye_,
                        const Vector& up_,
                        const Vector& lookat_ ) = 0;
    virtual void reset( const Vector& eye_,
                        const Vector& up_,
                        const Vector& lookat_,
                        Real hfov_, Real vfov_) {
      reset(eye_, up_, lookat_);
    }

    enum TransformCenter {
      LookAt,
      Eye,
      Origin
    };
    // AffineTransform needs to not be a reference in order to get the
    // transactions to work properly.  A copy is needed.
    virtual void transform(AffineTransform t, TransformCenter) = 0;
    virtual void autoview(const BBox bbox) = 0;
    virtual void output( std::ostream& os ) = 0; // Output a text
                                                 // description of the
                                                 // camera's state.
  protected:
    AspectRatioMode aspectRatioMode;

  private:
    Camera(const Camera&);
    Camera& operator=(const Camera&);
  };
}

#endif
