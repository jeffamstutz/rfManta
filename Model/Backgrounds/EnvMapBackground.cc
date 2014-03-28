
#include <Interface/RayPacket.h>
#include <Model/Backgrounds/EnvMapBackground.h>
#include <Core/Math/Trig.h>
#include <Core/Math/Expon.h>
#include <Core/Exceptions/InternalError.h>

using namespace Manta;

static Real OneOverPi=static_cast<Real>(1/M_PI);
static Real OneOverTwoPi=static_cast<Real>(1/(2*M_PI));
//static Real PiOverTwo=static_cast<Real>(M_PI/2);

EnvMapBackground::EnvMapBackground(Texture<Color>* image,
                                   MappingType map_type,
                                   const Vector& right,
                                   const Vector& up)
  : image(image),
    map_type(map_type) {
  // Initialize an orthonormal basis
  setRightUp(right, up);
}

EnvMapBackground::~EnvMapBackground(void) {
}

void EnvMapBackground::setRightUp(Vector right, Vector up)
{
  U=right.normal();
  V=Cross(up, U).normal();
  W=Cross(U, V);
}

void EnvMapBackground::preprocess(const PreprocessContext&) {
  // Do nothing
}

void EnvMapBackground::shade(const RenderContext& context, RayPacket& rays) const {
  switch (map_type) {
  case LatLon:
    LatLonMapping(context, rays);
    break;
  case CylindricalEqualArea:
    CylindricalEqualAreaMapping(context, rays);
    break;
  case DebevecSphere:
    DebevecMapping(context, rays);
    break;
  case OldBehavior:
    OldBehaviorMapping(context, rays);
    break;
  default:
    throw InternalError("Unknown map_type");
    break;
  }

  rays.setFlag(RayPacket::HaveTexture2);

  // Look up the pixel colors
  Packet<Color> colors;
  image->mapValues(colors, context, rays);

  for (int i=rays.begin(); i<rays.end(); ++i)
    rays.setColor(i, colors.get(i));
}

void EnvMapBackground::LatLonMapping(const RenderContext& context, RayPacket& rays) const {
  rays.normalizeDirections();
  // In standard "globe" coordinates we have:
  //
  // x = cos ( longitude ) * cos (latitude)
  // y = sin ( longitude ) * cos (latitude)
  // z = sin ( latitude )
  //
  // In relation to standard spherical coordinates, the only
  // difference is that while longitude = theta, latitude = \pi/2 -
  // phi. From trig: sin(\pi/2 - phi) = cos(phi)
  for (int i = rays.begin(); i < rays.end(); i++) {
    Vector local_dir(Dot(rays.getDirection(i), U),
                     Dot(rays.getDirection(i), V),
                     Dot(rays.getDirection(i), W));

    Real latitude = .5 * M_PI + Asin(local_dir[2]); // [-PI/2, PI/2] + PI/2 -> [0, PI]
    Real longitude = Atan2(local_dir[1], local_dir[0]);
    if (longitude < 0) longitude += 2. * M_PI;
    // longitude is now [0, 2PI), so map this to u
    Real index_u = longitude * OneOverTwoPi;
    Real index_v = latitude * OneOverPi;
    rays.setTexCoords(i, VectorT<Real, 2>(index_u, index_v));
  }
}

void EnvMapBackground::CylindricalEqualAreaMapping(const RenderContext& context, RayPacket& rays) const {
  rays.normalizeDirections();
  for (int i = rays.begin(); i < rays.end(); i++) {
    Vector local_dir(Dot(rays.getDirection(i), U),
                     Dot(rays.getDirection(i), V),
                     Dot(rays.getDirection(i), W));

    // In "standard" spherical coordinates we have:
    //
    // x = r * cos(theta) * sin(phi)
    // y = r * sin(theta) * sin(phi)
    // z = r * cos(phi)
    //
    // So phi = acos(z/r) .
    //
    // Dividing equation 2 by equation 1 we get:
    //
    // y/x = sin(theta)/cos(theta) = tan(theta)
    //
    // So theta = atan(y/x)

    // In this coordinate system W is the up vector, so z is up. To
    // get the latitude and not colatitude though, we need to compute
    // PI/2 - acos(z)
    Real phi = .5 * M_PI - Acos(local_dir[2]);

    // atan2 returns [-PI, PI]
    Real theta = Atan2(local_dir[1], local_dir[0]);
    // NOTE(boulos): Since atan2 is [-PI, PI] we want to turn [-PI, 0]
    // into [PI, 2PI] so we only add 2PI when theta is less than 0.
    if (theta < 0)
      theta += 2. * M_PI;

    // x = (longitude - longitude_center), for Lambert the longitude_center = 0
    Real index_u = theta * OneOverTwoPi;
    // y = sin(phi)
    // sin([-PI/2, PI/2]) -> [-1, 1] but we want phi = 0 to give the equator (index_v = .5)
    Real index_v = .5 * (1 + Sin(phi));

    rays.setTexCoords(i, VectorT<Real, 2>(index_u, index_v));
  }
}

