#include <Model/Primitives/SuperEllipsoid.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>
#include <iostream>

using namespace std;
using namespace Manta;

static const Real golden_mean = static_cast< Real >( 0.61803398874989484820 );
static const Real bracket_width = static_cast< Real >( 1.e-3 );
static const int max_newton_iter = 10;

SuperEllipsoid::SuperEllipsoid(
  Material *material,
  Vector const &center,
  Real radius,
  Real alpha,
  Real beta )
  : PrimitiveCommon( material, this ),
    center( center ),
    radius( radius ),
    inv_radius( 1 / radius ),
    two_over_alpha( 2 / alpha ),
    two_over_beta( 2 / beta ),
    alpha_over_beta( alpha / beta )
{
}

SuperEllipsoid::~SuperEllipsoid()
{
}

inline Real SuperEllipsoid::functionValue(
  Vector const &location ) const
{
  Real x_power = Pow( Abs( location.x() ), two_over_alpha );
  Real y_power = Pow( Abs( location.y() ), two_over_alpha );
  Real z_power = Pow( Abs( location.z() ), two_over_beta );
  return Pow( x_power + y_power, alpha_over_beta ) + z_power - 1;
}

inline Vector SuperEllipsoid::functionGradient(
  Vector const &location,
  Real &value ) const
{
  Real x_power = Pow( Abs(location.x() ), two_over_alpha );
  Real y_power = Pow( Abs(location.y() ), two_over_alpha );
  Real z_power = Pow( Abs(location.z() ), two_over_beta );
  Real x_and_y_power = Pow( x_power + y_power, alpha_over_beta - 1 );
  value = x_and_y_power * ( x_power + y_power ) + z_power - 1;
  return Vector( two_over_beta * x_and_y_power * x_power / location.x(),
                 two_over_beta * x_and_y_power * y_power / location.y(),
                 two_over_beta * z_power / location.z() );
}

inline Vector SuperEllipsoid::logarithmFunctionGradient(
  Vector const &location,
  Real &value ) const
{
  Real x_power = Pow( Abs(location.x() ), two_over_alpha );
  Real y_power = Pow( Abs(location.y() ), two_over_alpha );
  Real z_power = Pow( Abs(location.z() ), two_over_beta );
  Real x_and_y_power = ( Pow( x_power + y_power, 1 - alpha_over_beta ) * z_power + x_power + y_power );
  value = log( Pow( x_power + y_power, alpha_over_beta ) + z_power );
  return Vector( two_over_beta * x_power / ( location.x() * x_and_y_power ),
                 two_over_beta * y_power / ( location.y() * x_and_y_power ),
                 two_over_beta / ( location.z() * ( 1 + Pow( x_power + y_power, alpha_over_beta ) / z_power ) ) );
}

void SuperEllipsoid::computeBounds(
    PreprocessContext const &,
    BBox &bbox ) const
{
    bbox.extendByPoint( Vector( -1, -1, -1 ) );
    bbox.extendByPoint( Vector(  1,  1,  1 ) );
}

