#include <Core/Color/Color.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/ObjGroup.h>
#include <Model/Instances/Instance.h>
#include <Model/Lights/DirectionalLight.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Checker.h>
#include <Model/Materials/CopyTextureMaterial.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/OrenNayar.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/ThinDielectric.h>
#include <Model/Primitives/Cone.h>
#include <Model/Primitives/Cube.h>
#include <Model/Primitives/Cylinder.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Ring.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Primitives/Torus.h>
#include <Model/TexCoordMappers/LinearMapper.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/ImageTexture.h>
#include <Model/Textures/MarbleTexture.h>
#include <Model/Textures/WoodTexture.h>

#include <iostream>

#define Rect(material, center, dir1, dir2) \
  Parallelogram(material, (center) - (dir1) - (dir2), (dir1)*2, (dir2)*2)

using namespace Manta;

void addLights( LightSet* lights)
{
  lights->add(new DirectionalLight(Vector(0, 0, 1), Color::white()));
  
  //Patricia's Code
  //lights->add(new DirectionalLight(Vector(0, 1, 0), Color(RGB(.1, .1, .9))));
  
  lights->add(new PointLight(Vector(30, 30, -30), Color(RGB(.7, .7, .7))));
  lights->add(new PointLight(Vector(30, 30, 30), Color(RGB(.7, .7, .7))));  
  //lights->add(new PointLight(Vector(-30, 30, -30), Color(RGB(.7, .7, .7))));
  lights->add(new PointLight(Vector(-30, 30, 30), Color(RGB(.7, .7, .7))));

}

