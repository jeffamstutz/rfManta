from manta import *

global factory

import string

doAddDebugRays = False

def getVectorFromTokens(tokens, index):
    return Vector(string.atof(tokens[index+1]),
                  string.atof(tokens[index+2]),
                  string.atof(tokens[index+3]))

def addDebugRays(objs, infilename, radius, deadEndRayLength = 2):
    materials = [manta_new(Lambertian(Color(RGBColor(1,0,0)))),
                 manta_new(Lambertian(Color(RGBColor(1,1,0)))),
                 manta_new(Lambertian(Color(RGBColor(0,1,0)))),
                 manta_new(Lambertian(Color(RGBColor(0,1,1)))),
                 manta_new(Lambertian(Color(RGBColor(0,0,1)))),
                 manta_new(Lambertian(Color(RGBColor(1,0,1))))]
    infile = open(infilename, "rU")
    # Now read the lines
    lines = map( lambda x: string.strip(x, string.whitespace),
                 infile.readlines() )
    infile.close()
    # Loop over all the lines and pull out the relevant stuff
    for l in lines[0:]:
        tokens = l.split()
        if ("raytree:" in tokens):
            originIndex = tokens.index("origin")
            origin = getVectorFromTokens(tokens, originIndex)
            raydepth = string.atoi(tokens[tokens.index("depth")+1])
            matl = materials[raydepth%len(materials)]
            # Try and get the hitpos
            if ("hitpos" in tokens):
                print "Normal ray"
                hitposIndex = tokens.index("hitpos")
                hitpos = getVectorFromTokens(tokens, hitposIndex)
            else:
                print "Computing stub"
                # Get the direction and compute a distance
                directionIndex = tokens.index("direction")
                direction = getVectorFromTokens(tokens, directionIndex)
                hitpos = origin + (direction.normal() * deadEndRayLength)
            objs.add(manta_new(Cylinder(matl, origin, hitpos, radius)))

def addBookmarkFromString(scene, parameters):
    tokens = parameters.split()
    eye    = getVectorFromTokens(tokens, tokens.index("-eye"))
    lookat = getVectorFromTokens(tokens, tokens.index("-lookat"))
    up     = getVectorFromTokens(tokens, tokens.index("-up"))
    fov    = string.atof(tokens[tokens.index("-fov")+1])
    scene.addBookmark(parameters, eye, lookat, up, fov, fov)
    
def createDefaultScenePython():
    scene = manta_new(Scene())
    scene.setBackground(manta_new(ConstantBackground(ColorDB.getNamedColor("SkyBlue3").scaled(0.5))))
    red = manta_new(Phong(Color(RGBColor(0.6, 0, 0)),
                          Color(RGBColor(0.6,0.6,0.6)),
                          32, 0.4))
    checker1 = manta_new(CheckerTexture_Color(Color(RGBColor(.6,.6,.6)),
                                              Color(RGBColor(0,0,0)),
                                              Vector(1,0,0),
                                              Vector(0,1,0)))
    constant_color1 = manta_new(Constant_Color(Color(RGBColor(.6,.6,.6))))
    checker2 = manta_new(CheckerTexture_ColorComponent(0.2, 0.5, Vector(1,0,0),
                                             Vector(0,1,0)))
    plane_matl = manta_new(Phong(checker1, constant_color1, 32, checker2))
    #
    world = manta_new(Group())
    floor = manta_new(Parallelogram(plane_matl, Vector(-20,-20,0),
                                    Vector(40,0,0), Vector(0,40,0)))
    uniformmap = manta_new(UniformMapper())
    floor.setTexCoordMapper(uniformmap)
    world.add(floor)
    world.add(manta_new(Sphere(red, Vector(0,0,1.2), 1.0)))

    if (doAddDebugRays):
        objs = manta_new(Group())
        addDebugRays(objs, "raytree", 0.025)
        objs.add(world)
        dynbvh = manta_new(DynBVH())
        dynbvh.rebuild(objs)
        world = dynbvh
        
    scene.setObject(world)
    #
    lights = manta_new(LightSet())
    lights.add(manta_new(PointLight(Vector(0,5,8), Color(RGBColor(.6,.1,.1)))))
    lights.add(manta_new(PointLight(Vector(5,0,8), Color(RGBColor(.1,.6,.1)))))
    lights.add(manta_new(PointLight(Vector(5,5,2), Color(RGBColor(.2,.2,.2)))))
    lights.setAmbientLight(manta_new(ConstantAmbient(Color.black())))
    print lights
    #
    scene.setLights(lights)
    scene.getRenderParameters().maxDepth = 5
    addBookmarkFromString(scene, "pinhole( -eye 3 3 2 -lookat 0 0 0.3 -up 0 0 1 -fov 60 )")
    return scene

