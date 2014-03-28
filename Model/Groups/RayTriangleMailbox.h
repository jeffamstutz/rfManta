/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institute, University of Utah

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

#ifndef __RAYTRIANGLEMAILBOX__H__
#define __RAYTRIANGLEMAILBOX__H__

namespace Manta {
  namespace Kdtree {

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    // This class implements a hash table based on triangle # and ray # to
    // determine if a ray has already intersected a certain triangle.
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    template< int SizePowerOf2 >
    class RayTriangleMailboxT {
    public:
            typedef Ray * RayNumber;
      typedef int   TriangleNumber;


    private:
      enum { Size      =  (1 << (SizePowerOf2)),
             TableMask = ~(0xffffffff << SizePowerOf2) };

      struct TableEntry {
        RayNumber      ray_number;
        TriangleNumber triangle_number;
      };
      
      TableEntry table[Size];

    public:
      // Explicity clear the table.
      void clear() { memset( table, 0x0, sizeof(TableEntry)*Size); }

      inline bool only_check( const RayNumber &ray_number, const TriangleNumber &triangle_number ) {

        unsigned int entry = (triangle_number & TableMask);
        bool un_mapped = (table[entry].triangle_number != triangle_number) ||
          (table[entry].ray_number != ray_number);

        return un_mapped;
      }
      
      // Check if the specified triangle number / ray number pair is already
      // mapped in the table. Set the mapping in either case.
      // Returns TRUE if the mapping is NOT set.
      inline bool not_mapped( const RayNumber &ray_number, const TriangleNumber &triangle_number ) {

        unsigned int entry = (triangle_number & TableMask);
        bool un_mapped = (table[entry].triangle_number != triangle_number) ||
          (table[entry].ray_number != ray_number);

        table[entry].ray_number      = ray_number;
        table[entry].triangle_number = triangle_number;        
        
        return un_mapped;
      }

#if 0
      inline void record( const RayNumber &ray_number, const TriangleNumber &triangle_number ) {

        unsigned int entry = (triangle_number & TableMask);

        table[entry].ray_number      = ray_number;
        table[entry].triangle_number = triangle_number;        
      }
#endif
    };

    typedef RayTriangleMailboxT<8> RayTriangleMailbox;
  };
};

#endif