//Patricia's Stuff
void addBugs(Group* group)
{		
		Group* bug1 = new Group();

		Material* metalMat = new MetalMaterial(Color(RGB(0.95, 0.25, 0.25)), 10); 
				
		//bubbles
		Material* glass= new Dielectric(1.5, 1.0, Color(RGB(.80, .93 , .87)).Pow(0.2));
		Material* thinDielectric = new ThinDielectric(5, Color(RGB(.1, .5, .9)), 2);

		//Material* lambertianTest = new Lambertian(Color(RGB(.1, .9, .1)));
		//Material* phongTest = new Phong(Color(RGB(.9, .1, .6)), Color(RGB(1.0, 1.0, 1.0)), 400);
					
		//Bug1
		for(int yPos=5; yPos>0; yPos=yPos-2)
		{		
			Material* tempMat;			
			for(int i=0; i < 6-yPos; i=i+2)
			{
				if((i+yPos)/2==1)
					tempMat = thinDielectric; 				
				else
					tempMat = glass;
					
				Primitive* bubble1 = new Sphere(tempMat, Vector(i, yPos, 0), 1);
				bug1->add(bubble1);
				Primitive* bubble2 = new Sphere(tempMat, Vector(-1*i, yPos, 0), 1);
				bug1->add(bubble2);					
			}
		}
		
		for(int zPos=4; zPos>1; zPos=zPos-2)
		{			
			Material* tempMat;		
			for(int i=0; i < 6-zPos; i=i+2)
			{
				if((i+zPos)/2==1)
					tempMat = thinDielectric; 				
				else
					tempMat = glass;
				Primitive* bubble1 = new Sphere(tempMat, Vector(i, 1, zPos), 1);
				bug1->add(bubble1);
				Primitive* bubble2 = new Sphere(tempMat, Vector(-1*i, 1, zPos), 1);
				bug1->add(bubble2);					
			}
		}
		
		//Bug1 eyes
		Primitive* leftEye = new Sphere(metalMat, Vector(5, 0.75, 0.5), 0.25);
		bug1->add(leftEye);
		Primitive* rightEye = new Sphere(metalMat, Vector(5, 1.4, -0.5), -0.25);
		bug1->add(rightEye);
		//pupils
		Material* pupil = new Phong(Color(RGB(.9, .1, .6)), Color(RGB(1.0, 1.0, 1.0)), 400);
		Primitive* leftPupil = new Sphere(pupil, Vector(5.15, 0.75, 0.5), 0.15);
		bug1->add(leftPupil);
		Primitive* rightPupil = new Sphere(pupil, Vector(5.15, 1.4, -0.5), -0.15);
		bug1->add(rightPupil);
				
		//tail
		
		Material* metalMatCone = new MetalMaterial(Color(RGB(0.95, 0.25, 0.25)), 10); 
		Primitive* tail = new Cone(metalMatCone, .25, 3);
		
		//mouth
		Primitive* mouth = new Ring(metalMatCone, Vector(5, .4, -.2), Vector(3, .5, 0), .1, .09);
		bug1->add(mouth);
			
		//torus primitive
		Material* glass2 = new Dielectric(1.5, 1.0, Color(RGB(.9, .1, .9)).Pow(0.2));
		Primitive* tailRing = new Torus(glass2, .2, 1.0);
				
		Group* coneTorusGrp = new Group();
		coneTorusGrp->add(tail);
		coneTorusGrp->add(tailRing);
				
		AffineTransform t;
		t.initWithIdentity();
		t.rotate(Vector(0,-1,0), M_PI_2);
		t.translate(Vector(-6, 1, 0));
		bug1->add(new Instance(coneTorusGrp, t));
		
		AffineTransform a;
		a.initWithIdentity();
		a.translate(Vector(0,8,0));
														
		AffineTransform b;
		b.initWithIdentity();
		b.rotate(Vector(0,1,0), M_PI_4);
		b.translate(Vector(-8,15,2));
		
		AffineTransform c;
		c.initWithIdentity();
		c.rotate(Vector(0,0,1), M_PI_4);
		c.translate(Vector(13,7,-5));
		
		group->add(new Instance(bug1, a));
		group->add(new Instance(bug1, b));
		group->add(new Instance(bug1, c));
		
		//Bug 4: Optional
		/*AffineTransform d;
		d.initWithIdentity();
		d.rotate(Vector(1,0,0), M_PI_4);
		d.translate(Vector(-16,4,15));
		group->add(new Instance(bug1, d));
		*/		
		
		//Bug of different color patterns: Optional
		/*//Bug2
		Group* bug2 = new Group();
		
		for(int yPos=5; yPos>0; yPos=yPos-2)
		{	
			Material* tempMat;
			for(int i=0; i < 6-yPos; i=i+2)
			{
				if((i+yPos)/2==1)
					tempMat = glass; 				
				else
					tempMat = thinDielectric;
					
				Primitive* bubble1 = new Sphere(tempMat, Vector(i-5, yPos+6, -5), 1);
				bug2->add(bubble1);
				Primitive* bubble2 = new Sphere(tempMat, Vector(-1*i-5, yPos+6, -5), 1);
				bug2->add(bubble2);					
			}
		}
				
		for(int zPos=4; zPos>1; zPos=zPos-2)
		{			
			Material* tempMat;
			for(int i=0; i < 6-zPos; i=i+2)
			{
				if((i+zPos)/2==1)
					tempMat = glass; 				
				else
					tempMat = thinDielectric;
					
				Primitive* bubble1 = new Sphere(tempMat, Vector(i-5, 1+6, zPos-5), 1);
				bug2->add(bubble1);
				Primitive* bubble2 = new Sphere(tempMat, Vector(-1*i-5, 1+6, zPos-5), 1);
				bug2->add(bubble2);					
			}
		}
		
		//Bug2 eyes
		Primitive* b2leftEye = new Sphere(metalMat, Vector(0, 6.75, -4.5), 0.25);
		bug2->add(b2leftEye);
		Primitive* b2rightEye = new Sphere(metalMat, Vector(0, 7.4, -5.5), -0.25);
		bug2->add(b2rightEye);
		
		group->add(bug2);
		*/
		
}

