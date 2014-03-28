#include <Model/Primitives/Torus.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/Expon.h>
#include <assert.h>
using namespace Manta;
using namespace std;



/**
 * Roots of a quadratic polynomial.  This is based on the "alternate"
 * for of the quadratic formula given at MathWorld
 * (http://mathworld.wolfram.com/QuadraticEquation.html).
 * This version is supposed to be a little more numerically stable in
 * certain cases.  The coefficients are passed in with the greatest
 * exponent term first.
 *
 * @param coefficients  a 3-value array of the polynomial coefficients
 * @param roots  an array that will be filled with up to 2 real roots
 * @return  the number of real roots of the polynomial, from 0 to 2
 */
inline int quadratic_roots(double const coefficients[ 3 ], double roots[ 2 ] )
{
  double discriminant = ( coefficients[ 1 ] * coefficients[ 1 ] +
                          -4.0 * coefficients[ 0 ] * coefficients[ 2 ] );
  if ( discriminant < 0.0 )
    return 0;
  else if ( discriminant > 0.0 ) {
    double q = ( fabs( coefficients[ 1 ] ) + sqrt( discriminant ) ) * 0.5;
    if ( coefficients[ 1 ] > 0 )
      q = -q;
    roots[ 0 ] = q / coefficients[ 0 ];
    roots[ 1 ] = coefficients[ 2 ] / q;
    return 2;
  }
  else {
    roots[ 0 ] = coefficients[ 1 ] / coefficients[ 0 ] * -0.5;
    return 1;
  }
}

/**
 * Roots of a cubic polynomial.  This is based on Cardano's cubic
 * formula as given "Mathematics for 3D Game Programming & Computer
 * Graphics, 2nd Ed." by Eric Lengyel (p. 135).  When there are three
 * real roots, it uses a trigonometric approach to avoid the complex
 * arithmetic.
 *
 * @param coefficients  a 4-value array of the polynomial coefficients
 * @param roots  an array that will be filled with up to 3 real roots
 * @return  the number of real roots of the polynomial, from 1 to 3
 */
inline int cubic_roots(double const coefficients[ 4 ], double roots[ 3 ] )
{
  double a = coefficients[ 1 ] / coefficients[ 0 ];
  double b = coefficients[ 2 ] / coefficients[ 0 ];
  double c = coefficients[ 3 ] / coefficients[ 0 ];
  double shift = -a / 3.0;
  double p = ( b - a * a / 3.0 ) / 3.0;
  double negative_q = a * ( 2.0 * a * a - 9.0 * b ) / -54.0 + c * -0.5;
  double negative_discriminant = p * p * p + negative_q * negative_q;
  if ( negative_discriminant > 0.0 ) {
    double sqrt_negative_discriminant = sqrt( negative_discriminant );
    double r = negative_q + sqrt_negative_discriminant;
    r = ( r > 0 ? pow( r, 1.0 / 3.0 ) : -pow( -r, 1.0 / 3.0 ) );
    double s = negative_q - sqrt_negative_discriminant;
    s = ( s > 0 ? pow( s, 1.0 / 3.0 ) : -pow( -s, 1.0 / 3.0 ) );
    roots[ 0 ] = r + s + shift;
    return 1;
  }
  else if ( negative_discriminant < 0.0 ) {
    double theta = acos( negative_q / sqrt( -p * p * p ) ) / 3.0;
    double two_sqrt_negative_p = 2.0 * sqrt( -p );
    roots[ 0 ] = two_sqrt_negative_p * cos( theta ) + shift;
    roots[ 1 ] = two_sqrt_negative_p * cos( theta + M_PI * 2.0 / 3.0 ) + shift;
    roots[ 2 ] = two_sqrt_negative_p * cos( theta - M_PI * 2.0 / 3.0 ) + shift;
    return 3;
  }
  else {
//     if ( tolerant_equals( static_cast< float >( negative_q ), 0.0 ) ) {
    if (  fabs(negative_q) < 1e-7 ) {
      roots[ 0 ] = shift;
      return 1;
    }
    double r = negative_q > 0 ? pow( negative_q, 1.0 / 3.0 ) : -pow( -negative_q, 1.0 / 3.0 );
    roots[ 0 ] = r + r + shift;
    roots[ 1 ] = shift - r;
    return 2;
  }
}

/**
 * Roots of a quartic polynomial.  This is based on the
 * formula as given "Mathematics for 3D Game Programming & Computer
 * Graphics, 2nd Ed." by Eric Lengyel (p. 138).
 *
 * @param coefficients  a 5-value array of the polynomial coefficients
 * @param roots  an array that will be filled with up to 4 real roots
 * @return  the number of real roots of the polynomial, from 0 to 4
 */
