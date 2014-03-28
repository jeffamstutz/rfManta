#ifndef aip_h
#define aip_h

#include <stdio.h>
#define bigReal 1.E38  // FLT_MAX, DBL_MAX if double above
#include <limits.h>
#include <Core/Geometry/VectorT.h>
#include <Core/Util/Preprocessor.h>

using namespace Manta;

/**
 * Area of Intersection of Polygons
 *
 * Algorithm based on http://cap-lore.com/MathPhys/IP/
 *
 * Adapted 9-May-2006 by Lagado to java.
 */

typedef VectorT<double, 2> Point2D;
typedef long long hp;

class PolygonIntersect
{
public:
  /**
   * return the area of intersection of two polygons
   *
   * Note: the area result has little more accuracy than a float
   *  This is true even if the polygon is specified with doubles.
   */
  static double intersectionArea(const vector<Point2D> &a,
                                 const vector<Point2D> &b)
  {
    PolygonIntersect polygonIntersect;
    return polygonIntersect.inter(a, b);
  }

    //--------------------------------------------------------------------------

  struct Point {
    double x; double y;
    Point() { };
    Point(double x, double y) { this->x = x; this->y = y; }
  };
  struct Box {
    Point min; Point max;
    Box(Point min, Point max) { this->min = min; this->max = max; }
  };
  struct Rng {
    int mn; int mx;
    Rng() { }
    Rng(int mn, int mx) { this->mn = mn; this->mx = mx; }
  };
  struct IPoint { int x; int y; };
  struct Vertex { IPoint ip; Rng rx; Rng ry; int in; };

//   static const double gamut = 500000000.;
//   static const double mid = 500000000 / 2.;
#define gamut 500000000
#define mid   250000000

  //--------------------------------------------------------------------------

private:
  static void range(const vector<Point2D> &points, int c, Box &bbox)
  {
    while (c-- > 0) {
      bbox.min.x = Min(bbox.min.x, points[c][0]);
      bbox.min.y = Min(bbox.min.y, points[c][1]);
      bbox.max.x = Max(bbox.max.x, points[c][0]);
      bbox.max.y = Max(bbox.max.y, points[c][1]);
    }
  }

  static hp area(IPoint a, IPoint p, IPoint q) {
    return (hp)p.x * q.y - (hp)p.y * q.x +
      (hp)a.x * (p.y - q.y) + (hp)a.y * (q.x - p.x);
  }

  static bool ovl(Rng p, Rng q) {
    return p.mn < q.mx && q.mn < p.mx;
  }

    //--------------------------------------------------------------------------

  hp ssss;
  double sclx;
  double scly;

  PolygonIntersect() :ssss(0), sclx(0), scly(0){}

  void cntrib(int f_x, int f_y, int t_x, int t_y, int w) {
    ssss += (hp)w * (t_x - f_x) * (t_y + f_y) / 2;
  }

  void fit(const vector<Point2D> &x, int cx, Vertex ix[], int fudge, Box B)
  {
    int c = cx;
    while (c-- > 0) {
//       ix[c] = new Vertex();
//       ix[c].ip = new IPoint();
      ix[c].ip.x = ((int)((x[c][0] - B.min.x) * sclx - mid) & ~7)
        | fudge | (c & 1);
      ix[c].ip.y = ((int)((x[c][1] - B.min.y) * scly - mid) & ~7)
        | fudge;
    }

    ix[0].ip.y += cx & 1;
    ix[cx] = ix[0];

    c = cx;
    while (c-- > 0) {
      ix[c].rx = ix[c].ip.x < ix[c + 1].ip.x ?
        Rng(ix[c].ip.x, ix[c + 1].ip.x) :
        Rng(ix[c + 1].ip.x, ix[c].ip.x);
      ix[c].ry = ix[c].ip.y < ix[c + 1].ip.y ?
        Rng(ix[c].ip.y, ix[c + 1].ip.y) :
        Rng(ix[c + 1].ip.y, ix[c].ip.y);
      ix[c].in = 0;
    }
  }

  void cross(Vertex &a, Vertex &b, Vertex &c, Vertex &d,
             double a1, double a2, double a3, double a4)
  {
    double r1 = a1 / ((double) a1 + a2);
    double r2 = a3 / ((double) a3 + a4);

    cntrib((int)(a.ip.x + r1 * (b.ip.x - a.ip.x)),
           (int)(a.ip.y + r1 * (b.ip.y - a.ip.y)),
           b.ip.x, b.ip.y, 1);
    cntrib(d.ip.x, d.ip.y,
           (int)(c.ip.x + r2 * (d.ip.x - c.ip.x)),
           (int)(c.ip.y + r2 * (d.ip.y - c.ip.y)),
           1);
    ++a.in;
    --c.in;
  }

  void inness(Vertex P[], int cP, Vertex Q[], int cQ)
  {
    int s = 0;
    int c = cQ;
    IPoint p = P[0].ip;

    while (c-- > 0) {
      if (Q[c].rx.mn < p.x && p.x < Q[c].rx.mx) {
        bool sgn = 0 < area(p, Q[c].ip, Q[c + 1].ip);
        s += (sgn != (Q[c].ip.x < Q[c + 1].ip.x)) ? 0 : (sgn ? -1 : 1);
      }
    }
    for (int j = 0; j < cP; ++j) {
      if (s != 0)
        cntrib(P[j].ip.x, P[j].ip.y,
               P[j + 1].ip.x, P[j + 1].ip.y, s);
      s += P[j].in;
    }
  }

    //-------------------------------------------------------------------------

  double inter(const vector<Point2D> &a, const vector<Point2D> &b)
  {
    const int na = a.size();
    const int nb = b.size();
    Vertex* ipa = MANTA_STACK_ALLOC(Vertex, na + 1);
    Vertex* ipb = MANTA_STACK_ALLOC(Vertex, nb + 1);
    Box bbox = Box(Point(bigReal, bigReal),
                   Point(-bigReal, -bigReal));

    if (na < 3 || nb < 3)
      return 0;

    range(a, na, bbox);
    range(b, nb, bbox);

    double rngx = bbox.max.x - bbox.min.x;
    sclx = gamut / rngx;
    double rngy = bbox.max.y - bbox.min.y;
    scly = gamut / rngy;
    double ascale = sclx * scly;

    fit(a, na, ipa, 0, bbox);
    fit(b, nb, ipb, 2, bbox);

    for (int j = 0; j < na; ++j) {
      for (int k = 0; k < nb; ++k) {
        if (ovl(ipa[j].rx, ipb[k].rx) && ovl(ipa[j].ry, ipb[k].ry)) {
          hp a1 = -area(ipa[j].ip, ipb[k].ip, ipb[k + 1].ip);
          hp a2 = area(ipa[j + 1].ip, ipb[k].ip, ipb[k + 1].ip);
          bool o = a1 < 0;
          if (o == (a2 < 0)) {
            hp a3 = area(ipb[k].ip, ipa[j].ip, ipa[j + 1].ip);
            hp a4 = -area(ipb[k + 1].ip, ipa[j].ip,
                            ipa[j + 1].ip);
            if ((a3 < 0) == (a4 < 0)) {
              if (o)
                cross(ipa[j], ipa[j + 1], ipb[k], ipb[k + 1],
                      a1, a2, a3, a4);
              else
                cross(ipb[k], ipb[k + 1], ipa[j], ipa[j + 1],
                      a3, a4, a1, a2);
            }
          }
        }
      }
    }

    inness(ipa, na, ipb, nb);
    inness(ipb, nb, ipa, na);

    return ssss / ascale;
  }
};

#endif //aip_h