void makeTrees(Group* group)
{	
	Group* tree = new Group();
	
	//texture if used with lambertian for bark
	//MarbleTexture<Color>* barkTex1 = new MarbleTexture<Color>
	//(Color(RGB(0.3,0.3,0.3)), Color(RGB(0.9,0.9,0.5)),
	//15, 1.0, 10.0, 6.0, 2.0, 0.5);
	Material* leafMat = new MetalMaterial(Color(RGB(0.1, 0.8, 0.2)), 200);
	Material* bark = new MetalMaterial(Color(RGB(.3,.3,.3)), 200);
	
	Primitive* coneBottom = new Cone(bark, .75, 3);
	Primitive* cylBottom = new Cylinder(bark, Vector(0,0,0), Vector(0,0,4), .5);
	Primitive* cylTop = new Cylinder(bark, Vector(0,0,4.5), Vector(0,0,8), .5);
	Primitive* trunkJoint = new Sphere(bark, Vector(0,0,4.25), .5);
	Primitive* trunkJoint2 = new Sphere(bark, Vector(0,0,8.25), .5);
	Primitive* coneTop = new Cone(bark, .5, 2);
	Primitive* leaf = new Parallelogram(leafMat, Vector(0,0,10), Vector(0,.5,-1), Vector(5,0,0));
	
	Group* branches = new Group();
	branches->add(leaf);		
	
	Group* branchesLow = new Group();
	AffineTransform tl;
		tl.initWithIdentity();
		tl.rotate(Vector(0,1,1), M_PI/2);
		tl.translate(Vector(-10,-10,0));
		branchesLow->add(new Instance(leaf, tl));
	Group* branchesHigh = new Group();
	AffineTransform th;
		th.initWithIdentity();
		th.rotate(Vector(0,1,1), M_PI_2);
		th.translate(Vector(-10,-12,3));
		branchesHigh->add(new Instance(leaf, th));
		
	branches->add(branchesHigh);
	
	for(int i=0; i<5; i++)
	{
		Group* temp = new Group();
		Group* tempLow = new Group();
		Group* tempHigh = new Group();
	
		AffineTransform leaves;
			leaves.initWithIdentity();
			leaves.rotate(Vector(0,0,1), M_PI/(i/2));
			branches->add(new Instance(leaf, leaves));
			temp->add(new Instance(leaf, leaves));
			branches->add(new Instance(branchesLow, leaves));
			tempLow->add(new Instance(branchesLow, leaves));
			branches->add(new Instance(branchesHigh, leaves));
			tempHigh->add(new Instance(branchesHigh, leaves));
			
			
		AffineTransform leaves2;
			leaves2.initWithIdentity();
			leaves2.rotate(Vector(0,0,1), M_PI);
			branches->add(new Instance(temp, leaves2));
			branches->add(new Instance(tempLow, leaves));
			branches->add(new Instance(tempHigh, leaves));
	}		
	
	Group* treeMid = new Group();	
		
	//add group for top part of tree tilt
	Group* treeTop = new Group();
	treeTop->add(cylTop);
	treeTop->add(trunkJoint2);
	treeTop->add(branches);
	//adding the cone at top of tree
	AffineTransform tConeTop;
		tConeTop.initWithIdentity();
		tConeTop.translate(Vector(0,0,8.5));
		treeTop->add(new Instance(coneTop, tConeTop));
	//transform so leaning slightly
	AffineTransform tHigh;
		tHigh.initWithIdentity();
		tHigh.rotate(Vector(0,-1,0), M_PI_4/3);	
		tHigh.translate(Vector(1.2,0,0));	
		treeMid->add(new Instance(treeTop, tHigh));	
	
	//add rest of tree of til except botton cone
	treeMid->add(cylBottom);
	treeMid->add(trunkJoint);
	AffineTransform tTrunk;
		tTrunk.initWithIdentity();
		tTrunk.rotate(Vector(0,-1,0), M_PI_4/4);
		tTrunk.translate(Vector(0,0,-.5));
		tree->add(new Instance(treeMid, tTrunk));
	
	//add the very bottom cone		
	AffineTransform tConeBottom;
		tConeBottom.initWithIdentity();
		tConeBottom.translate(Vector(0, 0, -2));
		tree->add(new Instance(coneBottom, tConeBottom));
		
		
	AffineTransform t;
		t.initWithIdentity();
		t.scale(Vector(2,2,2));
		t.translate(Vector(0,0,-17));
	AffineTransform t2;
		t2.initWithScale(Vector(2,2,2));
		t2.rotate(Vector(0,0,1), M_PI_2);
		t2.translate(Vector(10,0,-17));
	group->add(new Instance(tree, t));
	group->add(new Instance(tree, t2));
	
}

void makeFloor(Group* group)
{		
	MarbleTexture<Color>* marbTex1 = new MarbleTexture<Color>
	(Color(RGB(0.9,0.3,0.9)), Color(RGB(0.9,0.9,0.5)),
	10, 1.0, 20.0, 6.0, 2.0, 0.5);
	
	MarbleTexture<Color>* marbTex2 = new MarbleTexture<Color>
	(Color(RGB(0.2,0.3,0.9)), Color(RGB(0.5,0.9,0.5)),
	5, 1.0, 15.0, 6.0, 2.0, 0.5);
	
	//Material* marble1 = new Phong(marbTex1, new Constant<Color>(Color::white()*0.6),
	//								50, new Constant<ColorComponent>(0));
							
	//Material* marble2 = new Phong(marbTex2, new Constant<Color>(Color::white()*0.6),
	//								50, new Constant<ColorComponent>(0));
									
	Material* marble3 = new Lambertian(marbTex1);
	Material* marble4 = new Lambertian(marbTex2);
	
	//Optional Floor Patterns
	//Primitive* test1 = new Sphere(marble2, Vector(0,0,0), 5);
	//Primitive* test2 = new Parallelogram(marble4, Vector(0,0,0), Vector(10,0,0), Vector(0,10,0));
	
																
	//Material* checkered_floor = new Checker(marble1,
    //                                      marble2,
    //                                      Vector(16,0,0), Vector(0,16,0));
										  
	Material* checkered_floor2 = new Checker(marble3,
											marble4,
											Vector(16,0,0), Vector(0,16,0));
										  
	//Object* floor = new Rect(checkered_floor,
    //                     Vector(0,0,-20.0),
    //                     Vector(80.0,0,0),
    //                     Vector(0,80.0,0));
						 
										  
	Object* floor1 = new Rect(checkered_floor2,
                         Vector(0,0,-20.0),
                         Vector(80.0,0,0),
                         Vector(0,80.0,0));
						 
	//Primitive* floor2 = new Parallelogram(checkered_floor2,
    //                     Vector(0,0,-20.0),
    //                     Vector(80.0,0,0),
    //                     Vector(0,80.0,0));		
						 					 
	group->add(floor1);
}

