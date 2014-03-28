#include <Interface/Context.h>
#include <Core/Color/ColorDB.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/UnknownColor.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Interface/Context.h>
#include <Model/AmbientLights/AmbientOcclusionBackground.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/AmbientLights/EyeAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Backgrounds/LinearBackground.h>
#include <Model/Backgrounds/EnvMapBackground.h>
#include <Model/Groups/BSP/BSP.h>
#include <Model/Groups/CellSkipper.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/KDTree.h>
#include <Model/Groups/ObjGroup.h>
#include <Model/Groups/RecursiveGrid.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/Transparent.h>
#include <Model/Lights/PointLight.h>
#include <Model/Lights/AreaLight.h>
#include <Model/MiscObjects/KeyFrameAnimation.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Readers/PlyReader.h>
#include <Model/Readers/IW.h>
#include <Model/Readers/MReader.h>
#include <Model/Textures/Constant.h>
#include <Core/Math/MinMax.h>
#include <Model/Materials/Checker.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/Textures/ImageTexture.h>
#include <Model/TexCoordMappers/LinearMapper.h>


#include <string>
#include <vector>
#include <locale>
#include <algorithm>
#include <iostream>
#include <fstream>


#include "UsePrivateCode.h"
#ifdef USE_PRIVATE_CODE
#include <Model/Groups/private/CGT.h>
#endif


using namespace Manta;
using namespace std;

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
class Decompress {
  char *fifo_name;
  pid_t proc_id;
  public:
  Decompress(string &model, char const *args[]) {
    fifo_name = static_cast<char *>(malloc(model.size() + 7));
    strcpy(fifo_name, model.c_str());
    strcat(fifo_name, "XXXXXX");
    char* ptr = mktemp(fifo_name);
    if (!ptr)
      cerr << "error trying to decompress\n";
    mkfifo(fifo_name, 0666);
    proc_id = fork();
    if (proc_id == 0) {
      int file_des = open(fifo_name, O_WRONLY, 0666);
      dup2(file_des, 1);
      close(file_des);
      execvp(args[0], const_cast<char* const*>(args));
    }
  }
  const char *get_fifo() {
    return fifo_name;
  }
  ~Decompress() {
    waitpid(proc_id, 0, 0);
    unlink(fifo_name);
    free(fifo_name);
  }
};
#endif

void argumentError(int i, const vector<string>& args) {
  cerr << "Valid options for scene meshViewer:\n";
  cerr << " -BSP   - use BSP acceleration structure\n";
  cerr << " -DynBVH   - use DynBVH acceleration structure\n";
#ifdef USE_PRIVATE_CODE
  cerr << " -CGT      - use Coherent Grid Traversal acceleration structure\n";
#endif
  cerr << " -KDTree   - use KDTree acceleration structure\n";
  cerr << " -CellSkipper - use CellSkipper grid acceleration structure\n";
  cerr << " -rgrid    - use single ray recursive grid acceleration structure\n";
  cerr << " -model    - Required. The file to load (obj, ply, iw, or m file)\n";
  cerr << "             Can call this multiple times to load an animation.\n";
  cerr << " -save [filename]    - save acceleration structure to file (currently kdtree and bsp).\n";
  cerr << " -load [filename]    - load acceleration structure from file (currently kdtree and bsp).\n";
  cerr << " -saveOBJ [filename] - convert the mesh to an OBJ and MTL file (omit filename extension).\n";
  cerr << " -animationLength    - Number of seconds animation takes\n";
  cerr << " -interpolateNormals - creates vertex normals if the data does not already contain vertex normals.\n";
  cerr << " -useFaceNormals     - force to use only face normals\n";
  cerr << " -smoothAnimation    - interpolates between keyframes.\n";
  cerr << " -fixedAnimation     - interpolates between fixed steps over -animationLength frames.\n";
  cerr << " -noloop             - do not loop after an animation cycle.\n";
  cerr << " -triangleType       - Triangle implementation to use.\n"
    << "                       Wald_tri, KS_tri.\n";
  cerr << " -overrideMatl       - Force to use a material.\n"
    << "                       flat, eyelight, lambertian, phong, dielectric, transparent.\n";
  cerr << " -lightPosition      - Location of light.\n";
  cerr << " -ambient            - Type of ambient lighting to use.\n";
  cerr << "                     - arc, AO {bounce} {samples N}, constant, eye.\n";
  cerr << " -addFloor           - Adds a checkered floor.\n";
  cerr << " -background         - color R G B, image file, bgColor r g b, bgAngle radians, real (the current background) ";
  throw IllegalArgument("scene triangleSceneViewer", i, args);
}