def createDielectricTestScene():
    scene = manta_new(Scene())
    scene.setBackground(manta_new(ConstantBackground(Color(RGBColor(0.5, 0.8, 0.9)))))
    # world will be added to the scene
    world = manta_new(Group())
    # Collection of objects that will be put in the BVH.
    objs = Array1_ObjectP()
    # The gound plane
    groundmatl = manta_new(Lambertian(Color(RGBColor(0.95, 0.65, 0.35))))
    # We can't add the plane to objs, because it doesn't have a proper
    # bounding box.
    world.add(manta_new(Plane(groundmatl, Vector(0,0,1), Vector(0,0,2.5))))
    # Metal sphere
    ball_matl = manta_new(MetalMaterial(Color(RGBColor(0.8, 0.8, 0.8)), 100))
    objs.add(manta_new(Sphere(ball_matl, Vector(-6, 3.5, 3.5), 1.0)))
    #
    lenscale = 2.5
    glass = Color(RGBColor(pow(0.80, lenscale), pow(0.93, lenscale), pow(0.87, lenscale)))
    for i in range(4):
        eta = 1 + i*0.5 + .05
        transp_matl = manta_new(Dielectric(eta, 1, glass))
#         transp_matl = manta_new(Lambertian(Color(RGBColor(0.3,0.1,0.1))))
        corner = Vector(i*1.3 - 4, -3, 2.5+1.e-4);
        size = Vector(0.20, 2.5, 1.4);
        objs.add(manta_new(Cube(transp_matl, corner, corner+size)))
        #
    # Line of rings
    ringmatl = manta_new(Lambertian(Color(RGBColor(.6, .6, .9))))
    r = .30
    inner_radius = r*0.5
    center = Vector(-6, 0, 2.5+r)
    offset = Vector(2.25*r, 0, 0);
    for i in range(9):
        objs.add(manta_new(Ring(ringmatl,
                                center+offset*i,
                                Vector(0.2, -1, -0.2),
                                inner_radius, r-inner_radius)))
        #
    # Create a BVH
    world.add(manta_new(RealisticBvh(objs.get_objs(), objs.size())))
    scene.setObject(world)
    #
    lights = manta_new(LightSet())
    lights.add(manta_new(PointLight(Vector(20, 30, 100), Color(RGBColor(.9,.9,.9)))))
    lights.add(manta_new(PointLight(Vector(-40, -30, 50), Color(RGBColor(.3,.1,.1)))))
    lights.setAmbientLight(manta_new(ConstantAmbient(Color(RGBColor(.4, .4, .4)))))
    print lights
    #
    scene.setLights(lights)
    scene.getRenderParameters().maxDepth = 25
    scene.getRenderParameters().importanceCutoff = 0.01
    #
    addBookmarkFromString(scene, "pinhole( -eye 8 -18 8.5 -lookat -4.7 2.5 2.5 -up 0 0 1 -fov 15 )")
    return scene

