#!/usr/bin/python

# Manta/wxPython example script. This script demonstrates how to use
# features of the wxManta GUI. Make a copy and modify it to fit your
# own application.

import sys
import getopt
import re

# This will barf and give warnings if you don't have a new enough
# version.  The frozen bit turns this off for systems like py2app
# where the version is frozen in the system.  See:
# http://wiki.wxpython.org/index.cgi/MultiVersionInstalls
if not hasattr(sys, "frozen"):
    # These two lines are optional, use them if your application requires a specific version of wx.
    import wxversion
    wxversion.ensureMinimal('2.5.3')
    # This will select a particular version if you have more than one installed.
    #wxversion.select("2.5")
import wx

import wxManta
from manta import *

def usage():
    print "Usage: python test.py [options]"
    print "Where options contains one or more of:"
    print "-n --np=<threads>"
    print "-s --scene=\"<scene name>( <scene args > )\""
    print "-r --res=<width>x<height>"
    print "-h --help"
    print "bin/manta compatibility options:"
    print "   --imagetype      <spec>"
    print "   --shadows        <spec>"    
    print "   --imagetraverser <spec>"    
    print "   --loadbalancer   <spec>"
    print "   --pixelsampler   <spec>"    
    print "   --renderer       <spec>"

def main():

    # Default options.
    num_workers = 1
    size = (512, 512)
    sceneCreator = wxManta.createDefaultScenePython

    # Value of None indicates that a default will be used.
    imagetype      = None;
    shadows        = None;
    imagetraverser = None;
    loadbalancer   = None;
    pixelsampler   = None;
    renderer       = None;

    # Parse command line options. Note these have to conform to getopt
    # So -np would be parsed as -n <space> p. Use --np=<threads> instead.
    try:
        opts, args = getopt.getopt(sys.argv[1:],
                                   "n:s:r:",
                                   ["np=",
                                    "scene=",
                                    "imagetype=",
                                    "shadows=",
                                    "imagetraverser=",
                                    "loadbalancer=",
                                    "pixelsampler=",
                                    "renderer=",
                                    "res=",
                                    ] )

    except getopt.GetoptError:
        usage()
        sys.exit(2)

    for o, a in opts:
        if o in ("-n", "--np"):
            if (a == "nprocs"):
                num_workers = Thread.numProcessors(),
            else:
                try:
                    num_workers = int(a)
                except ValueError:
                    usage()
                    sys.exit(2)

        elif o in ("--imagetype"):
            imagetype = str(a)
            
        elif o in ("--shadows"):
            shadows = str(a)            

        elif o in ("--imagetraverser"):
            imagetraverser = str(a)

        elif o in ("--loadbalancer"):
            loadbalancer = str(a)

        elif o in ("--imagetraverser"):
            imagetraverser = str(a)

        elif o in ("--pixelsampler"):
            pixelsampler = str(a)            

        elif o in ("--renderer"):
            renderer = str(a)

        elif o in ("-r", "--res"):
            m = re.match("^(\d+)x(\d+)$", a );
            if (m):
                size = (int(m.group(1)), int(m.group(2)))
            else:
                print "Bad resolution string"
                usage()
                sys.exit(2)
                    
        elif o in ("-s", "--scene"):
            sceneArg = str(a)
            sceneCreator = lambda frame,engine: wxManta.createPluginScene(frame, engine, sceneArg)
        elif o in ("-h", "--help"):
            usage();
            sys.exit(0)

        # Add additional command line args here.

    ###########################################################################
    # Create the application.
    try:            
        app = wxManta.MantaApp( sceneCreator,
                                num_workers=num_workers,
                                renderSize=size,
                                begin_rendering=False
                                )
        
    except Exception, e:
        print e.type() + " occurred initializing Manta."
        print e.message()
        sys.exit(1)

    ###########################################################################
    # Setup manta factory options before starting the renderer.
    factory = Factory(app.frame.engine,True)

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

    # Perform any additional setup here.

    # Start rendering.
    app.frame.StartEngine()
    app.MainLoop()

if __name__ == "__main__":
    main()




