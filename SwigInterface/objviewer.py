#
#  For more information, please see: http://software.sci.utah.edu
#
#  The MIT License
#
#  Copyright (c) 2005-2006
#  Scientific Computing and Imaging Institute, University of Utah
#
#  License for the specific language governing rights and limitations under
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#

# Import wxManta gui and some system modules.
import wxManta
import getopt, sys


# Import the manta module, the mantainterface module which was %import'ed
# into swig/example.i is automatically included by the manta module.
from manta import *

from pycallback import *

filename = ""
ignore_vn = False
default_material = None;
triangle_type = MeshTriangle.KENSLER_SHIRLEY_TRI

# Re-create the default scene using the example texture.
def initialize_scene( frame, engine ):

    # Create a scene object.
    scene = manta_new(Scene())
    scene.setBackground(manta_new(ConstantBackground(ColorDB.getNamedColor("white"))))

    # Load an obj file
    
    global filename, ignore_vn
    print "File: " + filename
    
    try:
        obj = manta_new( ObjGroup( filename, default_material, triangle_type ) )
    except InputError,e:
        print "Error: " + e.message()
        exit(1)

    # Turn off vertex normals (code appears broken in some models)
    if (ignore_vn):
        obj.discardVertexNormals();
        
    # Create a bvh.
    bvh = manta_new( DynBVH() )
    bvh.setGroup( obj )
    bvh.rebuild()
    
    # scene.setObject(world)
    scene.setObject( bvh )
    
    # Lights.
    lights = manta_new(LightSet())
    lights.add(manta_new(HeadLight(1.0, Color(RGBColor(.8,.8,.9)) )))
    lights.setAmbientLight(manta_new(ConstantAmbient(Color.black())))
    #
    scene.setLights(lights)
    scene.getRenderParameters().maxDepth = 5

    engine.setScene( scene )

def usage():
    print "Usage: python test.py [options]"
    print "Where options contains one or more of:"
    print "-n --np=<threads>"
    print "-f --file=<filename"
    print "   --ignore_vn   Ignore vertex normals in file."

def main():

    # Default options.
    num_workers = 1
    
    # Value of None indicates that a default will be used.
    camera         = "pinhole( -normalizeRays -createCornerRays )";
    imagetype      = None;
    shadows        = "noshadows";
    imagetraverser = "tiled( -tilesize 8x8 -square )";
    loadbalancer   = None;
    pixelsampler   = None;
    renderer       = None;
    autoview       = False;

    # Parse command line options. Note these have to conform to getopt
    # So -np would be parsed as -n <space> p. Use --np=<threads> instead.
    try:
        opts, args = getopt.getopt(sys.argv[1:], "n:f:", ["np=",
                                                          "file=",
                                                          "camera=",
                                                          "imagetype=",
                                                          "shadows=",
                                                          "imagetraverser=",
                                                          "loadbalancer=",
                                                          "pixelsampler=",
                                                          "renderer=",
                                                          "ignore_vn"]
                                   )

    except getopt.GetoptError,e:
        print e
        usage()
        sys.exit(2)

    global filename, ignore_vn

    for o, a in opts:
        if o in ("-n", "--np"):
            try:
                num_workers = int(a)
            except ValueError:
                usage()
                sys.exit(2)
                
        elif o in ("-f", "--file"):
            filename = a
            
        elif o in ("--ignore_vn"):
            ignore_vn = True;
            
        elif o in ("--camera"):
            camera = str(a).replace(';',' ');
            
        elif o in ("--imagetype"):
            imagetype = str(a).replace(';',' ');
            
        elif o in ("--shadows"):
            shadows = str(a).replace(';',' ');

        elif o in ("--imagetraverser"):
            imagetraverser = str(a).replace(';',' ');

        elif o in ("--loadbalancer"):
            loadbalancer = str(a).replace(';',' ');

        elif o in ("--imagetraverser"):
            imagetraverser = str(a).replace(';',' ');

        elif o in ("--pixelsampler"):
            pixelsampler = str(a).replace(';',' ');

        elif o in ("--renderer"):
            renderer = str(a).replace(';',' ');
        # Add additional command line args here.
        

    ###########################################################################
    # Create the application.
    app = wxManta.MantaApp( initialize_scene,
                            num_workers )

    factory = Factory(app.frame.engine,True)

    if (camera):
        cam = factory.createCamera(camera);
        if (not cam):
            print "Invalid camera, choices:"
            for key in factory.listCameras():
                print key
            sys.exit(2)
        else:
            app.frame.engine.setCamera(0,cam);


    if (imagetype):
        if (not factory.selectImageType( imagetype )):
            print "Invalid image type, choices:"
            for key in factory.listImageTypes():
                print key
            sys.exit(2)
        
    if (shadows):
        if (not factory.selectShadowAlgorithm( shadows )):
            print "Invalid shadow algorithm, choices:"
            for key in factory.listShadowAlgorithms():
                print key
            sys.exit(2)
    
    if (imagetraverser):
        if (not factory.selectImageTraverser( imagetraverser )):
            print "Invalid image traverser, choices:"
            for key in factory.listImageTraversers():
                print key
            sys.exit(2)
            
    if (loadbalancer):
        if (not factory.selectLoadBalancer( loadbalancer )):
            print "Invalid load balancer, choices:"
            for key in factory.listLoadBalancers():
                print key
            sys.exit(2)
            
    if (pixelsampler):
        if (not factory.selectPixelSampler( pixelsampler )):
            print "Invalid pixel sampler, choices:"
            for key in factory.listPixelSamplers():
                print key
            sys.exit(2)
            
    if (renderer):
        if (not factory.selectRenderer( renderer )):
            print "Invalid renderer, choices:"
            for key in factory.listRenderers():
                print key        
            sys.exit(2)

    ###########################################################################
    # Perform any additional setup
    # cbArgs = ( manta_new( TiledImageTraverser( 64, 64 ) ), )
    # app.frame.engine.addTransaction("set image traverser",
    #                            manta_new(createMantaTransaction(app.frame.engine.setImageTraverser, cbArgs)))


    
    # app.frame.engine.setShadowAlgorithm( manta_new( HardShadows( True ) ) )

    # Start rendering.
    app.MainLoop()

if __name__ == "__main__":
    main()