void SuperEllipsoid::intersect(const RenderContext&, RayPacket& rays) const
{

  rays.computeInverseDirections();
  for( int i = rays.begin(); i < rays.end(); ++i ) {
    Vector offset_center = rays.getOrigin(i) - center;

    // First check if the ray hits the bounding box and whether it could
    // remotely produce a hit of interest.
    // TODO: Maybe factor this out into a common routine?
    Vector inverseDirection = rays.getInverseDirection(i);
    Real tnear, tfar, t1, t2;
    t1 = ( -radius - offset_center.x() ) * inverseDirection.x();
    t2 = ( radius - offset_center.x() ) * inverseDirection.x();
    if( t1 > t2 ) {
      Real temp = t1;
      t1 = t2;
      t2 = temp;
    }
    tnear = t1;
    tfar = t2;
    t1 = ( -radius - offset_center.y() ) * inverseDirection.y();
    t2 = ( radius - offset_center.y() ) * inverseDirection.y();
    if( t1 > t2 ) {
      Real temp = t1;
      t1 = t2;
      t2 = temp;
    }
    tnear = Max( t1, tnear );
    tfar = Min( t2, tfar );
    t1 = ( -radius - offset_center.z() ) * inverseDirection.z();
    t2 = ( radius - offset_center.z() ) * inverseDirection.z();
    if( t1 > t2 ) {
      Real temp = t1;
      t1 = t2;
      t2 = temp;
    }
    tnear = Max( Max( t1, tnear ), (Real)T_EPSILON );
    tfar = Min( Min( t2, tfar ), rays.getMinT(i) );

    if ( tnear > tfar || tfar <= T_EPSILON || tnear >= rays.getMinT(i) )
      continue;

    // A few preliminary early exit tests...
    Real near_value, far_value;
    Vector rayD = rays.getDirection(i);
    Real near_deriv = Dot( functionGradient( ( offset_center + tnear * rayD )
                                             * inv_radius, near_value ),
                           rayD );
    Real far_deriv = Dot( functionGradient( ( offset_center + tfar * rayD )
                                            * inv_radius, far_value ),
                          rayD );
    if ( ( near_value < (Real)T_EPSILON && far_value < (Real)T_EPSILON ) ||
         ( near_value * far_value > 0 && near_deriv * far_deriv > 0 ) )
      continue;

    // Try to find the minimum of the super ellipsoid function using the
    // Golden Section Search.  We'll use this to bracket the root.
    if ( near_deriv * far_deriv <= 0 ) {
      Real a_bracket = tnear;
      Real b_bracket = tfar;
      Real left = golden_mean * a_bracket + ( 1 - golden_mean ) * b_bracket;
      Real left_value = functionValue( ( offset_center + left * rayD )
                                       * inv_radius );
      Real right = ( 1 - golden_mean ) * a_bracket + golden_mean * b_bracket;
      Real right_value = functionValue( ( offset_center + right * rayD )
                                        * inv_radius );
      while( Abs( b_bracket - a_bracket ) > bracket_width ) {
        if ( left_value < right_value ) {
          b_bracket = right;
          right = left;
          right_value = left_value;
          left = golden_mean * a_bracket + ( 1 - golden_mean ) * b_bracket;
          left_value = functionValue( ( offset_center + left * rayD )
                                      * inv_radius );
        } else {
          a_bracket = left;
          left = right;
          left_value = right_value;
          right = ( 1 - golden_mean ) * a_bracket + golden_mean * b_bracket;
          right_value = functionValue( ( offset_center + right * rayD )
                                       * inv_radius );
        }
      }

      // If our minimum is positive, we missed the superquadric - it
      // couldn't have crossed zero.
      if ( right_value >= (Real)-T_EPSILON )
        continue;

      // Narrow the range with the location of the minimum found
      if ( right_value * near_value < 0 ) {
        tfar = right;
        far_value = right_value;
      } else {
        tnear = right;
        near_value = right_value;
      }

    }

    // Bail if the root still isn't bracketed.  Otherwise use Newtown's
    // search on the logarithm of the super ellipsoid function to find the
    // root.  If Newton's method flies off, we just reset it with a
    // bisection step.
    if ( near_value * far_value >= 0 )
      continue;
    Real troot = ( tnear + tfar ) * (Real)0.5;
    Real root_value;
    Real root_deriv =
      Dot( logarithmFunctionGradient( ( offset_center + troot * rayD )
                                      * inv_radius, root_value ),
           rayD );
    int iterations = 0;
    while ( Abs( tfar - tnear ) >= (Real)T_EPSILON &&
            Abs( root_value ) >= (Real)T_EPSILON &&
            ++iterations < max_newton_iter ) {
      if ( root_value * near_value < 0 ) {
        tfar = troot;
        far_value = root_value;
      } else {
        tnear = troot;
        near_value = root_value;
      }
      troot -= root_value / root_deriv;
      if ( troot <= tnear || troot >= tfar )
        troot = ( tnear + tfar ) * (Real)0.5;
      root_deriv =
        Dot( logarithmFunctionGradient( ( offset_center + troot * rayD )
                                        * inv_radius, root_value ),
             rayD );
    }

    // Finally, set the hit location
    rays.hit( i, troot, getMaterial(), this, getTexCoordMapper() );

  }

}

void SuperEllipsoid::computeNormal(
  RenderContext const &,
  RayPacket &rays ) const
{
  Real ignored;
  rays.computeHitPositions();
  for( int i = rays.begin(); i < rays.end(); i++ )
    rays.setNormal(i, functionGradient( Vector(rays.getHitPosition(i)),
                                        ignored ));
  rays.setFlag(RayPacket::HaveNormals);
}

void SuperEllipsoid::computeTexCoords2(
  const RenderContext &,
  RayPacket &rays ) const
{
  rays.computeHitPositions();
  for( int i = rays.begin(); i < rays.end(); i++ ) {
    Vector n = ( rays.getHitPosition(i) - center ) * inv_radius;
    Real angle = Clamp( n.z(), (Real)-1, (Real)1 );
    Real theta = Acos( angle );
    Real phi = Atan2( n.x(), n.y() );
    Real x = ( phi + (Real)M_PI ) * (Real)( 0.5 * M_1_PI );
    Real y = theta * (Real)M_1_PI;
    rays.setTexCoords(i, Vector( x, y, 0));
  }
  rays.setFlag( RayPacket::HaveTexture2 | RayPacket::HaveTexture3 );
}

void SuperEllipsoid::computeTexCoords3(
  const RenderContext &,
  RayPacket &rays ) const
{
  rays.computeHitPositions();
  for( int i = rays.begin(); i < rays.end(); i++ ) {
    Vector n = ( rays.getHitPosition(i) - center ) * inv_radius;
    Real angle = Clamp( n.z(), (Real)-1, (Real)1 );
    Real theta = Acos( angle );
    Real phi = Atan2( n.x(), n.y() );
    Real x = ( phi + (Real)M_PI ) * (Real)( 0.5 * M_1_PI );
    Real y = theta * (Real)M_1_PI;
    rays.setTexCoords(i, Vector( x, y, 0));
  }
  rays.setFlag( RayPacket::HaveTexture2 | RayPacket::HaveTexture3 );
}
