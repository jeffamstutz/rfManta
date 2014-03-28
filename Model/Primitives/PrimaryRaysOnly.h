/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

  License for the specific language governing rights and limitations under
  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifndef Manta_Model_PrimaryRaysOnly_h
#define Manta_Model_PrimaryRaysOnly_h

#include <Interface/Primitive.h>

namespace Manta
{

/*! 
\class PrimaryRaysOnly
\author Carson
\brief only intersects with eyerays
*/
class PrimaryRaysOnly : public Primitive
{
public:
    //! constructor
    /*!
        \param primitive to use for all functions, intersection method only called with eye rays
    */
    PrimaryRaysOnly(Primitive* p) { prim = p; }
    
    virtual void preprocess(const PreprocessContext& c) { prim->preprocess(c); }
    //! intersection method (uses primitive from constructor)
    /*!
        \post calls intersect method of primitive if rays are from eye
    */
    virtual void intersect(const RenderContext& context,
                           RayPacket& rays) const;
    
    virtual void computeNormal(const RenderContext& context,
                               RayPacket& rays) const { prim->computeNormal(context, rays); }
    virtual void computeGeometricNormal(const RenderContext& context,
                                        RayPacket& rays) const { prim->computeGeometricNormal(context, rays); }

    // Compute dPdu and dPdv (aka tangent vectors) - these are not
    // normalized. Cross(dPdu, dPdv) = k*N where k is some scale
    // factor. By default, we'll just construct a coordinate frame
    // around the Normal so that the invariant is satisfied.
    virtual void computeSurfaceDerivatives(const RenderContext& context,
                                           RayPacket& rays) const { prim->computeSurfaceDerivatives(context, rays); }

    virtual void setTexCoordMapper(const TexCoordMapper* new_tex) { prim->setTexCoordMapper(new_tex); };

    virtual void getRandomPoints(Packet<Vector>& points,
                                 Packet<Vector>& normals,
                                 Packet<Real>& pdfs,
                                 const RenderContext& context,
                                 RayPacket& rays) {  prim->getRandomPoints(points, normals,pdfs,context,rays); }
                                 
    virtual void computeBounds(const Manta::PreprocessContext& c, Manta::BBox& b) const { prim->computeBounds(c,b); }
                                 
protected:
    Primitive* prim;
};

}

#endif

