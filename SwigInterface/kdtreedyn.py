
import wxManta
import wx
import getopt, sys

from manta import *

from KDTreeDynFrame import *

###############################################################################
###############################################################################
# Global Options
###############################################################################
###############################################################################

# Remember to declare these are "global" when defining them.
builder_name = "Build_OnLogn_cc"
file_name = ""
leaf_threshold       = 8
depth_threshold      = 64
cost_threshold       = 1.0

###############################################################################
###############################################################################
# Create Scene
###############################################################################
###############################################################################

def initialize_scene( frame, engine ):

    ###########################################################################
    # Create basic scene with background.
    scene = manta_new( Scene() )
    scene.getRenderParameters().maxDepth = 5
    scene.setBackground(manta_new(ConstantBackground(ColorDB.getNamedColor("FloralWhite"))))

    engine.selectShadowAlgorithm( "noshadows" )

    # Create lights.
    lights = manta_new(LightSet())
    lights.add(manta_new(PointLight(Vector(0,5,8), Color(RGBColor(.6,.1,.1)))))
    lights.add(manta_new(PointLight(Vector(5,0,8), Color(RGBColor(.1,.6,.1)))))
    lights.add(manta_new(PointLight(Vector(5,5,2), Color(RGBColor(.2,.2,.2)))))
    lights.setAmbientLight(manta_new(ConstantAmbient(Color.black())))
    scene.setLights(lights)

    ###########################################################################
    # Create an empty scene.
    world = manta_new( Group() )
    scene.setObject( world )
    engine.setScene( scene )

    ###########################################################################
    # Load in the model data.

    red = manta_new(Phong(Color(RGBColor(0.6, 0, 0)),
                          Color(RGBColor(0.6,0.6,0.6)),
                          32, 0.4))

    flat = manta_new(Flat(manta_new(NormalTexture())))

    kdtree = manta_new( KDTreeDyn( flat ))
    load_obj_KDTreeDyn( file_name, kdtree )

    ###########################################################################
    # Build the kdtree.
    global leaf_threshold, depth_threshold, cost_threshold, builder_name

    # Try to find the builder class.
    try:
        import manta
        builder_class = getattr(manta,builder_name);
    except AttributeError,e:
        print "Could not find builder class: " + builder_name
        return scene

    print "Using builder: " + builder_name
        
    # Construct the builder.
    builder = builder_class( leaf_threshold,
                             depth_threshold,
                             cost_threshold )

    # Execute the build.
    try:
        builder.build( kdtree )
    except Exception,e:
        print e.type() + " during kdtree build:"
        print e.message()
        return scene

    # Add the kdtree to the scene.
    world.add( kdtree )

    # sphere = manta_new(Sphere(red, Vector(0,0,1.2), 1.0))
    # world.add(sphere)

    # Create the explorer.
    global explorer
    explorer = KDTreeDynFrame( frame, frame.engine, kdtree )

    # Add the explorer to the main frame menu.
    kd_menu = wx.Menu()
    frame.Bind(wx.EVT_MENU, explorer.OnShow,
               kd_menu.Append(wx.NewId(), "Explorer"))
    frame.menuBar.Append( kd_menu, "&KDTreeDyn" );


###############################################################################
###############################################################################
# Main
###############################################################################
###############################################################################
    
def usage():
    print "Usage: python " + sys.argv[0] + " [options]"
    print "Where options contains one or more of:"
    print "-n --np=<threads>"
    print "-f --file=<file name>"
    print "-b --build=<builder class> Default: " + builder_name
    print "   --depth_threshold=<n>"
    print "      Create a leaf if depth is equal to n"
    print "   --leaf_threshold=<n>"
    print "      Create a leaf if the total number of faces is less than n."
    print "   --cost_threshold=<f>"
    print "      Make a leaf if the (cost of splitting)*f > (leaf cost)."

def main():

    global file_name, leaf_threshold, depth_threshold, refinement_threshold, cost_threshold, builder_name

    # Default options.
    num_workers = 1

    # Parse command line options. Note these have to conform to getopt
    # So -np would be parsed as -n <space> p. Use --np=<threads> instead.
    try:
        opts, args = getopt.getopt(sys.argv[1:],
                                   "n:f:b:",
                                   ["np=",
                                    "file=",
                                    "builder=",
                                    "leaf_threshold=",
                                    "depth_threshold=",
                                    "cost_threshold=" ] )

    except getopt.GetoptError:
        usage()
        sys.exit(2)
        
    for o, a in opts:
        try:
            if o in ("-n", "--np"):
                num_workers = int(a)
            elif o in ("-f", "--file"):
                file_name = a
            elif o in ("-b", "--builder"):
                builder_name = a
            elif o in ("--leaf_threshold"):
                leaf_threshold = int(a)
            elif o in ("--depth_threshold"):
                depth_threshold = int(a)
            elif o in ("--cost_threshold"):
                cost_threshold = float(a)
        except ValueError:
            print "Error parsing " + a + " as a number."
            usage()
            sys.exit(2)


        # Add additional command line args here.
        
    ###########################################################################
    # Create the application.
    app = wxManta.MantaApp( initialize_scene,
                            num_workers )


    ###########################################################################
    # Perform any additional setup

    # Start rendering.
    app.MainLoop()

if __name__ == "__main__":
    main()




