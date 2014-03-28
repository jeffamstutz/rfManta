
/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005-2006
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

#include <Model/Materials/MaterialTable.h>


#include <Interface/RayPacket.h>
#include <Interface/Context.h>

using namespace Manta;

MaterialTable::~MaterialTable() {

  // Delete all of the owned materials.
  for (size_t i=0;i<material_table.size();++i) {
    if (material_table[i].material && material_table[i].owned) {
      delete material_table[i].material;
    }
  }
}

void MaterialTable::preprocess(const PreprocessContext& context) {

  // Call preprocess on each material.
  for (size_t i=0;i<material_table.size();++i) {
    if (material_table[i].material) {
      material_table[i].material->preprocess( context );
    }
  }
}

void MaterialTable::shade(const RenderContext& context, RayPacket& rays) const {

  // Determine material id's.
  Packet<int> id;
  id_texture->mapValues( id, context, rays );

  // Shade continuous subpackets of matching materials.
  for(int i = rays.begin();i<rays.end();){
    const Material *hit_matl = material_table[ id.data[i] ].material;
    int end = i+1;

    while(end < rays.end() && rays.wasHit(end) &&
          material_table[id.data[end]].material == hit_matl)
      end++;
    
    // Shade the subpacket of rays.
    RayPacket subPacket(rays, i, end);
    hit_matl->shade(context, subPacket);
    i=end;
    
  }
}

void MaterialTable::setMaterial(unsigned int material_id, Material *material, bool owned ) {
  // Check to see that the table size is correct.
  if (material_id >= material_table.size()) {
    material_table.resize( material_id+1, Entry( 0, false ) ); 
  }

  // Check to see if another owned material already occupies the spot.
  if (material_table[material_id].material &&
      material_table[material_id].owned) {
    delete material_table[material_id].material;
    material_table[material_id].material = 0;
  }

  // Lastly assign the pointer.
  material_table[material_id].material = material;
  material_table[material_id].owned = owned;
}

Material *MaterialTable::getMaterial( unsigned int material_id ) {

  return material_table[material_id].material;
}