// NOTE(boulos): This code was probably written by James or Christiaan
// but it's not 100% documented. It looks like a cylindrical mapping,
// but I'm not sure which one (as there are many). So I'm keeping it
// around for now.
void EnvMapBackground::OldBehaviorMapping(const RenderContext& context, RayPacket& rays) const {
  for (int i=rays.begin(); i<rays.end(); ++i) {
    const Vector inDir(rays.getDirection(i).normal());
#if 1
    // Note:  Don't invert ray direction during projection into the cylindrical
    //   basis ...
    Vector dir(Dot(inDir, U), Dot(inDir, V), Dot(inDir, W));
    VectorT<Real, 2> texcoords;

    // Compute and scale u
    //   ... and simply invert the y component here
    //
    //   result of Atan2(-y, x) is arctan(y/x) with the quadrant determined by
    //     signs of both arguments, so it is in [-Pi, Pi]
    //   result + Pi is in [0, TwoPi]
    //   result/TwoPi is in [0, 1]
    texcoords[0]=(Atan2(-dir.y(), dir.x()) + Pi)*OneOverTwoPi;

    // Compute and scale v
    //   ... and simply scale by 1/Pi to get v in [0, 1]
    //
    //   result of Acos(z) is in [0, Pi]
    //   1/Pi*result is in [0, 1]
    texcoords[1]=Acos(dir.z())*OneOverPi;
#else
    Vector dir(-Dot(inDir, U), -Dot(inDir, V), -Dot(inDir, W));
    VectorT<Real, 2> texcoords;

    // Compute and scale u
    //   result of Atan2(y, x) is arctan(y/x) with the quadrant determined by
    //     signs of both arguments, so it is in [-Pi, Pi]
    //   result + Pi is in [0, TwoPi]
    //   result/TwoPi is in [0, 1]
    texcoords[0]=(Atan2(dir.y(), -dir.x()) + Pi)*OneOverTwoPi;

    // Compute and scale v
    //   result of Acos(z) is in [0, Pi]
    //   PiOverTwo - result is in [PiOver2, -PiOver2]
    //   Sin(result) is in [1, -1]
    //   1 + result is in [0, 2]
    //   0.5*result is in [0, 1]
    texcoords[1]=0.5*(1 + Sin(PiOverTwo - Acos(dir.z())));
#endif

    rays.setTexCoords(i, texcoords);
  }
}

void EnvMapBackground::DebevecMapping(const RenderContext& context, RayPacket& rays) const {
  rays.normalizeDirections();
  for (int i = rays.begin(); i < rays.end(); i++) {
    Vector local_dir(Dot(rays.getDirection(i), U),
                     Dot(rays.getDirection(i), V),
                     Dot(rays.getDirection(i), W));

    // In Debevec's mapping, he considers Y to be up and -Z to be
    // forward (a standard right handed graphics coordinate
    // system). So to convert our standard UVW space into his, we need
    // to rotate (in our heads) the UVW coordinate system by 90
    // degrees around X. This makes X_deb=X_uvw, Y_deb=Z_uvw and,
    // Z_deb=Y_uvw. Where _deb is the debevec mapping and uvw is the
    // coordinate frame we are given.

    // Given the above note, Debevec explains his mapping as:
    //
    // (X_deb*r, Y_deb*r) where r=(1/PI)*Acos(Z_deb)/sqrt(X_deb^2+Y_deb^2)
    //
    Real r = OneOverPi * Acos(local_dir[1])/Sqrt(local_dir[0]*local_dir[0] + local_dir[2]*local_dir[2]);
    Real u = local_dir[0] * r;
    Real v = local_dir[2] * r;
    // This results in a mapping where u,v \in [-1, 1]^2 so we scale
    // and shift
    u = .5 * (1 + u);
    v = .5 * (1 + v);

    rays.setTexCoords(i, VectorT<Real, 2>(u, v));
  }
}
