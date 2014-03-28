
#include <Core/Color/ColorDB.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/UnknownColor.h>
#include <Core/Math/MinMax.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Image/TGAFile.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Backgrounds/LinearBackground.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <Model/Groups/ObjGroup.h>
#include <Model/Instances/Instance.h>
#include <Model/Instances/InstanceRST.h>
#include <Model/Instances/InstanceRT.h>
#include <Model/Instances/InstanceST.h>
#include <Model/Instances/InstanceT.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Checker.h>
#include <Model/Materials/CopyTextureMaterial.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/NullMaterial.h>
#include <Model/Materials/Phong.h>
#include <Model/MiscObjects/Difference.h>
#include <Model/MiscObjects/Intersection.h>
#include <Model/MiscObjects/KeyFrameAnimation.h>
#include <Model/Primitives/Cone.h>
#include <Model/Primitives/Cube.h>
#include <Model/Primitives/Disk.h>
#include <Model/Primitives/Heightfield.h>
#include <Model/Primitives/Hemisphere.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/QuadFacedHexahedron.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Primitives/SuperEllipsoid.h>
#include <Model/Primitives/Torus.h>
#include <Model/Primitives/WaldTriangle.h>
#include <Model/Readers/PlyReader.h>
#include <Model/TexCoordMappers/LinearMapper.h>
#include <Model/TexCoordMappers/SphericalMapper.h>
#include <Model/TexCoordMappers/UniformMapper.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/ImageTexture.h>
#include <Model/Textures/MarbleTexture.h>
#include <Model/Textures/NormalTexture.h>
#include <Model/Textures/OakTexture.h>
#include <Model/Textures/WoodTexture.h>

#include <string>
#include <vector>
#include <iostream>

#include "UsePrivateCode.h"
#ifdef USE_PRIVATE_CODE
#include <Model/Groups/private/CGT.h>
#include <fstream>
#endif