void makeWalls(Group* group)
{
	//Material* wall = new Dielectric(1.5, 1.0, Color(RGB(.80, .2 , .87)).Pow(0.2));
	Material* wall = new Phong(Color(RGB(.9, .1, .6)), Color(RGB(1.0, 1.0, 1.0)), 400);
	
	group->add(new Rect(wall, Vector(80,0,20), Vector(0,0,40), Vector(0,80,0)));
	group->add(new Rect(wall, Vector(-80,0,20), Vector(0,0,40), Vector(0,80,0)));
	group->add(new Rect(wall, Vector(0,80,20), Vector(0,0,40), Vector(80,0,0)));
	group->add(new Rect(wall, Vector(0,-80,20), Vector(0,0,40), Vector(80,0,0)));


}

//Simple Exploring Code: Irrelevent to Scene
void makeRandomStuff(Group* group)
{
		//Random Box
		//Primitive* box = new Cube(metalMatCone, Vector(0, 0, 0), Vector(5, 2, 1));
		//group->add(box);	
		
		//Ring Primitive
		//Material* orenNayar = new OrenNayar(Color(RGB(.1, .9, .1)), 10);
		Material* thinDielectric = new ThinDielectric(5, Color(RGB(.9, .3, .4)), 2);
		Primitive* randomRing = new Ring(thinDielectric, Vector(-3, 4, -5.5), Vector(0, 1, 1), 2, 1);
		group->add(randomRing);
				
		/*Material* metalMatCone = new MetalMaterial(Color(RGB(0.95, 0.25, 0.25)), 10); 
		Primitive* randomCone = new Cone(metalMatCone, .25, 3);
		//group->add(randomCone);
		
		//torus primitive
		Material* glass2 = new Dielectric(1.5, 1.0, Color(RGB(.9, .1, .9)).Pow(0.2));
		Primitive* torusTest = new Torus(glass2, .2, 1.0);
		//group->add(torusTest);
		
		Group* coneTorusGrp = new Group();
		coneTorusGrp->add(torusTest);
		coneTorusGrp->add(randomCone);
				
		AffineTransform t;
		t.initWithIdentity();
		//t.scale(Vector(1, 1, 1));
		t.rotate(Vector(0,-1,0), M_PI_2);
		t.translate(Vector(-6, 1, 0));
		group->add(new Instance(coneTorusGrp, t));
		*/
		

}

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
	if (args.size() != 0){
      std::string arg = args[0];
      std::cerr << "Unknown option: " << arg << "\n";
	  return 0;
    }


  Vector up( 0.0f, 0.0f, 1.0f );
  Vector right( 1.0f, 0.0f, 0.0f );

  // Start adding geometry
  Group* group = new Group();
  Scene* scene = new Scene();


  addBugs(group);
  makeTrees(group);
  makeFloor(group);
  makeWalls(group);
  //makeRandomStuff(group);
  scene->setBackground( new ConstantBackground( Color(RGB(0.9, 0.9, 0.9) ) ) );

  DynBVH* bvh = new DynBVH();
  bvh->setGroup(group);
  scene->setObject(bvh);
  //scene->setObject(group);

  LightSet* lights = new LightSet();
  addLights( lights );

  lights->setAmbientLight(new ConstantAmbient( Color::white()*0.2));
  scene->setLights(lights);

  // Add a default camera
  /*Vector eye(3,2,-10);
  Vector lookat(3,2,0);
  Real   fov=45;*/
  
  scene->addBookmark("trees", Vector(49.956, 49.6588, 16.4095), Vector(2.63869, 8.73985, 0.121314), Vector(-0.433617, -0.253682, 0.864652), 45, 45);
  
  Vector eye(40,10,-10);
  scene->getRenderParameters().setImportanceCutoff(.01);
  Vector lookat(3,7,0);
  Real fov=45;
  scene->addBookmark("default view", eye, lookat, up, fov, fov);


  return scene;
}