inline int quartic_roots(double const coefficients[ 5 ], double roots[ 4 ] )
{
  double a = coefficients[ 1 ] / coefficients[ 0 ];
  double b = coefficients[ 2 ] / coefficients[ 0 ];
  double c = coefficients[ 3 ] / coefficients[ 0 ];
  double d = coefficients[ 4 ] / coefficients[ 0 ];
  double a_squared = a * a;
  double shift = a * -0.25;
  double p = -0.375 * a_squared + b;
  double q = a * a_squared * 0.125 + a * b * -0.5 + c;
  double r = a_squared * ( -0.01171875 * a_squared + b * 0.0625 ) + a * c * -0.25 + d;
  double subproblem[ 4 ];
  subproblem[ 0 ] = 1.0;
  subproblem[ 1 ] = p * -0.5;
  subproblem[ 2 ] = -r;
  subproblem[ 3 ] = r * p * 0.5 + q * q * -0.125;
  cubic_roots( subproblem, roots );
  double y = roots[ 0 ];
  double j = sqrt( 2.0 * y - p );
  double k = sqrt( y * y - r );
  subproblem[ 0 ] = 1;
  subproblem[ 1 ] = j;
  subproblem[ 2 ] = q >= 0.0 ? y - k : y + k;
  int number_of_roots = quadratic_roots( subproblem, roots );
  subproblem[ 1 ] = -j;
  subproblem[ 2 ] = q >= 0.0 ? y + k : y - k;
  number_of_roots += quadratic_roots( subproblem, roots + number_of_roots );
  for ( int i = 0; i < number_of_roots; ++i )
    roots[ i ] += shift;
  return number_of_roots;
}

Torus::Torus(Material* mat, double minor_radius, double major_radius)
  : PrimitiveCommon(mat, this), minor_radius(minor_radius), major_radius(major_radius)
{
  assert(minor_radius >= 0 && major_radius >= 0);
}

Torus* Torus::clone(CloneDepth depth, Clonable* incoming)
{
  Torus *copy;
  if (incoming)
    copy = dynamic_cast<Torus*>(incoming);
  else
    copy = new Torus();

  PrimitiveCommon::clone(depth, copy);
  copy->minor_radius = minor_radius;
  copy->major_radius = major_radius;
  return copy;
}

Interpolable::InterpErr Torus::serialInterpolate(const std::vector<keyframe_t> &keyframes)
{
  PrimitiveCommon::interpolate(keyframes);
  minor_radius = 0.0f;
  major_radius = 0.0f;

  for (size_t i=0; i < keyframes.size(); ++i) {
    Torus *torus = dynamic_cast<Torus*>(keyframes[i].keyframe);
    if (torus == NULL)
      return notInterpolable;
    minor_radius += torus->minor_radius * keyframes[i].t;
    major_radius += torus->major_radius * keyframes[i].t;
  }
  return success;
}

void Torus::computeBounds(const PreprocessContext&, BBox& bbox) const
{
  bbox.extendByBox(BBox( Vector( -major_radius - minor_radius,
                                 -major_radius - minor_radius,
                                 -minor_radius ),
                         Vector( major_radius + minor_radius,
                                 major_radius + minor_radius, 
                                 minor_radius ) ));
}

void Torus::intersect(const RenderContext&, RayPacket& rays) const
{
  for(int i=rays.begin(); i<rays.end(); i++) {
    const Vector O = rays.getOrigin(i);
    const Vector D = rays.getDirection(i);

    const double O_dot_D = Dot(O,D);
    const double major_radius_squared = major_radius * major_radius;
    const double minor_radius_squared = minor_radius * minor_radius;

    double term = (  Dot(O,O)  - major_radius_squared - minor_radius_squared );
    double coefficients[ 5 ];
    coefficients[ 0 ] = 1.0;
    coefficients[ 1 ] = 4.0 * O_dot_D;
    coefficients[ 2 ] = 2.0 * term + 4.0 * ( O_dot_D * O_dot_D + major_radius_squared * D[ 2 ] * D[ 2 ] );
    coefficients[ 3 ] = 4.0 * O_dot_D * term + 8.0 * major_radius_squared * O[ 2 ] * D[ 2 ];
    coefficients[ 4 ] = term * term - 4.0 * major_radius_squared * ( minor_radius_squared - O[ 2 ] * O[ 2 ] );
    double roots[ 4 ];
    int number_of_roots = quartic_roots( coefficients, roots );
    for ( int root = 0; root < number_of_roots; ++root )
      rays.hit(i, roots[root], getMaterial(), this, getTexCoordMapper());
  }
}

void Torus::computeNormal(const RenderContext&, RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i=rays.begin(); i<rays.end(); i++) {
    Vector intersection_point = rays.getHitPosition(i);
    double magnitude = Sqrt( intersection_point[ 0 ] * intersection_point[ 0 ] +
                            intersection_point[ 1 ] * intersection_point[ 1 ] );
    double one_over_minor_radius = 1.0f / minor_radius;
    double scale = ( 1.0f - major_radius / magnitude ) * one_over_minor_radius;
    Vector normal( intersection_point[ 0 ] * scale,
                   intersection_point[ 1 ] * scale,
                   intersection_point[ 2 ] * one_over_minor_radius );

    rays.setNormal(i,normal );
  }
}

void Torus::computeTexCoords2(const RenderContext&,
                              RayPacket& rays) const
{
  for(int i=rays.begin();i<rays.end();i++)
    rays.setTexCoords(i, rays.scratchpad<Vector>(i));
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void Torus::computeTexCoords3(const RenderContext& context,
                              RayPacket& rays) const
{
  computeTexCoords2(context, rays);
}