Mesh* LoadModel(std::string modelName, Material* defaultMatl, Material *overrideMatl,
    MeshTriangle::TriangleType triangleType, bool useFaceNormals,
    bool interpolateNormals ) {
  Mesh* frame = NULL;
  if (!strncmp(modelName.c_str()+modelName.length()-4, ".ply", 4)) {
    frame = new Mesh;
    if (!readPlyFile(modelName, AffineTransform::createIdentity(), frame, defaultMatl, triangleType))
      printf("error loading or reading ply file: %s\n", modelName.c_str());
  }
  else if (modelName.length() > 4 && !strncmp(modelName.c_str()+modelName.length()-5, ".plyg", 5)) {
    frame = new Mesh;
    ifstream in(modelName.c_str());
    while (in) {
      string modelName;
      in >> modelName;
      if (modelName.length() > 4 && !strncmp(modelName.c_str()+modelName.length()-4, ".ply", 4)) {
        if (!readPlyFile(modelName, AffineTransform::createIdentity(), frame, defaultMatl, triangleType))
          printf("error loading or reading ply file: %s\n", modelName.c_str());
      }
#ifndef _WIN32
      else if (!strncmp(modelName.c_str()+modelName.length()-8, ".ply.bz2", 8)) {
        char const *args[] = {"bzip2", "-dc", modelName.c_str(), 0};
        Decompress decomp(modelName, args);
        frame = new Mesh;
        if (!readPlyFile(decomp.get_fifo(), AffineTransform::createIdentity(), frame, defaultMatl, triangleType))
          printf("error loading or reading ply file: %s\n", modelName.c_str());
      }
      else if (!strncmp(modelName.c_str()+modelName.length()-7, ".ply.gz", 7)) {
        char const *args[] = {"gzip", "-dc", modelName.c_str(), 0};
        Decompress decomp(modelName, args);
        frame = new Mesh;
        if (!readPlyFile(decomp.get_fifo(), AffineTransform::createIdentity(), frame, defaultMatl, triangleType))
          printf("error loading or reading ply file: %s\n", modelName.c_str());
      }
#endif
    }
  }
  else if (!strncmp(modelName.c_str()+modelName.length()-4, ".obj", 4)) {
    frame = new ObjGroup(modelName.c_str(), defaultMatl, triangleType);
  }
  else if (!strncmp(modelName.c_str()+modelName.length()-3, ".iw", 3)) {
    frame = readIW(modelName, triangleType);
  }
  else if  (!strncmp(modelName.c_str()+modelName.length()-2, ".m", 2)) {
    frame = readM(modelName, defaultMatl, triangleType);
  }
#ifndef _WIN32
  // NOTE(aek): Don't bother trying this with OBJ files.  The OBJ reader
  // library that we use makes two passes through the file using rewind(),
  // which doesn't play well with pipes.  Any readers that stream in a
  // single pass should work, however.
  else if (!strncmp(modelName.c_str()+modelName.length()-8, ".ply.bz2", 8)) {
    char const *args[] = {"bzip2", "-dc", modelName.c_str(), 0};
    Decompress decomp(modelName, args);
    frame = new Mesh;
    if (!readPlyFile(decomp.get_fifo(), AffineTransform::createIdentity(), frame, defaultMatl, triangleType))
      printf("error loading or reading ply file: %s\n", modelName.c_str());
  }
  else if (!strncmp(modelName.c_str()+modelName.length()-7, ".ply.gz", 7)) {
    char const *args[] = {"gzip", "-dc", modelName.c_str(), 0};
    Decompress decomp(modelName, args);
    frame = new Mesh;
    if (!readPlyFile(decomp.get_fifo(), AffineTransform::createIdentity(), frame, defaultMatl, triangleType))
      printf("error loading or reading ply file: %s\n", modelName.c_str());
  }
  else if (!strncmp(modelName.c_str()+modelName.length()-7, ".iw.bz2", 7)) {
    char const *args[] = {"bzip2", "-dc", modelName.c_str(), 0};
    Decompress decomp(modelName, args);
    frame = readIW(decomp.get_fifo(), triangleType);
  }
  else if (!strncmp(modelName.c_str()+modelName.length()-6, ".iw.gz", 6)) {
    char const *args[] = {"gzip", "-dc", modelName.c_str(), 0};
    Decompress decomp(modelName, args);
    frame = readIW(decomp.get_fifo(), triangleType);
  }
  else if (!strncmp(modelName.c_str()+modelName.length()-6, ".m.bz2", 6)) {
    char const *args[] = {"bzip2", "-dc", modelName.c_str(), 0};
    Decompress decomp(modelName, args);
    frame = readM(decomp.get_fifo(), defaultMatl, triangleType);
  }
  else if (!strncmp(modelName.c_str()+modelName.length()-5, ".m.gz", 5)) {
    char const *args[] = {"gzip", "-dc", modelName.c_str(), 0};
    Decompress decomp(modelName, args);
    frame = readM(decomp.get_fifo(), defaultMatl, triangleType);
  }
#endif

  if (overrideMatl) {
    frame->materials[0] = overrideMatl;
    for (size_t i = 0; i < frame->face_material.size(); i++) {
      frame->face_material[i] = 0;
    }
  }

  // NOTE(boulos): This code seems a bit strange, but what it's doing
  // is if you ask it to use the face normals it discards them from
  // the mesh and then if you ask it to build smooth normals, it does
  // so (but only if they're not there already). It seems like we
  // should throw some errors for some of these combinations...
  // NOTE(thiago): calling useFaceNormals and interpolateNormals at the
  // same time is the only contradictory option.
  if (useFaceNormals) {
    frame->discardVertexNormals();
  }
  if (interpolateNormals && !frame->hasVertexNormals())
    frame->interpolateNormals();

  return frame;
}