def createProg4TestScene():
    scene = manta_new(Scene())
    scene.setBackground(manta_new(ConstantBackground(Color(RGBColor(0.5, 0.8, 0.9)))))
    #
    # world will be added to the scene
    world = manta_new(Group())
    # Collection of objects that will be put in the BVH.
    objs = Array1_ObjectP()
    #
    groundmatl = manta_new(Lambertian(Color(RGBColor(0.25, 0.95, 0.25))))
    world.add(manta_new(Plane(groundmatl,
                              Vector(0,0,1), Vector(0,0,2.5))))
    #
    lenscale = 2.5
    glass = Color(RGBColor(pow(0.80, lenscale), pow(0.93, lenscale), pow(0.87, lenscale)))
    for i in range(4):
        eta = 1 + i*0.5 + .05
        transp_matl = manta_new(Dielectric(eta, 1, glass))
        corner = Vector(i*1.3 - 4, -3, 2.5+1.e-4)
        size = Vector(0.20, 2.5, 1.4)
        objs.add(manta_new(Cube(transp_matl, corner, corner+size)))
        #
    # Line of rings
    ringmatl = manta_new(Lambertian(Color(RGBColor(.6, .6, .9))))
    r = .30
    inner_radius = r*0.5
    center = Vector(-6, 0, 2.5+r)
    offset = Vector(2.25*r, 0, 0)
    for i in range(9):
        objs.add(manta_new(Ring(ringmatl,
                                center+offset*i,
                                Vector(0.2, -1, -0.2),
                                inner_radius, r-inner_radius)))
    # Metal balls
    ball_matl = manta_new(MetalMaterial(Color(RGBColor(0.8, 0.8, 0.8)), 100))
    objs.add(manta_new(Sphere(ball_matl, Vector(-6, 3.5, 3.5), 1.0)))
    objs.add(manta_new(Sphere(ball_matl, Vector(-7, 2.5, 2.5), 1.0)))
    objs.add(manta_new(Sphere(ball_matl, Vector(-5, 4.5, 4.5), 1.0)))
    #
    # Blue balls
    phongmatl = manta_new(Phong(Color(RGBColor(0.3, 0.3, 0.9)),
                                Color(RGBColor(1,1,1)), 30))
    for i in range(5):
        objs.add(manta_new(Sphere(phongmatl, Vector(-4.5 + i, 3.5 + i/4., 3 + i/2.), 0.5)))
    #
    phongmatl2 = manta_new(Phong(Color(RGBColor(0.9, 0.3, 0.3)),
                                 Color(RGBColor(1,1,1)), 30))
    objs.add(manta_new(Cube(phongmatl2,
                            Vector(-4.5, -4.5, 2),
                            Vector(-8.5, -8.5, 8))))
    # Create a BVH
    world.add(manta_new(RealisticBvh(objs.get_objs(), objs.size())))
    scene.setObject(world)
    #
    lights = manta_new(LightSet())
    lights.add(manta_new(PointLight(Vector(20, 30, 100), Color(RGBColor(.9,.9,.9)))))
    lights.add(manta_new(PointLight(Vector(-40, -30, 50), Color(RGBColor(.3,.1,.1)))))
    lights.setAmbientLight(manta_new(ConstantAmbient(Color(RGBColor(.4, .4, .4)))))
    #
    scene.setLights(lights)
    scene.getRenderParameters().maxDepth = 25
    scene.getRenderParameters().importanceCutoff = 0.01
    #
    addBookmarkFromString(scene, "pinhole( -eye 8.3 -18 7 -lookat -4.7 2.5 3 -up 0 0 1 -fov 15 )")
    return scene

def createTransparentShadowScene():
    scene = manta_new(Scene())
    scene.setBackground(manta_new(ConstantBackground(Color(RGBColor(0.5, 0.8, 0.9)))))
    #
    # world will be added to the scene
    world = manta_new(Group())
    # Collection of objects that will be put in the BVH.
    objs = Array1_ObjectP()
    #
    checker1 = manta_new(CheckerTexture_Color(Color(RGBColor(.6,.6,.6)),
                                              Color(RGBColor(0.1,0.6,0.1)),
                                              Vector(1,0,0),
                                              Vector(0,1,0)))
    groundmatl = manta_new(Lambertian(checker1))
    print groundmatl
    world.add(manta_new(Plane(groundmatl,
                              Vector(0,0,1), Vector(0,0,2.5))))
    #
    lenscale = 2.5
    glass = Color(RGBColor(pow(0.80, lenscale), pow(0.93, lenscale), pow(0.87, lenscale)))
    if (doAddDebugRays):
        transp_matl = manta_new(Dielectric(1.0, 1.0, glass))
    else:
        transp_matl = manta_new(Dielectric(1.5, 1.0, glass))
#    transp_matl.setDoSchlick(True)
    print transp_matl
    objs.add(manta_new(Cube(transp_matl,
                            Vector(-5, -2, 2.95 ),
                            Vector(-1, 2, 3.36))))

    objs.add(manta_new(Sphere(transp_matl,
                              Vector( -3, 0, 4.5), 0.5)))

    # Thin dielectric
#     objs.add(manta_new(Parallelogram(manta_new(ThinDielectric(1.2, Color(RGBColor(0.7, 0.2, 0.4)), 0.1)),
#                                      Vector(-1, -2, 4.0),
#                                      Vector(2, 0, 0),
#                                      Vector(0, 2, 0))))
    
    ringmatl = manta_new(Lambertian(Color(RGBColor(.9, .3, .2))))
    print ringmatl
    r = .30
    inner_radius = r*0.5
    center = Vector(-6, 0, 3.5+r)
    offset = Vector(2.25*r, 0, 0)
