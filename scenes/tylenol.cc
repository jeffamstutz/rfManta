#include <Core/Color/ColorDB.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Groups/Group.h>
#include <Model/Lights/HeadLight.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Primitives/Sphere.h>

#include <iostream>
#include <map>
#include <string>

using namespace Manta;

class AtomNotFound{
public:
  AtomNotFound(const std::string& name) : name(name) {}
  std::string name;
};

struct atom{
  atom() : vdw_radius(0), r(0), g(0), b(0) {}

  atom(float vdw_radius, float r, float g, float b) :
    vdw_radius(vdw_radius), r(r), g(b), b(b) {}

  float vdw_radius; // van der waals radius
  float r, g, b; // CPK color
};

typedef std::map<std::string, atom> atom_table;
static atom_table atom_data;

Sphere *new_atom(const std::string& name, float x, float y, float z){
  // Place atom named by `name' at position (x,y,z), measured in any
  // units, but Angstroms for PDB database.
  atom_table::const_iterator i = atom_data.find(name);
  if(i == atom_data.end())
    throw AtomNotFound(name);

  const atom& A = i->second;

  return new Sphere(new Lambertian(Color(RGBColor(A.r,A.g,A.b))),
		    Vector(x,y,z), A.vdw_radius);
}

MANTA_PLUGINEXPORT
Scene *make_scene(const ReadContext&, const vector<string>& args)
{
  // Create atom data.  Radius data taken from
  // http://en.wikipedia.org/wiki/Van_der_Waals_radius, accessed
  // 2007/12/13.  Color data taken from
  // http://jmol.sourceforge.net/jscolors/, accessed same day.
  try{
    atom_data["C"] = atom(1.7, 0.565,0.565,0.565);
    atom_data["H"] = atom(1.20,1.000,1.000,1.000);
    atom_data["O"] = atom(1.52,1.000,0.051,0.051);
    atom_data["N"] = atom(1.55,0.188,0.314,0.973);
  }

  catch(AtomNotFound& anf){
    std::cerr << "error: atom `" << anf.name << "' not in atom database." << std::endl;
    return 0;
  }

  Group *group = new Group;

  // Molecule geometry data taken from
  // http://redpoll.pharmacy.ualberta.ca/drugbank/drugBank/drugPDB_unzipped/APRD00252_PDB.txt
  group->add(new_atom("C", 3.979, 0.149, 0.137));
  group->add(new_atom("C", 2.516, -0.124, -0.100));
  group->add(new_atom("O", 2.172, -1.145, -0.657));
  group->add(new_atom("N", 1.590, 0.767, 0.306));
  group->add(new_atom("C", 0.227, 0.462, 0.192));
  group->add(new_atom("C", -0.217, -0.833, 0.424));
  group->add(new_atom("C", -1.560, -1.132, 0.312));
  group->add(new_atom("C", -2.466, -0.140, -0.034));
  group->add(new_atom("C", -2.023, 1.154, -0.266));
  group->add(new_atom("C", -0.681, 1.456, -0.149));
  group->add(new_atom("O", -3.788, -0.436, -0.144));
  group->add(new_atom("H", 4.572, -0.677, -0.257));
  group->add(new_atom("H", 4.263, 1.073, -0.367));
  group->add(new_atom("H", 4.161, 0.249, 1.207));
  group->add(new_atom("H", 1.867, 1.619, 0.678));
  group->add(new_atom("H", 0.489, -1.605, 0.693));
  group->add(new_atom("H", -1.905, -2.140, 0.492));
  group->add(new_atom("H", -2.728, 1.925, -0.536));
  group->add(new_atom("H", -0.336, 2.463, -0.330));
  group->add(new_atom("H", -4.180, -0.295, 0.728));
	     
  Scene *scene = new Scene;
  scene->setBackground(new ConstantBackground(Color(RGBColor(0.6,0.6,0.6))));
  scene->setObject(group);

  LightSet *lights = new LightSet;
  //lights->add(new PointLight(Vector(0,10,0), Color(RGBColor(1,1,1))));
  lights->add(new HeadLight(3, Color(RGBColor(1,1,1))));
  lights->setAmbientLight(new ConstantAmbient(ColorDB::getNamedColor("black")));
  scene->setLights(lights);
  
  return scene;
}