Material* stringToMaterial(string matlName) {
  Material* result = 0;
  if (matlName == "flat" ||
      matlName == "eyelight") {
    result = new Flat(Color::white() * 0.8);
  } else if (matlName == "lambert" ||
      matlName == "lambertian" ||
      matlName == "diffuse") {
    result = new Lambertian(Color::white() * 0.8);
  } else if (matlName == "phong") {
    result = new Phong(Color::white() * 0.8,
        Color::white()* 0.9,
        32, 1.f);
  } else if (matlName == "dielectric" ||
      matlName == "glass") {
    result = new Dielectric(1.4, 1.0, Color(RGB(.80, .89, .75)));
  } else if (matlName == "transparent"){
    result = new Transparent(Color(RGB(.50, .89, .45)), 0.5);
  }
  return result;
}

AmbientLight* stringToAmbientLight(string ambientName) {
  AmbientLight* result = NULL;
  if (ambientName == "arc") {
    Color cup(RGB(0.2, 0.2, 0.2));
    Color cdown(RGB(0.1, 0.1, 0.1));
    Vector up(0,1,0);
    result = new ArcAmbient(cup, cdown, up);
  }
  else if (ambientName == "ao") {
    result = new AmbientOcclusionBackground(Color::white() * 0.5, 1, 36);
  }
  else if (ambientName == "constant") {
    Color ambientColor(RGB(.3, .3, .3));
    result = new ConstantAmbient(ambientColor);
  }
  else if (ambientName == "eye") {
    Color ambientColor(RGB(.2, .2, .2));
    result = new EyeAmbient(ambientColor);
  }

  return result;
}