#     for i in range(9):
#         objs.add(manta_new(Ring(ringmatl,
#                                 center+offset*i,
#                                 Vector(0, 0, 1),
#                                 inner_radius, r-inner_radius)))

    if (doAddDebugRays):
        addDebugRays(objs, "raytree", 0.025)
    # Create a BVH
    world.add(manta_new(RealisticBvh(objs.get_objs(), objs.size())))
    scene.setObject(world)
    #
    lights = manta_new(LightSet())
    lights.add(manta_new(PointLight(Vector(20, 30, 100), Color(RGBColor(.9,.9,.9)))))
#    lights.add(manta_new(PointLight(Vector(-40, -30, 50), Color(RGBColor(.3,.1,.1)))))
    lights.setAmbientLight(manta_new(ConstantAmbient(Color(RGBColor(.4, .4, .4)))))
    #
    scene.setLights(lights)
    scene.getRenderParameters().maxDepth = 25
    scene.getRenderParameters().importanceCutoff = 0.01
    scene.addBookmark("createTransparentShadowScene camera",
                      Vector(8.3, -18, 7),
                      Vector(-4.7, 2.5, 3), Vector(0, 0, 1), 15)
    addBookmarkFromString(scene, "pinhole( -eye 8.06072 -17.7429 8.73358 -lookat -4.98977 2.23422 2.74486 -up -0.0793095 0.0464671 0.995767 -fov 15 )");
    addBookmarkFromString(scene, "pinhole( -eye 7.92503 -16.4782 12.1826 -lookat -4.42502 2.59203 2.7444 -up -0.216459 0.129008 0.967731 -fov 15 )")
    #
    return scene

def setupDefaultEngine(numworkers):
    print "Using " + str(numworkers) + " rendering threads."
    engine = createManta()

    global factory
    # For whatever reason the compiler on my laptop blows up garbage
    # collecting this thing, so I'm not going to delete it.
    factory = manta_new(Factory( engine ))
    
    engine.changeNumWorkers(numworkers)
    factory.selectImageType("rgba8")
    factory.selectLoadBalancer("workqueue")
    factory.selectImageTraverser("tiled(-square -tilesize 30x5)")
#    factory.selectImageTraverser("tiled")
    factory.selectPixelSampler("singlesample")
    #factory.selectPixelSampler("jittersample(-numberOfSamples 4)")
    factory.selectRenderer("raytracer")
    if (doAddDebugRays):
        factory.selectShadowAlgorithm("noshadows")
    else:
        factory.selectShadowAlgorithm("hard(-attenuate)")
    #factory.selectShadowAlgorithm("hard")
    #
    return engine

def addSyncXInterface(engine, camera, xres, yres):

    xinterface = factory.createUserInterface("X")
    xinterface.startup()
    args = vectorStr()
    ogl_display = manta_new(OpenGLDisplay(args))
    sync_display = manta_new(SyncDisplay(args))
    sync_display.setChild(ogl_display)
    sync_automator = manta_new(SyncFrameAutomator(engine, sync_display))
    sync_automator.startup()
    engine.createChannel(sync_display, camera, False, xres, yres)
#    engine.createChannel("opengl", camera, False, xres, yres)

def addXInterface(engine, camera, xres, yres):
    xinterface = factory.createUserInterface("X")
    xinterface.startup()
    display = factory.createImageDisplay("opengl");
    engine.createChannel(display, camera, False, xres, yres)

def addNullInterface(engine, camera, xres, yres):
    ui = factory.createUserInterface("null")
    ui.startup()
    engine.createChannel("null", camera, False, xres, yres)

def addFileInterface(engine, camera, xres, yres):
    ui = factory.createUserInterface("null")
    ui.startup()
    display = factory.createImageDisplay("file(-type tga -prefix runmanta)")
    engine.createChannel(display, camera, False, xres, yres)

engine = setupDefaultEngine(1)

defaultCamera = factory.createCamera("pinhole(-normalizeRays)");

addXInterface(engine, defaultCamera, 512, 512)
#addNullInterface(engine, defaultCamera, 512, 512)
#addFileInterface(engine, defaultCamera, 512, 512)

#scene = createDefaultScene()
#scene = createDefaultScenePython()
#scene = createDielectricTestScene()
#scene = createProg4TestScene()
scene = createTransparentShadowScene()
engine.setScene(scene)


# if __name__ == "__main__":
#     engine.beginRendering(True)
# else:
#     engine.beginRendering(False)

# For running as a script
engine.beginRendering(True)
# For running within python
#engine.beginRendering(False)