using namespace Manta;
using namespace std;

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
  std::cout << "Make_scene args: " << args.size() << std::endl;

  Real scale = 1;
  Real texscale = 20;
  int numx = 8;
  int numy = 8;
  string primtype = "sphere";
  string material = "default";
  string texture = "default";
  string arraytype = "spin";
  string modelName = "";
  string imageName = "texture.tga";
  bool set_primtype = false;
  bool bgpoly = true;
  bool bgpoly_first = false;
  for(size_t i=0;i<args.size();i++){
    string arg = args[i];
    if(arg == "-scale"){
      if(!getArg<Real>(i, args, scale))
        throw IllegalArgument("scene primtest -scale", i, args);
    } else if(arg == "-texscale"){
      if(!getArg<Real>(i, args, texscale))
        throw IllegalArgument("scene primtest -texscale", i, args);
    } else if(arg == "-num"){
      if(!getResolutionArg(i, args, numx, numy))
        throw IllegalArgument("scene primtest -num", i, args);
    } else if(arg == "-material"){
      if(!getStringArg(i, args, material))
        throw IllegalArgument("scene primtest -material", i, args);
    } else if(arg == "-texture"){
      if(!getStringArg(i, args, texture))
        throw IllegalArgument("scene primtest -texture", i, args);
    } else if(arg == "-array"){
      if(!getStringArg(i, args, arraytype))
        throw IllegalArgument("scene primtest -array", i, args);
    } else if(arg == "-model"){
      if(!getStringArg(i, args, modelName))
        throw IllegalArgument("scene primtest -model", i, args);
    } else if(arg == "-image"){
      if(!getStringArg(i, args, imageName))
        throw IllegalArgument("scene primtest -image", i, args);
    } else if(arg == "-nobgpoly"){
      bgpoly = false;
    } else if(arg == "-bgpolyfirst"){
      bgpoly_first = true;
    } else {
      if(arg[0] == '-' || set_primtype) {
        cerr << "Valid options for scene primtest:\n";
        cerr << " -scale - sets size of primitives\n";
        cerr << " -num MxN - sets array of primitives to M by N\n";
        throw IllegalArgument("scene primtest", i, args);
      } else {
        primtype = arg;
        set_primtype = true;
      }
    }
  }

  Group* group = new Group();
  int max = Max(numx, numy);


  if(bgpoly && bgpoly_first){
    Texture<Color>* tex = new CheckerTexture<Color>(Color(RGB(0.4, 0.4, 0.4)),
                                                    Color(RGB(0.2, 0.2, 0.2)),
                                                    Vector(1,0,0)*numx*2,
                                                    Vector(0,1,0)*numy*2);
    //    Material* bgmatl = new Lambertian(tex);
    Material* bgmatl = new Phong(tex,
                                 new Constant<Color>(Color(RGB(0.9, 0.9, 0.9))),
                                 32, NULL);

    Vector anchor(-scale-1./max, -scale-1./max, -1.5/max);
    Vector v1(scale*2+2./max, 0, 0);
    Vector v2(0, scale*2+2./max, 0);
#if 0
    group->add(new Triangle(bgmatl, anchor, anchor+v1, anchor+v2));
    group->add(new Triangle(bgmatl, anchor+v1+v2, anchor+v2, anchor+v1));
#else
    group->add(new Parallelogram(bgmatl, anchor, v1, v2));
#endif
  }



  Material* matl;
  TexCoordMapper* mapr = 0;

  if (texture == "default") {
    if(material == "redphong" || material == "default")
      matl=new Phong(Color(RGB(.6,0,0)), Color(RGB(.6,.6,.6)),
                     32, (ColorComponent)0.4);
    else if(material == "redlambertian")
      matl=new Lambertian(Color(RGB(.6,0,0)));
    else if(material == "null")
      matl=new NullMaterial();
    else if(material == "metal")
      matl = new MetalMaterial(Color(RGB(0.7,0.7,0.8)));
    else if(material == "checker")
      matl = new Phong(new CheckerTexture<Color>(Color(RGB(.6,.6,.6)),
                                                 Color(RGB(.6,0,0)),
                                                 Vector(1,0,0)*texscale,
                                                 Vector(0,1,0)*texscale),
                       new Constant<Color>(Color(RGB(.6,.6,.6))),
                       32,
                       new Constant<ColorComponent>(0));
    else if(material == "checker2")
      matl = new Phong(new CheckerTexture<Color>(Color(RGB(.6,.6,.6)),
                                                 Color(RGB(.6,0,0)),
                                                 Vector(1,0,0)*texscale,
                                                 Vector(0,1,0)*texscale),
                       new Constant<Color>(Color(RGB(.6,.6,.6))),
                       32,
                       new CheckerTexture<ColorComponent>
                       ((ColorComponent)0.2,
                        (ColorComponent)0.5,
                        Vector(1,0,0)*texscale,
                        Vector(0,1,0)*texscale));
    else if(material == "checker3")
      matl = new Checker(new Phong(Color(RGB(.6,.6,.6)), Color(RGB(.6,.6,.6)), 32, (ColorComponent)0.2),
                         new Phong(Color(RGB(.6,0,0)), Color(RGB(.6,.6,.6)), 32, (ColorComponent)0.5),
                         Vector(1,0,0)*texscale, Vector(0,1,0)*texscale);
    else if(material == "marble")
      {
        matl = new Phong(
                         new MarbleTexture<Color>(
                                                  Color(RGB(0.1,0.2,0.5)), Color(RGB(0.7,0.8,1.0)),
                                                  10.0, 1.0, 15.0, 6, 2.0, 0.6 ),
                         new Constant<Color>(Color(RGB(.6,.6,.6))),
                         32,
                         new Constant<ColorComponent>(0));
        mapr = new UniformMapper();
      }
    else if(material == "wood")
      {
        matl = new Lambertian(
                              new WoodTexture<Color>(
                                                     Color(RGB(0.32,0.25,0.21)), Color(RGB(0.41,0.35,0.3)),
                                                     12.0, 20.0, 5.0, 5.0, 6, 2.0, 0.6 ) );
        mapr = new UniformMapper();
      }
    else if(material == "oak")
      {
        matl = new Lambertian(
                              new OakTexture<Color>(
                                                    Color(RGB(0.15,0.077,0.028)), Color(RGB(0.5,0.2,0.067)),
                                                    64.0, 0.5, 200.0, 0.02, 1.0, 0.3, 0.4, 1.0, 2.0, 1.0, 0.4 ) );
        mapr = new UniformMapper();
      }
    else if(material == "image")
      {
        Image *img = readTGA( imageName );
        matl = new Lambertian( new ImageTexture<Color>( img ) );
        mapr = new UniformMapper();
      }
    else if(material == "dielectric")
      {
        matl = new Dielectric(1.6, 1.0, Color(RGB(.9, .8, .8)));
      }
    else
      throw IllegalArgument("Unknown material type for primtest: "+material, 0, args);
  } // end if (texture == "default")
  else {
    Texture<Color>* tex = NULL;
    if (texture == "normal") {
      tex = new NormalTexture();
    } else {
      try {
        if (texture == "default")
          texture = "red4";
        Color color = ColorDB::getNamedColor(texture);
        tex = new Constant<Color>(color);
      } catch (UnknownColor& e) {
        throw IllegalArgument("Unknown color name for texture: "+texture, 0, args);
      }
    }
    if (material == "copy") {
      matl = new CopyTextureMaterial(tex);
    } else if (material == "flat") {
      matl = new Flat(tex);
    } else if (material == "phong" || material == "default") {
      matl=new Phong(tex, new Constant<Color>(Color(RGB(.6,.6,.6))),
                     32, new Constant<ColorComponent>((ColorComponent)0.4));
    } else if (material == "lambertian") {
      matl = new Lambertian( tex );
    } else {
      throw IllegalArgument("Unknown material type: "+material, 0, args);
    }
  }


  Object* spinprim = 0;
  if(primtype == "simplesphere"){
    for(int i=0;i<numx;i++){
      for(int j=0;j<numy;j++){
        int idx = j*numx+i;
        Real radius = (idx+1)/((numx*numy)*scale/max);
        Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                 (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                 0);
        Primitive* prim = new Sphere( matl, p, radius );
        if ( mapr )
          prim->setTexCoordMapper( mapr );
        group->add( prim );
      }
    }
  } else if (primtype == "simplesuperellipsoid"){
    for(int i=0;i<numx;i++){
      for(int j=0;j<numy;j++){
        int idx = j*numx+i;
        Real radius = (idx+1)/((numx*numy)*scale/max);
        Real alpha = (i+1)/static_cast<Real>(numx)*2;
        Real beta = (j+1)/static_cast<Real>(numy)*2;
        Vector p((i/static_cast<Real>(numx-1) - (Real)0.5)*scale*2,
                 (j/static_cast<Real>(numy-1) - (Real)0.5)*scale*2,
                 0);
        Primitive* prim = new SuperEllipsoid( matl, p, radius, alpha, beta );
        if ( mapr )
          prim->setTexCoordMapper( mapr );
        group->add( prim );
      }
    }
  } else if(primtype == "simplebox"){
    Vector p2(scale/max, scale/max, scale/max);
    for(int i=0;i<numx;i++){
      for(int j=0;j<numy;j++){
        Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                 (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                 0);
        Primitive* prim = new Cube( matl, p-p2, p+p2);
        if ( mapr )
          prim->setTexCoordMapper( mapr );
        group->add( prim );
      }
    }
  } else if(primtype == "simplehex"){
    Real x = scale/max;

    for(int i=0;i<numx;i++){
      for(int j=0;j<numy;j++){
        Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                 (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                 0);
        Primitive* prim = new QuadFacedHexahedron(matl,
                                                  p + Vector(-1.5*x, -1.5*x, -x),
                                                  p + Vector( 1.5*x, -1.5*x, -x),
                                                  p + Vector( 1.5*x,  1.5*x, -x),
                                                  p + Vector(-1.5*x,  1.5*x, -x),

                                                  p + Vector(-0.5*x, -0.5*x,  x),
                                                  p + Vector( 0.5*x, -0.5*x,  x),
                                                  p + Vector( 0.5*x,  0.5*x,  x),
                                                  p + Vector(-0.5*x,  0.5*x,  x),
                                                  true); // "true" means to test each face for co-planarity.
        if ( mapr )
          prim->setTexCoordMapper( mapr );
        group->add( prim );
      }
    }
  } else if(primtype == "sphere"){
    Primitive* prim = new Sphere(matl, Vector(0,0,0), scale/max);
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    spinprim = prim;
  } else if(primtype == "box"){
    Vector p2(scale/max/1.732, scale/max/1.732, scale/max/1.732);
    Primitive* prim = new Cube(matl, -p2, p2);
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    spinprim = prim;
  } else if(primtype == "hex"){
    Real x = scale/max/1.732;
    Primitive* prim = new QuadFacedHexahedron(matl,
                                              Vector(-2*x, -x, -1.5*x),
                                              Vector( x,  x, -x),
                                              Vector( x,  x, -x),
                                              Vector(-x,  x, -x),
                                              Vector(-x, -x,  x),
                                              Vector( x, -x,  x),
                                              Vector( x,  x,  x),
                                              Vector(-x,  x,  x));
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    spinprim = prim;
  } else if(primtype == "cone"){
     Primitive* prim = new Cone(matl, scale/max, scale/max*2);
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    spinprim = prim;
    //group->add(prim);
  } else if(primtype == "torus"){
     Primitive* prim = new Torus(matl, scale/max/2, scale/max*2);
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    //spinprim = prim;
    group->add(prim);
  } else if(primtype == "intersection"){
    Vector p2(scale/max/1.414, scale/max/1.414, scale/max/1.414);
    Primitive* o1 = new Cube(matl, -p2, p2);
    Primitive* o2 = new Sphere(matl, Vector(0,0,0), scale/max);
    SphericalMapper* map = new SphericalMapper(Vector(0,0,0), scale/max);
    o1->setTexCoordMapper( mapr ? mapr : map );
    o2->setTexCoordMapper( mapr ? mapr : map );
    spinprim = new Intersection(o1, o2);
  } else if(primtype == "difference"){
    Vector p2(scale/max/1.414, scale/max/1.414, scale/max/1.414);
    Primitive* o1 = new Cube(matl, -p2, p2);
    Primitive* o2 = new Sphere(matl, Vector(0,0,0), scale/max);
    Real s = scale/max/(Real)1.414*2*(Real)(1+1.e-10);
    LinearMapper* map = new LinearMapper(Vector(0,0,0),
                                         Vector(s,0,0),
                                         Vector(0,s,0),
                                         Vector(0,0,s));
    o1->setTexCoordMapper( mapr ? mapr : map );
    o2->setTexCoordMapper( mapr ? mapr : map );
    spinprim = new Difference(o1, o2);
  } else if (primtype == "disk") {
    Primitive* prim = new Disk(matl, Vector(0, 0, 0), Vector(0, 0, 1),
                               scale / max, Vector(1, 0, 0),
                               0.25 * M_PI, 1.75 * M_PI);
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    spinprim = prim;
  } else if (primtype == "hemisphere") {
    Primitive* prim = new Hemisphere(matl, Vector(0, 0, 0), scale / max,
                                     Vector(0, 0, 1));
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    spinprim = prim;
  } else if (primtype == "superellipsoid"){
    Primitive* prim = new SuperEllipsoid(matl, Vector(0, 0, 0), scale / max, 0.5, 1.5);
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    spinprim = prim;
  } else if(primtype == "mesh") {
    AccelerationStructure *as= new DynBVH();
    Mesh* mesh = new Mesh();
    if  (!strncmp(modelName.c_str()+modelName.length()-4, ".obj", 4)) {
      mesh = new ObjGroup(modelName.c_str());
    } else {
      throw InternalError("No OBJ model found");
    }
    as->setGroup(mesh);
    spinprim = as;
  } else if(primtype == "ply"){
#ifdef USE_PRIVATE_CODE
    bgpoly = false;
    AffineTransform t;
    t.initWithScale(Vector(100, 100, 100));
    //t.initWithScale(Vector(1, 1, 1));

    KeyFrameAnimation *animation = new KeyFrameAnimation();
    //KeyFrameAnimation *animation = new KeyFrameAnimation(KeyFrameAnimation::linear);
    //AccelerationStructure *as= new Grid();
    AccelerationStructure *as= new DynBVH();
    group->add(animation);
    animation->useAccelerationStructure(as);


    Mesh *frame = NULL;
    if (!strncmp(modelName.c_str()+modelName.length()-4, ".ply", 4)) {
      frame = new Mesh;
      if (!readPlyFile(modelName, t, frame, matl))
        printf("error loading or reading ply file: %s\n", modelName.c_str());
      animation->push_back(frame);
    }
    else if  (!strncmp(modelName.c_str()+modelName.length()-4, ".obj", 4)) {
      frame = new ObjGroup(modelName.c_str());
      animation->push_back(frame);
    }
    else {
      //let load a hardcoded animation.
      //TODO: do something smart so we don't need to hardcode this.
      char model[128] = "/Users/thiago/data/models/hand/hand_00.obj";
      //char model[128] = "/Users/thiago/data/models/armadillo-kz/frame001.obj";
      for (int i=0; i <= 43; ++i) {
        model[36] = '0'+(i/10);
        model[37] = '0'+(i%10);
        //     for (int i=1; i < 11; ++i) {
        //       model[45] = '0'+(i/10);
        //       model[46] = '0'+(i%10);

        cout <<"loading " <<model<<endl;
        Group *frame = new ObjGroup(model);
        animation->push_back(frame);
      }
    }

    animation->setDuration(3);
    animation->startAnimation();

#endif //USE_PRIVATE_CODE
  } else if (primtype == "heightfield") {

    Vector anchor(-scale-1./max, -scale-1./max, -1.5/max);
    Vector v1(scale*20+2./max, 0, 0);
    Vector v2(0, scale*20+2./max, .1);

    Primitive* prim = new Heightfield(matl, modelName.c_str(), anchor+v1, anchor+v2);
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    group->add(prim);
  } else if (primtype == "waldtriangle") {
    Vector p2(scale/max/1.732, scale/max/1.732, scale/max/1.732);

    Vector v0(-p2);
    Vector v1(v0 + Vector(p2.x(), 0, 0));
    Vector v2(v0 + Vector(p2.x(), p2.y(), 0));

    // NOTE(boulos): This does nothing for WaldTriangles (why's that again?)
    WaldTriangle* prim = new WaldTriangle();
    Mesh* mesh = new Mesh();
    mesh->vertices.push_back(v0);
    mesh->vertices.push_back(v1);
    mesh->vertices.push_back(v2);

    mesh->materials.push_back(matl);

    mesh->vertex_indices.push_back(0);
    mesh->vertex_indices.push_back(1);
    mesh->vertex_indices.push_back(2);

    mesh->texture_indices.push_back(Mesh::kNoTextureIndex);
    mesh->texture_indices.push_back(Mesh::kNoTextureIndex);
    mesh->texture_indices.push_back(Mesh::kNoTextureIndex);

    mesh->face_material.push_back(0);

    mesh->addTriangle(prim);
    //prim->update();
    if ( mapr )
      prim->setTexCoordMapper( mapr );
    spinprim = prim;
  } else {
    throw IllegalArgument("Unknown primitive type for primtest: "+primtype, 0, args);
  }

  if(spinprim){
    if(arraytype == "spin"){
      for(int i=0;i<numx;i++){
        for(int j=0;j<numy;j++){
          Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                   (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                   0);
          Real a1 = i/static_cast<Real>(numx)*(Real)M_PI*2;
          Real a2 = j/static_cast<Real>(numy)*(Real)M_PI*2;
          AffineTransform t;
          t.initWithIdentity();
          t.rotate(Vector(0,1,0), a1);
          t.rotate(Vector(1,0,0), a2);
          t.translate(p);
          group->add(new InstanceRT(spinprim, t));
        }
      }
    } else if(arraytype == "shift"){
      for(int i=0;i<numx;i++){
        for(int j=0;j<numy;j++){
          Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                   (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                   0);
          group->add(new InstanceT(spinprim, p));
        }
      }
    } else if(arraytype == "scale"){
      for(int i=0;i<numx;i++){
        for(int j=0;j<numy;j++){
          Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                   (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                   0);
          int idx = j*numx+i;
          Real scale = (idx+1)/static_cast<Real>(numx*numy);
          group->add(new InstanceST(spinprim, Vector(scale, scale, scale), p));
        }
      }
    } else if(arraytype == "nuscale"){
      for(int i=0;i<numx;i++){
        for(int j=0;j<numy;j++){
          Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                   (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                   0);
          Real xscale = (i+1)/static_cast<Real>(numx);
          Real yscale = (j+1)/static_cast<Real>(numy);
          group->add(new InstanceST(spinprim, Vector(xscale, yscale, 1), p));
        }
      }
    } else if(arraytype == "spinscale"){
      for(int i=0;i<numx;i++){
        for(int j=0;j<numy;j++){
          Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                   (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                   0);
          Real a1 = i/static_cast<Real>(numx)*(Real)M_PI*2;
          Real a2 = j/static_cast<Real>(numy)*(Real)M_PI*2;
          int idx = j*numx+i;
          Real scale = (idx+1)/static_cast<Real>(numx*numy);
          AffineTransform t;
          t.initWithIdentity();
          t.scale(Vector(scale, scale, scale));
          t.rotate(Vector(0,1,0), a1);
          t.rotate(Vector(1,0,0), a2);
          t.translate(p);
          group->add(new InstanceRST(spinprim, t));
        }
      }
    } else if(arraytype == "spinscale2"){
      for(int i=0;i<numx;i++){
        for(int j=0;j<numy;j++){
          Vector p((numx>1 ? i/static_cast<Real>(numx-1)-(Real)0.5 : 0)*scale*2,
                   (numy>1 ? j/static_cast<Real>(numy-1)-(Real)0.5 : 0)*scale*2,
                   0);
          Real a1 = i/static_cast<Real>(numx)*(Real)M_PI*2;
          Real a2 = j/static_cast<Real>(numy)*(Real)M_PI*2;
          int idx = j*numx+i;
          Real scale = (idx+1)/static_cast<Real>(numx*numy);
          AffineTransform t;
          t.initWithIdentity();
          t.scale(Vector(scale, scale, scale));
          t.rotate(Vector(0,1,0), a1);
          t.rotate(Vector(1,0,0), a2);
          t.translate(p);
          group->add(new Instance(spinprim, t));
        }
      }
    } else {
      throw IllegalArgument("Unknown array type for primtest: "+primtype, 0, args);
    }
  }
  if(bgpoly && !bgpoly_first){
    Texture<Color>* tex = new CheckerTexture<Color>(Color(RGB(0.4, 0.4, 0.4)),
                                                    Color(RGB(0.2, 0.2, 0.2)),
                                                    Vector(1,0,0)*numx*2,
                                                    Vector(0,1,0)*numy*2);
    //    Material* bgmatl = new Lambertian(tex);
    Material* bgmatl = new Phong(tex,
                                 new Constant<Color>(Color(RGB(0.9, 0.9, 0.9))),
                                 32, NULL);

    Vector anchor(-scale-1./max, -scale-1./max, -1.5/max);
    Vector v1(scale*2+2./max, 0, 0);
    Vector v2(0, scale*2+2./max, 0);
#if 0
    group->add(new Triangle(bgmatl, anchor, anchor+v1, anchor+v2));
    group->add(new Triangle(bgmatl, anchor+v1+v2, anchor+v2, anchor+v1));
#else
    group->add(new Parallelogram(bgmatl, anchor, v1, v2));
#endif
  }
  Scene* scene = new Scene();
  scene->setBackground(new LinearBackground(Color(RGB(0.2, 0.4, 0.9)),
                                            Color(RGB(0.0,0.0,0.0)),
                                            Vector(0,1,0)));
  scene->setObject(group);

  LightSet* lights = new LightSet();
  lights->add(new PointLight(Vector(-2,4,-8), Color(RGB(1,1,1))*1));
  Color cup(RGB(0.3, 0.3, 0.3));
  Color cdown(RGB(0.62, 0.62, 0.62));
  Vector up(0,1,0);
  lights->setAmbientLight(new ArcAmbient(cup, cdown, up));
  scene->setLights(lights);
  return scene;
}