void writeMeshOBJ(Mesh* mesh, string& filename) {
  cerr << "Writing mesh to OBJ/MTL..." << endl;
  string obj_name = filename+".obj";
  string mtl_name = filename+".mtl";
  FILE* output = fopen( obj_name.c_str(), "w" );
  fprintf( output, "mtllib %s\n\n", mtl_name.c_str() );
  for (size_t i = 0; i < mesh->vertices.size(); ++i)
    fprintf(output, "v %f %f %f\n", mesh->vertices[i][0], mesh->vertices[i][1], mesh->vertices[i][2]);
  fprintf(output, "\n");
  for (size_t i = 0; i < mesh->texCoords.size(); ++i)
    fprintf(output, "vt %f %f %f\n", mesh->texCoords[i][0], mesh->texCoords[i][1], mesh->texCoords[i][2]);
  fprintf(output, "\n");
  for (size_t i = 0; i < mesh->vertexNormals.size(); ++i)
    fprintf(output, "vn %f %f %f\n", mesh->vertexNormals[i][0], mesh->vertexNormals[i][1], mesh->vertexNormals[i][2]);
  fprintf(output, "\n");
  int last_mtl = -1;
  for (size_t i = 0; i < mesh->vertex_indices.size(); i+=3) {
    int mtl = mesh->face_material[i/3];
    if (mtl != last_mtl) {
      fprintf(output, "usemtl mtl_%d\n", mtl);
      last_mtl = mtl;
    }
    if (mesh->texture_indices.size() < i+3 ||
        mesh->texture_indices[i+0] == Mesh::kNoTextureIndex ||
        mesh->texture_indices[i+1] == Mesh::kNoTextureIndex ||
        mesh->texture_indices[i+2] == Mesh::kNoTextureIndex)
      if (mesh->normal_indices.size() < i+3)
        fprintf(output, "f %d %d %d\n",
            mesh->vertex_indices[i+0]+1,
            mesh->vertex_indices[i+1]+1,
            mesh->vertex_indices[i+2]+1);
      else
        fprintf(output, "f %d//%d %d//%d %d//%d\n",
            mesh->vertex_indices[i+0]+1,
            mesh->normal_indices[i+0]+1,
            mesh->vertex_indices[i+1]+1,
            mesh->normal_indices[i+1]+1,
            mesh->vertex_indices[i+2]+1,
            mesh->normal_indices[i+2]+1);
    else if (mesh->normal_indices.size() < i+3)
      fprintf(output, "f %d/%d %d/%d %d/%d\n",
          mesh->vertex_indices[i+0]+1,
          mesh->texture_indices[i+0]+1,
          mesh->vertex_indices[i+1]+1,
          mesh->texture_indices[i+1]+1,
          mesh->vertex_indices[i+2]+1,
          mesh->texture_indices[i+2]+1);
    else
      fprintf(output, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
          mesh->vertex_indices[i+0]+1,
          mesh->texture_indices[i+0]+1,
          mesh->normal_indices[i+0]+1,
          mesh->vertex_indices[i+1]+1,
          mesh->texture_indices[i+1]+1,
          mesh->normal_indices[i+1]+1,
          mesh->vertex_indices[i+2]+1,
          mesh->texture_indices[i+2]+1,
          mesh->normal_indices[i+2]+1);
  }
  fprintf(output, "\n");
  fclose(output);
  output = fopen(mtl_name.c_str(), "w");
  for (size_t i = 0; i < mesh->materials.size(); ++i) {
    // TODO: Handle other material types, such as Phong.
    Lambertian* mat = dynamic_cast<Lambertian*>(mesh->materials[i]);
    const Constant<Color>* con = mat ? dynamic_cast<const Constant<Color>*>(mat->getDiffuse()) : 0;
    Color rgb = con ? con->getValue() : Color(RGB(1,1,1));
    fprintf(output, "newmtl mtl_%d\n", static_cast<int>(i));
    fprintf(output, "Ka 0 0 0\n");
    fprintf(output, "Kd %f %f %f\n", rgb[0], rgb[1], rgb[2]);
    fprintf(output, "Ks 0 0 0\n");
    fprintf(output, "illum 2\n");
    fprintf(output, "Ns 0\n");
  }
  fclose(output);
}

  MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
  std::cout << "Make_scene args: " << args.size() << std::endl;

  vector<string> fileNames;
  AccelerationStructure *as = new DynBVH();
  bool interpolateNormals = false;
  bool useFaceNormals = false;
  bool smoothAnimation = false;
  bool fixedAnimation = false;
  bool noLoop = false;
  float animationLength = 5; //in seconds
  Material* overrideMatl = 0;
  Material* defaultMatl  = 0;
  bool setModel = false;
  bool setLight = false;
  bool addFloor = false;
  Vector lightPosition;
  Color lightColor = Color::white();
  MeshTriangle::TriangleType triangleType = MeshTriangle::KENSLER_SHIRLEY_TRI;
  Background* background = NULL;

  string saveName;
  string loadName;
  string saveOBJName;

  string ambientName = "eye";
  AmbientLight* ambient = stringToAmbientLight(ambientName);;

  for(size_t i=0;i<args.size();i++){
    string arg = args[i];
    if(arg == "-model"){
      fileNames.push_back("");
      if(!getStringArg(i, args, fileNames.back()))
        throw IllegalArgument("scene MeshLoader -model", i, args);
      setModel = true;
    } else if (arg == "-BSP") {
      delete as;
      as = new BSP;
    } else if (arg == "-DynBVH") {
      delete as;
      as = new DynBVH;
    } else if (arg == "-KDTree") {
      delete as;
      as = new KDTree;
    } else if (arg == "-CellSkipper") {
      delete as;
      as = new CellSkipper;
    } else if (arg == "-rgrid") {
      delete as;
      as = new RecursiveGrid;
    } else if (arg == "-CGT") {
#ifdef USE_PRIVATE_CODE
      delete as;
      as = new Grid;
#else
      throw IllegalArgument("CGT is not available to you.", i, args);
#endif
    } else if(arg == "-save"){
      if (!getStringArg(i, args, saveName))
        throw IllegalArgument("wrong argument to -save", i, args);
    } else if(arg == "-load"){
      if (!getStringArg(i, args, loadName))
        throw IllegalArgument("wrong argument to -load", i, args);
    } else if(arg == "-saveOBJ"){
      if (!getStringArg(i, args, saveOBJName))
        throw IllegalArgument("wrong argument to -saveOBJ", i, args);
    } else if (arg == "-animationLength") {
      if(!getArg<float>(i, args, animationLength))
        throw IllegalArgument("scene MeshLoader -animationLength", i, args);
    } else if (arg == "-interpolateNormals") {
      interpolateNormals = true;
    } else if (arg == "-useFaceNormals") {
      useFaceNormals = true;
    } else if (arg == "-smoothAnimation") {
      smoothAnimation = true;
    } else if (arg == "-fixedAnimation") {
      fixedAnimation = true;
    } else if (arg == "-addFloor") {
      addFloor = true;
    } else if (arg == "-background") {
      if (args[i+1] == "image") {
        i++;
        string filename;
        if (!getStringArg(i, args, filename))
          throw IllegalArgument("wrong argument to -background image", i, args);
        EnvMapBackground::MappingType mapping_type = EnvMapBackground::DebevecSphere;
        ImageTexture<Color>* t = LoadColorImageTexture(filename, &std::cerr);
        t->setInterpolationMethod(ImageTexture<Color>::Bilinear);
        Vector up(0,1,0), right(1,0,0);
        background = new EnvMapBackground(t, mapping_type, right, up);

      } else
      {
        Vector colorv;
        if (!getVectorArg(i, args, colorv))
          throw IllegalArgument("wrong argument to -bakckground r g b", i, args);
        background = new ConstantBackground(Color(RGB(colorv[0], colorv[1], colorv[2])));
      }
    } else if (arg == "-noLoop") {
      noLoop = true;
    } else if (arg == "-triangleType") {
      string triangleName;
      if (!getStringArg(i, args, triangleName))
        throw IllegalArgument("scene triangleSceneViewer -triangleType", i, args);
      if (triangleName == "Wald_tri")
        triangleType = MeshTriangle::WALD_TRI;
      else if (triangleName == "KS_tri")
        triangleType = MeshTriangle::KENSLER_SHIRLEY_TRI;
      else
        throw IllegalArgument("scene triangleSceneViewer -triangleType", i, args);
    } else if (arg == "-overrideMatl") {
      string matlName;
      if (!getStringArg(i, args, matlName))
        throw IllegalArgument("scene triangleSceneViewer -overrideMatl", i, args);

      std::transform(matlName.begin(), matlName.end(), matlName.begin(), ::tolower);
      overrideMatl = stringToMaterial(matlName);
      if (0 == overrideMatl)
        throw IllegalArgument("scene triangleSceneViewer -overrideMatl unknown material", i, args);
    } else if (arg == "-defaultMatl") {
      string matlName;
      if (!getStringArg(i, args, matlName))
        throw IllegalArgument("scene triangleSceneViewer -defaultMatl", i, args);

      std::transform(matlName.begin(), matlName.end(), matlName.begin(), ::tolower);
      defaultMatl = stringToMaterial(matlName);
      if (0 == defaultMatl)
        throw IllegalArgument("scene triangleSceneViewer -defaultMatl unknown material", i, args);
    } else if (arg == "-lightPosition") {
      if (!getVectorArg(i, args, lightPosition))
        throw IllegalArgument("scene triangleSceneViewer -lightPosition", i, args);
      setLight = true;
    } else if (arg == "-lightColor") {
      Vector lc;
      if (!getVectorArg(i, args, lc))
        throw IllegalArgument("scene triangleSceneViewer -lightColor", i, args);
      lightColor = Color(RGB(lc[0], lc[1], lc[2]));
    } else if (arg == "-ambient") {
      if (!getStringArg(i, args, ambientName))
        throw IllegalArgument("scene triangleSceneViewer -ambient", i, args);

      std::transform(ambientName.begin(), ambientName.end(), ambientName.begin(), ::tolower);

      if (ambient) delete ambient;
      ambient = stringToAmbientLight(ambientName);

      if (ambient == NULL)
        throw IllegalArgument("scene triangleSceneViewer -ambient unknown ambient model", i, args);

      if (ambientName == "ao") {
        bool bounce = false;
        int samples = 36;
        double cutoff = 10;
        Background* ao_bg = NULL;
        Color bgColor(RGB(0.4,0.4,0.4));
        double bgAngle = 2.0*Pi;
        while (i+1 < args.size()) {
          if (args[i+1] == "bounce") {
            ++i;
            bounce = true;
          }
          else if (args[i+1] == "samples") {
            ++i;
            if (!getIntArg(i, args, samples))
              throw IllegalArgument("scene triangleSceneViewer -ambient ao samples", i, args);
          }
          else if (args[i+1] == "cutoff") {
            ++i;
            if (!getDoubleArg(i, args, cutoff))
              throw IllegalArgument("scene triangleSceneViewer -ambient ao cutoff", i, args);
          }
          else if (args[i+1] == "background")
          {
            i++;
            if (args[i+1] == "image") {
              i++;
              string filename;
              if (!getStringArg(i, args, filename))
                throw IllegalArgument("wrong argument to -background image", i, args);
              EnvMapBackground::MappingType mapping_type = EnvMapBackground::DebevecSphere;
              ImageTexture<Color>* t = LoadColorImageTexture(filename, &std::cerr);
              t->setInterpolationMethod(ImageTexture<Color>::Bilinear);
              Vector up(0,1,0), right(1,0,0);
              ao_bg = new EnvMapBackground(t, mapping_type, right, up);

            } else if (args[i+1] == "real")
            {
              i++;
              ao_bg = background;
            }
          }
          else if (args[i+1] == "bgColor")
          {
            i++;
            Vector c;
            if (!getVectorArg(i, args, c))
              throw IllegalArgument("wrong argument to -ambient ao bgColor r g b", i, args);
            bgColor = Color(RGB(c[0], c[1], c[2]));
          }
          else if (args[i+1] == "bgAngle")
          {
            i++;
            if (!getDoubleArg(i, args, bgAngle))
              throw IllegalArgument("scene triangleSceneViewer -ambient ao bgAngle", i, args);
          }
          else break;
        }
        AmbientOcclusionBackground *ao = static_cast<AmbientOcclusionBackground*>(ambient);
        ao->setSamplingAngle(bgAngle);
        ao->setNumRays(samples);
        ao->gatherColor(bounce);
        ao->setCutoffDistance(cutoff);
        ao->setBackground(ao_bg);
        ao->setBackgroundColor(bgColor);
      }

    } else {
      argumentError(i, args);
    }
  }

  if (args.empty() || !setModel)
    argumentError(0, args);

  Group* group = new Group();

  string modelName = fileNames[0];
  if (!strncmp(modelName.c_str()+modelName.length()-5, ".anim", 5)) {
    fileNames.clear();
    //read in data from modelName and
    ifstream in(modelName.c_str());
    while (in >> modelName)
      fileNames.push_back(modelName);
  }

  if (fileNames.size() > 1) {
    KeyFrameAnimation *animation = new KeyFrameAnimation();
    if (smoothAnimation)
      animation->setInterpolation(KeyFrameAnimation::linear);
    if (fixedAnimation)
      animation->setInterpolation(KeyFrameAnimation::fixed);

    group->add(animation);
    animation->useAccelerationStructure(as);

    for (size_t i=0; i < fileNames.size(); ++i) {
      modelName = fileNames[i];
      cout << "loading " << modelName <<endl;
      Mesh* frame = LoadModel(modelName, defaultMatl, overrideMatl, triangleType,
          useFaceNormals, interpolateNormals);
      animation->push_back(frame);
    }

    animation->lockFrames(false);
    animation->setDuration(animationLength);
    animation->loopAnimation(!noLoop);
    animation->startAnimation();
  } else {
    // If we're just a single mesh, load it directly instead of using
    // the animation class.
    Mesh* singleFrame = LoadModel(fileNames[0], defaultMatl, overrideMatl, triangleType,
        useFaceNormals, interpolateNormals);
    as->setGroup(singleFrame);

    if (!saveOBJName.empty())
      writeMeshOBJ(singleFrame, saveOBJName);

    group->add(as);
  }

  if (!loadName.empty()) {
    bool success = as->buildFromFile(loadName);
    if (!success) {
      cerr << "AccelerationStructure had trouble loading " << loadName<<endl;
      cerr << "Building it from scratch instead\n";
    }
  }

  if (!saveName.empty()) {
    as->saveToFile(saveName);
  }

  Scene* scene = new Scene();
  //   scene->setBackground(new LinearBackground(Color(RGB(0.2, 0.4, 0.9)),
  //                                             Color(RGB(0.9,0.9,0.9)),
  //                                             Vector(0,1,0)));

  if (addFloor)
  {
    Texture<Color>* tile_tex = new CheckerTexture<Color>(Color::white()*.8, Color(RGB(.2,.3,.5)),
        Vector(4,0,0), Vector(0,4,0));
    //Material* tile = new Phong( tile_tex, new Constant<Color>(Color::white()*.8), 32, new Constant<ColorComponent>(.2) );
    Material* tile = new Lambertian(tile_tex);
    PreprocessContext context;

    BBox bounds;
    group->preprocess(context);
    group->computeBounds(context, bounds);

    Vector anchor = Vector(bounds.getMin().x()*2.0, bounds.getMin().y(), bounds.getMin().z()*2.0);
    Vector v1 = Vector(bounds.getMax().x() - bounds.getMin().x(), 0, 0)*2.0;
    Vector v2 = Vector(0,0,bounds.getMax().z() - bounds.getMin().z())*2.0;
    Primitive* floor = new Parallelogram(tile, anchor, v1, v2);

    floor->setTexCoordMapper( new LinearMapper( anchor, Vector(v1.x()/3.0,0,0), Vector(0, 0, v1.x()/3.0), Vector(0,1,0)) );

    group->add(floor);
  }


  if (!background)
    background = new ConstantBackground(Color(RGB(0.1, 0.1, 0.1)));
  scene->setBackground(background);

  scene->setObject(group);

  LightSet* lights = new LightSet();
#if 0
  Color area_light_color = Color(RGB(1,1,1))*20;
  Real areaLightDim = bbox.diagonal().length()*.30;

  Parallelogram* area_light_geometry =
    new Parallelogram(new Lambertian(area_light_color),
        lightOrigin, Vector(areaLightDim,0,0),
        Vector(0,0,areaLightDim));
  //   group->add(area_light_geometry);

  lights->add(new AreaLight(area_light_geometry, area_light_color));
#else
  if (!setLight) {
    BBox bbox;
    PreprocessContext dummyContext;
    group->preprocess(dummyContext); // need to preprocess before computing bounds
    group->computeBounds(dummyContext, bbox);
    lightPosition = bbox[1] + bbox.diagonal()*.3;
  }
  lights->add(new PointLight(lightPosition, lightColor));
#endif



  lights->setAmbientLight(ambient);
  scene->setLights(lights);
  return scene;
}
