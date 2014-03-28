#!/usr/bin/python

import sys
import getopt
import time
import string
import re
import wx

from wx import glcanvas
import FloatSpin as FS
import wx.py

import threading
import math
import os

from manta import *
from pycallback import *


from MantaCameraPath import *
from CameraFrame import *
from BackgroundFrame import *
from LightFrame  import *
from MiscFrames import *
from ThreadsFrame import *

from wx.lib.evtmgr import eventManager as EventManager;

# import traceback # Use traceback.print_stack() for a stack trace.

# Determine frame resolution.
xres = 512
yres = 512

###############################################################################
###############################################################################
# ParseCommandLine.
# Parses commandline into a list of <option> <list of arguments> pairs
#   option must be preceded by one or more '-' and followed by a space or '=' along
#   with arguments seperated by space or commas.  example  --np=4  --camera=4 5 3 1,2,3 4 4 4
# Example usage:
#   args = wxManta.parseCommandLine(sys.argv)
#   rest = []
#   for o, a in args:
#        if o in ("customOption"):
#            try:
#                cn = int(a[0])
#                print str("customOption: ") + str(int(a[0])) 
#            except ValueError:
#                sys.exit(2)
#        else:
#            rest.append( (o,a) )  #send to MantaApp
#      app = wxManta.MantaApp( initialize_scene,
#                            num_workers, (512,512), False, None, rest )
###############################################################################
###############################################################################
def parseCommandLine(commandLine):
    argvs = commandLine[1:]
    args = []
    j = -1
    for i in range(len(argvs)):
        if (len(argvs[i]) > 0 and argvs[i][0] == "-"):
            temp = (argvs[i].lstrip("-")).rsplit("=")
            temp2 = temp[1:]
            if (len(temp2) > 0):
                temp2 = temp2[0].rsplit(",")
            args.append( (temp[0], temp2) )
            j+=1
        elif(j >= 0):
            args[j][1].append(argvs[i].rsplit(","))
    print str("pcl: args: ") + str(args)
    return args


###############################################################################
###############################################################################
# Default Scene.
# Unlike make_scene, this function doesn't return anything. It must call
# engine.setScene( ... ) explicitly.
#
# engine is of type MantaInterface.
# frame  is of type MantaFrame (see below)
#
###############################################################################
###############################################################################

def createEmptyScenePython( frame, engine ):

    frame.SetTitle( "Empty Scene" );

    # Empty scene.
    scene = manta_new(Scene())
    scene.setBackground(manta_new(ConstantBackground(ColorDB.getNamedColor("white"))))
    scene.getRenderParameters().maxDepth = 5
    
    # Empty object group.
    scene.setObject(manta_new(Group()))

    # Empty light set.
    lights = manta_new(LightSet())
    lights.setAmbientLight(manta_new(ConstantAmbient(Color.black())))
    scene.setLights(lights)
    
    engine.setScene( scene )

def createDefaultScenePython( frame, engine ):

    frame.SetTitle( "Default Scene" );

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
    #plane_matl = manta_new(Lambertian(checker1))
    
    world = manta_new(Group())
    floor = manta_new(Parallelogram(plane_matl, Vector(-20,-20,0),
                                    Vector(40,0,0), Vector(0,40,0)))
    uniformmap = manta_new(UniformMapper())
    floor.setTexCoordMapper(uniformmap)
    world.add(floor)
    world.add(manta_new(Sphere(red, Vector(0,0,1.2), 1.0)))
    scene.setObject(world)
    #
    lights = manta_new(LightSet())
    lights.add(manta_new(PointLight(Vector(0,5,8), Color(RGBColor(.6,.1,.1)))))
    lights.add(manta_new(PointLight(Vector(5,0,8), Color(RGBColor(.1,.6,.1)))))
    lights.add(manta_new(PointLight(Vector(5,5,2), Color(RGBColor(.2,.2,.2)))))
    lights.setAmbientLight(manta_new(ConstantAmbient(Color.black())))
    #
    scene.setLights(lights)
    scene.getRenderParameters().maxDepth = 5

    engine.setScene( scene )

def createPluginScene( frame, engine, scene_parameters ):
    print "scene_parameters = %s" % (scene_parameters,)
    scene = frame.factory.readScene(scene_parameters)
    
class DisplayThread(threading.Thread):
    """Display Thread that will trigger a display event when the manta pipeline is ready to display an image."""
    def __init__(self, notify_window, sync_display):
        threading.Thread.__init__(self)
        self.notify_window = notify_window
        self.sync_display = sync_display
        self.want_abort = False
        self.start()

    def run(self):
        """DisplayThread run function."""
        while(not self.want_abort):
            self.sync_display.waitOnFrameReady();
            # We need to check the state of the thread one more time
            # before triggerring another event (which blocks and keeps
            # the program from exiting properly.
            if (not self.want_abort):
                wx.CallAfter(self.notify_window.FrameReady)

    def abort(self):
        """abort this thread."""
        self.want_abort = True
        
    

class mantaGLCanvas(glcanvas.GLCanvas):
    # updateFramerate is a function that you call to update the framerate
    def __init__(self, parent, sync_display, opengl_display, updateFramerate,
                 size=wx.Size(xres,yres)):

        if (sys.platform == "darwin" or
            (wx.VERSION[0]*10 + wx.VERSION[1]) < 26):
            # This line for OSX
            glcanvas.GLCanvas.__init__(self, parent, -1, style=wx.NO_BORDER, size=size)
        else:
            # This line for linux.
            glcanvas.GLCanvas.__init__(self, parent, -1, attribList=[glcanvas.WX_GL_DOUBLEBUFFER, glcanvas.WX_GL_RGBA, 0], style=wx.NO_BORDER, size=size)

        self.sync_display = sync_display
        self.opengl_display = opengl_display
        # We bind the paint event to allow for some initialization
        # after the window appears.  We'll unbind it after that, since
        # we don't care about expose or "paint me" events.
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.init = 0
        self.prev_time = time.time()
        self.updateFramerate = updateFramerate
        self.framerate = 1;

        self.Bind(wx.EVT_SIZE, self.OnSize)

    def printSize(self, name, size):
        print "%s size = (%d, %d)" % (name, size.x, size.y)

    def OnSize(self, event):
        pass
        event.Skip(True)
        
    def OnPaint(self, event):
        # Note that you must always generate a wxPaintDC object,
        # otherwise certain platforms will not think that you painted
        # the screen and will call enless refreshes.
        dc = wx.PaintDC(self)
        if not self.init:
            self.InitGL()
            self.init = 1
            # Unbind the OnPaint event, since we don't need it anymore.
            self.Bind(wx.EVT_PAINT, None)
        self.clearScreen()

    def clearScreen(self):
        size = self.GetSize()
        self.SetCurrent()
        PureOpenGLDisplay.setGLViewport(0, 0, size.x, size.y)
        PureOpenGLDisplay.clearScreen(0.05, 0.1, 0.2, 0.0)
        self.SwapBuffers()
        
    def InitGL(self):
        self.opengl_display.init()
        self.clearScreen()

    def FrameReady(self):
        self.SetCurrent()
        self.opengl_display.displayImage(self.sync_display.getCurrentImage())
        self.sync_display.doneRendering()
        self.SwapBuffers()
        current_time = time.time()
        delta_time = current_time - self.prev_time;

        self.prev_time = current_time
        self.framerate = 1.0/delta_time

        # if (delta_time > 0.5):
        #    framerate = 1.0/delta_time
        #    self.updateFramerate(framerate)

###############################################################################
###############################################################################
# MANTA FRAME          MANTA FRAME          MANTA FRAME          MANTA FRAME   
###############################################################################
###############################################################################

class MantaFrame(wx.Frame):
    def __init__(self,
                 initialize_callback,
                 num_workers,
                 renderSize,
                 parent=None,
                 title="Manta",
                 ):
        """Note this constructor doesn't Show the frame or call
        Manta's beginRendering, in case a derived class should preform
        additional setup first."""
        
        wx.Frame.__init__(self, parent=parent, title=title)

        # Setup events
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_IDLE, self.OnIdle)

        # Create the StatusBar
        self.statusbar = self.CreateStatusBar()

        # The dialog_id_map holds pointers from the (arbitrary) wxId to the dialog's class
        self.dialog_id_map = {}
        # The dialog_map holds pointers from the (arbitrary) wxId to the instantiation of the class
        self.dialog_map = {}
        
        # Create the menu
        self.menuBar = wx.MenuBar()

        self.manta_menu = wx.Menu()
        self.Bind(wx.EVT_MENU, self.OnAbout,
                  self.manta_menu.Append(wx.NewId(), "About Manta"))

        # Threads.
        dialog_id = wx.NewId()
        self.dialog_id_map[dialog_id] = ThreadsFrame
        self.thread_menu = self.manta_menu.Append(dialog_id, "Threads")
        self.Bind(wx.EVT_MENU, self.OnShowDialog, 
                  self.thread_menu)

        # Python Shell
        dialog_id = wx.NewId()
        self.dialog_id_map[dialog_id] = MantaShellFrame
        self.Bind(wx.EVT_MENU, self.OnShowDialog, 
                  self.manta_menu.Append(dialog_id, "Python Shell"))

        
        self.Bind(wx.EVT_MENU, self.OnCloseWindow,
                  self.manta_menu.Append(wx.NewId(), "&Quit Manta"))
        self.menuBar.Append(self.manta_menu, "&Manta")
        
        self.file_menu = wx.Menu()
        self.Bind(wx.EVT_MENU, self.OnImportPython,
                  self.file_menu.Append(wx.NewId(), "Import &Python"))
        self.menuBar.Append(self.file_menu, "&File")
        
        self.view_menu = wx.Menu()
        self.Bind(wx.EVT_MENU, self.OnAutoView,
                  self.view_menu.Append(wx.NewId(), "&Auto View"))

        # Edit camera dialog.
        dialog_id = wx.NewId()
        self.dialog_id_map[dialog_id] = CameraFrame
        self.Bind(wx.EVT_MENU, self.OnShowDialog, 
                  self.view_menu.Append(dialog_id, "Edit Camera"))

        # Edit Background dialog.
        dialog_id = wx.NewId()
        self.dialog_id_map[dialog_id] = BackgroundFrame
        self.Bind(wx.EVT_MENU, self.OnShowDialog,
                  self.view_menu.Append(dialog_id, "Edit Background"))

        # Camera Path dialog.
        dialog_id = wx.NewId()
        self.dialog_id_map[dialog_id] = MantaCameraPathFrame
        self.Bind( wx.EVT_MENU, self.OnShowDialog,
                   self.view_menu.Append(dialog_id, "Camera Paths"))

        # Capture frames dialog.
        dialog_id = wx.NewId()
        self.dialog_id_map[dialog_id] = MantaCaptureFrame
        self.Bind( wx.EVT_MENU, self.OnShowDialog,
                   self.view_menu.Append(dialog_id, "Capture Frames"))

        self.menuBar.Append(self.view_menu, "&Camera")

        # Edit ligts.
        self.light_menu = wx.Menu()
        
        light_dialog_id = wx.NewId()
        self.dialog_id_map[light_dialog_id] = LightFrame
        self.Bind(wx.EVT_MENU, self.OnShowDialog,
                  self.light_menu.Append(light_dialog_id, "Edit Lights"))
        light_dialog_id = wx.NewId()
        self.menu_lights_visible = self.light_menu.Append(light_dialog_id, "Enable Visible Lights")
        self.Bind(wx.EVT_MENU, self.OnClickToggleVisibleLights, self.menu_lights_visible)
        self.menuBar.Append(self.light_menu, "&Lights")


        self.SetMenuBar(self.menuBar)

        # Create the Main panel that will hold the renderer
        self.panel = wx.Panel(self, size=renderSize)

        #######################################################################
        # Setup manta.

        # Basic settings.
        self.engine = createManta()

        # I thought this would be garbage collected properly, but for
        # some reason on my laptop it crashes in the desctructor.
        self.factory = manta_new(Factory( self.engine ))
        
        self.factory.selectImageType      ("rgba8")
        self.factory.selectLoadBalancer   ("workqueue")
        self.factory.selectImageTraverser ("tiled")
        self.factory.selectPixelSampler   ("singlesample")
        self.factory.selectRenderer       ("raytracer")
        self.factory.selectShadowAlgorithm("hard")

        # Create the camera.
        camera = self.factory.createCamera("pinhole(-eye 3 3 2 -lookat 0 0 0.3 -up 0 0 1 -fov 60)")

        self.engine.changeNumWorkers     (num_workers)

        # Image display.
        use_stereo = False

#        renderFrameType = "glx"
#        renderFrameType = "glx_sync"
        renderFrameType = "ogl"
        display = None
        if (renderFrameType == "glx"):
            # This uses GLX directly
            display = manta_new( OpenGLDisplay( None, self.panel.GetHandle()))
            self.canvas = self.panel
        elif (renderFrameType == "glx_sync"):
            # This uses the GLX with SyncDisplay
            glx_display = manta_new(OpenGLDisplay(None,self.panel.GetHandle()))
            self.sync_display = manta_new(SyncDisplay(vectorStr()))
            display = self.sync_display
            self.sync_display.setChild(glx_display)
            self.sync_thread = DisplayThread(self, self.sync_display)
            self.canvas = self.panel
        elif (renderFrameType == "ogl"):
            # This uses PureOpenGLDisplay
            self.sync_display = manta_new(SyncDisplay(vectorStr()))
            display = self.sync_display
            self.sync_display.setChild(manta_new( NullDisplay(vectorStr()) ))
            opengl_display = manta_new(PureOpenGLDisplay(""))
            self.canvas = mantaGLCanvas(self.panel, self.sync_display,
                                        opengl_display, self.updateFramerate,
                                        size = renderSize)
            self.sync_thread = DisplayThread(self.canvas, self.sync_display)

        # Create a display channel.
        self.channelID = self.engine.createChannel( display, camera,
                                                    use_stereo, renderSize[0], renderSize[1] )

        # Basic scene.
        initialize_callback( self, self.engine ) 


        ############################################################
        # Layout.  This needs to happen *after* we create all the GUI
        # elements, including the canvas.
        box = wx.BoxSizer(wx.HORIZONTAL)
        box.Add(self.panel, 1, wx.EXPAND)
        self.SetSizerAndFit(box)
        self.panel.SetMinSize((20,20))
        self.SetMinSize(self.GetSize() - self.panel.GetSize()
                        + self.panel.GetMinSize())

        #######################################################################
        # Setup the UI events.
        EventManager.Register( self.OnMotion,     wx.EVT_MOTION, self.canvas )
        EventManager.Register( self.OnLeftDown,   wx.EVT_LEFT_DOWN, self.canvas )
        EventManager.Register( self.OnLeftDClick, wx.EVT_LEFT_DCLICK, self.canvas )
        EventManager.Register( self.OnLeftUp,     wx.EVT_LEFT_UP, self.canvas )
        EventManager.Register( self.OnRightDown,  wx.EVT_RIGHT_DOWN, self.canvas )
        EventManager.Register( self.OnRightUp,    wx.EVT_RIGHT_UP, self.canvas )
        EventManager.Register( self.OnMouseWheel, wx.EVT_MOUSEWHEEL, self.canvas )        
        
        
        # Keyboard events are ignored for wx.Frame.
        EventManager.Register( self.OnChar, wx.EVT_KEY_DOWN, self.canvas )

        # Make canvas the initial focus
        wx.CallAfter(self.canvas.SetFocus)


        ##################################################################
        # globals
        self.trackball_radius = 0.8
        self.left_mouse_is_down = False
        self.right_mouse_is_down = False
        self.timeView = None
        self.animationCallbackHandle = None
        
        self.waitingToDeleteLights = False
        self.lightsVisible = False
        self.originalObject = None
        self.lightsRenderables = []
        
    ###########################################################################
    ## toggleVisibleLightsHelper
    ###########################################################################
    def toggleVisibleLightsHelper(self, proc, numProcs):
        for i in range(len(self.lightsRenderables)):
            manta_delete(self.lightsRenderables[i])
        self.lightsRenderables = []
        self.waitingToDeleteLights = False
        
        
    ###########################################################################
    ## toggleVisibleLights
    ###########################################################################
    def toggleVisibleLights(self):
        if (self.waitingToDeleteLights == False):
            if (self.lightsVisible):
                self.menu_lights_visible.SetText("Enable Visible Lights")
                self.lightsVisible = False
                self.engine.getScene().setObject(self.originalObject)
                self.engine.addOneShotCallback(MantaInterface.Relative, 2, manta_new(createMantaOneShotCallback( self.toggleVisibleLightsHelper, ())))
                self.waitingToDeleteLights = True
            else:
                self.menu_lights_visible.SetText("Disable Visible Lights")
                self.lightsVisible = True
                scene = self.engine.getScene()
                group = manta_new(Group())
                self.originalObject = scene.getObject()
                group.add(self.originalObject)
                lights = scene.getLights()
                for i in range(lights.numLights()):
                    #if asPoint(lights.getLight(i)):
                    light = PointLight.fromLight(lights.getLight(i))
                    if light != None:
                        renderable = manta_new( PrimaryRaysOnly(manta_new( Sphere(manta_new( CopyTextureMaterial( light.getColor())), light.getPosition(), 0.5))) )
                        if renderable != None:
                            group.add(renderable)
                            self.lightsRenderables.append(renderable)
                scene.setObject(group)
                
    ###########################################################################
    ## OnToggleVisibleLights
    ###########################################################################
    def OnClickToggleVisibleLights(self, evt):
        self.toggleVisibleLights()
        
    ###########################################################################
    ## OnSize
    ###########################################################################
    def OnSize(self, event):
        panelSize = self.panel.GetSize()
        self.canvas.SetSize(panelSize)
        cbArgs = (self.channelID, panelSize.width, panelSize.height)
        self.engine.addOneShotCallback(MantaInterface.Relative, 0,
                                       manta_new(createMantaOneShotCallback(self.changeResolution, cbArgs)))
        event.Skip(True)

    def changeResolution(self, a, b, channel, new_xres, new_yres):
        self.engine.changeResolution(channel, new_xres, new_yres, True);
        
    ###########################################################################
    ## OnCloseWindow
    ###########################################################################
    def OnCloseWindow(self, event):
        self.ShutdownEngine()

        if self.scene_frame != None :
            func = getattr( self.scene_frame, "shutDown", None )
            if func:
                self.scene_frame.shutDown()
        self.Destroy()

    ###########################################################################
    ## StartEngine
    ##   If a "sceneFrame" is passed in, then sceneFrame.shutDown() will be called
    ## in the event that "quit" is requested.
    ###########################################################################
    def StartEngine( self, sceneFrame = None ):
        self.scene_frame = sceneFrame
        self.Show()
        self.engine.beginRendering(False)

    ###########################################################################
    ## ShutdownEngine
    ###########################################################################
    def ShutdownEngine(self):
        # print "Exiting the engine"
        # sync_thread and sync_display only exist for when we use a
        # SyncDisplay class.  Since this doesn't happen all the time
        # we use the try block to allow continued progress
        try:
            self.sync_thread.abort()
        except: pass
        self.engine.finish()
        try:
            self.sync_display.doneRendering()
            self.sync_display.doneRendering()
        except: pass
        self.engine.blockUntilFinished()
        # print "Engine exited"

    ###########################################################################
    ## OnImportPython
    ###########################################################################
    def OnImportPython(self, event):
        dialog = wx.FileDialog(self, "Open Python Module",
                               os.getcwd(),
                               style=wx.OPEN,
                               wildcard="Python (*.py)|*.py|All files (*.*)|*.*")
        if (dialog.ShowModal() == wx.ID_OK):
            # Obtain the file name.
            filename = dialog.GetPath()

            # Invoke the script during a transaction.
            self.PythonImportTransaction( filename )
        dialog.Destroy()

    ###########################################################################
    ## PythonImportTransaction
    ###########################################################################        
    def PythonImportTransaction( self, filename ):

        self.engine.addTransaction("import python",
                                   manta_new(createMantaTransaction(self.MantaPythonImport, (filename,))))

    ###########################################################################
    ## MantaPythonImport
    ###########################################################################        
    def MantaPythonImport(self, filename):

        # Determine the filename path.
        filepath = os.path.dirname(filename)

        # Add the filename path to the PYTHONPATH
        sys.path.append( filepath )

        # Extract the module name and import the script.
        filename = os.path.splitext(os.path.basename(filename))[0];
        try:
            __import__( filename )
        except(ImportError):
            wx.MessageDialog(self,"Error importing " + filename,"Error",
                             style=wx.OK,
                             pos=wx.DefaultPosition)

        # Remove the file path.
        sys.path.remove( filepath )
        
    ###########################################################################
    ## OnMotion
    ###########################################################################
    def OnMotion( self, event ):
        if ( self.left_mouse_is_down ):
            self.mouseRotate(event)
        elif (self.right_mouse_is_down ):
            self.mousePan(event)
        event.Skip()

    ###########################################################################
    ## OnChar
    ###########################################################################
    def OnChar( self, event ):
        key = event.GetKeyCode()
        if (key < 256):
            key = chr(key)

            # WARNING: Not sure why, but the comparison below MUST be with the capital letter...

            if key == "L":
                LightFrame(self, self.engine).Show()
            elif key == 'C':
                self.engine.addTransaction("output camera",
                                           manta_new(createMantaTransaction(self.showCameraPosition, () )))
            elif key == 'P':
                change = 1
                if (event.ShiftDown()):
                    change = -1
                self.engine.addTransaction("processor count change",
                                           manta_new(createMantaTransaction(self.changeNumWorkers, (self.engine.numWorkers(), change) )))
            elif key == 'I':
                self.toggleVisibleLights()
            elif key == 'Q':
                dialog = wx.MessageDialog( None, 'Are you sure you want to quit?', 'Quit?',
                                           wx.CANCEL | wx.OK | wx.NO_DEFAULT | wx.ICON_EXCLAMATION )
                result = dialog.ShowModal()
                if result == wx.ID_OK :
                    self.Close()
            elif key == 'T':
                self.OnTKey(event)
            elif key == 'V':
                self.OnAutoView(None)
            elif key == ' ':
                # Start or stop animation time.
                self.engine.addTransaction("toggle animation time",
                                           manta_new(createMantaTransaction(self.ToggleAnimationTime,())))
            
            else:
                print "Unknown key '%s'" % key
        else:
            pass
            #print "Found special key '%s'" % key
            
        event.Skip()
        


    ###########################################################################
    ## animationCallback
    ###########################################################################
    def ToggleAnimationTime(self):
        if (self.engine.timeIsStopped()):
            self.engine.startTime();
        else:
            self.engine.stopTime();
        
    ###########################################################################
    ## OnShowDialog
    ###########################################################################
    def OnShowDialog(self, event):
        # Get the dialog
        dialog_id = self.dialog_id_map[event.GetId()]
        if (dialog_id == None):
            wx.LogError("Unknown dialog ID")
            return

        if event.GetId() not in self.dialog_map :
            # Create the dialog
            dialog = dialog_id(self, self.engine)
            self.dialog_map[event.GetId()] = dialog
        else :
            dialog = self.dialog_map[event.GetId()]

        moveToMouse( dialog )
        dialog.Raise()
        dialog.Show( True )

    ###########################################################################
    ## OnAbout
    ###########################################################################
    def OnAbout(self, event):

        message = \
        "The Manta Interactive Ray Tracer\n\n" + \
        "(c) 2005-2009 Scientific Computing and Imaging Institute.\n" + \
        "University of Utah\n\n" + \
        "Revision Information:\n" + getAboutString() + "\n" + \
        "wx.VERSION = %s" % (wx.VERSION,)
                   
        wx.MessageBox( message, "About Manta",
                       wx.TE_MULTILINE | wx.OK | wx.ICON_INFORMATION, self)


    ###########################################################################
    ## OnAutoView
    ###########################################################################
    def OnAutoView(self, event):
        
        # Add the transaction.
        self.engine.addTransaction("auto view",
                                   manta_new(createMantaTransaction(self.MantaAutoView,() )))

    def MantaAutoView(self):

        # Find the bounding box of the scene.
        bounds = BBox();
        self.engine.getScene().getObject().computeBounds( PreprocessContext(), bounds );

        # Invoke the autoview method.
        channel = 0
        self.engine.getCamera(channel).autoview( bounds )
        

    ###########################################################################
    ## FrameReady
    ###########################################################################
    def FrameReady(self):
        self.sync_display.renderFrame()
        self.sync_display.doneRendering()

    ###########################################################################
    ## OnLeftUp
    ###########################################################################
    def OnLeftUp( self, event ):
        self.left_mouse_is_down = False

    ###########################################################################
    ## OnRightDown
    ###########################################################################
    def OnRightDown( self, event ):

        # Set this for the motion event.
        self.right_mouse_is_down = True
        
        mouse_pos = event.GetPosition()

        self.last_x = mouse_pos.x
        self.last_y = mouse_pos.y
        
        event.Skip()

    ###########################################################################
    ## OnRightUp
    ###########################################################################
    def OnRightUp( self, event ):

        self.right_mouse_is_down = False

    ###########################################################################
    ## OnMouseWheel
    ###########################################################################
    def OnMouseWheel( self, event ):

        self.mouseDolly( event )
        

    ###########################################################################
    ## OnLeftDown
    ###########################################################################
    def OnLeftDown( self, event ):
        # Set this for the motion event
        self.left_mouse_is_down = True
        
        mouse_pos = event.GetPosition()
        window_size = self.canvas.GetSize()
        # You need to make sure that the results are floating point
        # instead of integer.
        xpos = 2.0*mouse_pos.x/window_size.width - 1.0
        ypos = 1.0 - 2.0*mouse_pos.y/window_size.height
        self.rotate_from = self.projectToSphere(xpos, ypos,
                                                self.trackball_radius)
        event.Skip()

    ###########################################################################
    ## OnLeftDCLick
    ###########################################################################
    def OnLeftDClick( self, event ):

        # Compute pixel coordinates.
        mouse_pos = event.GetPosition()
        window_size = self.canvas.GetSize()
        xpos = (2.0*mouse_pos.x+1.0)/window_size.width - 1.0
        ypos = 1.0 - (2.0*mouse_pos.y+1.0)/window_size.height

        self.engine.addTransaction("shootOneRay",
                                   manta_new(createMantaTransaction(self.shootOneRay, (xpos, ypos, self.channelID) )))
        
        event.Skip()

    def shootOneRay( self, xpos, ypos, channel_index ):

        # Construct a ray packet.
        data = RayPacketData()
        rays = RayPacket( data, RayPacket.UnknownShape, 0, 1, 0, 0 )

        # Pass a color to store the result.
        color = Color(RGBColor())

        # Shoot a single ray.
        self.engine.shootOneRay( color, rays, xpos, ypos, channel_index )

        if (rays.wasHit(0)):
            lookat = rays.getHitPosition(0)

            # print "Location = (%s, %s, %s)" % (lookat.x(), lookat.y(), lookat.z())
            
            # Change the camera look at position.
            camera = self.engine.getCamera( self.channelID )
            camera.reset( Vector(camera.getPosition()),
                          Vector(camera.getUp()),
                          Vector(lookat)
                          
                          )
        
    ###########################################################################
    ## mouseRotate
    ###########################################################################
    def mouseRotate( self, event ):
        mouse_pos = event.GetPosition()
        window_size = self.canvas.GetSize()
        xpos = 2.0*mouse_pos.x/window_size.width - 1.0
        ypos = 1.0 - 2.0*mouse_pos.y/window_size.height

        to = self.projectToSphere(xpos, ypos, self.trackball_radius);
        trans = AffineTransform()
        trans.initWithRotation(to, self.rotate_from);
        self.rotate_from = to;

        # Obtain the camera.
        channel = 0
        camera = self.engine.getCamera(channel)
        cbArgs = ( trans, Camera.LookAt )
        self.engine.addTransaction("rotate",
                                   manta_new(createMantaTransaction(camera.transform, cbArgs)))
        self.last_x = mouse_pos.x;
        self.last_y = mouse_pos.y;

    ###########################################################################
    ## mousePan
    ###########################################################################
    def mousePan( self, event ):

        # Determine current mouse position.
        mouse_pos = event.GetPosition();
        window_size = self.canvas.GetSize()
        xpos = 2.0*mouse_pos.x/window_size.width - 1.0
        ypos = 1.0 - 2.0*mouse_pos.y/window_size.height

        # Determine previous mouse position.
        prev_xpos = 2.0*self.last_x/window_size.width - 1.0
        prev_ypos = 1.0 - 2.0*self.last_y/window_size.height

        delta_x = prev_xpos - xpos
        delta_y = prev_ypos - ypos

        # Obtain the camera.
        channel = 0
        camera = self.engine.getCamera(channel)
        
        self.engine.addTransaction("pan",
                                   manta_new(createMantaTransaction(camera.translate,
                                                                    (Vector(delta_x, delta_y, 0),))))
        # Update last position.
        self.last_x = mouse_pos.x;
        self.last_y = mouse_pos.y;

    ###########################################################################
    ## mouseDolly
    ###########################################################################
    def mouseDolly(self, event ):
        
        # Get wheel input.
        delta = event.GetWheelDelta()
        rotation = event.GetWheelRotation()/delta
        rotation = rotation * 0.1

        # Invert the scale if going the other way
        if (rotation > 0):
            rotation = rotation/(rotation-1)
        else:
            rotation = -rotation

        # Obtain the camera.
        channel = 0
        camera = self.engine.getCamera(channel)

        # Add transaction.
        self.engine.addTransaction("dolly",
                                   manta_new(createMantaTransaction(camera.dolly, (rotation,))))

    ###########################################################################
    ## projectToSphere
    ###########################################################################
    def projectToSphere(self, x, y, radius):
        x /= radius
        y /= radius
        rad2 = x*x+y*y
        if ( rad2 > 1 ):
            rad = math.sqrt(rad2)
            x /= rad
            y /= rad
            return Vector(x,y,0)
        else:
            z = math.sqrt(1-rad2)
            return Vector(x,y,z)

    ###########################################################################
    ## showCameraPosition
    ###########################################################################
    def showCameraPosition(self):
        data = self.engine.getCamera(0).getBasicCameraData()        
        print "Camera: " + str(data.eye.x()) + ", " + str(data.eye.y()) + ", " + str(data.eye.z())
        print "        " + str(data.lookat.x()) + ", " + str(data.lookat.y()) + ", " + str(data.lookat.z())
        print "        " + str(data.up.x()) + ", " + str(data.up.y()) + ", " + str(data.up.z())

    ###########################################################################
    ## changeNumWorkers
    ###########################################################################
    def changeNumWorkers(self, value, change):
        newProcs = value.value + change
        if (newProcs < 1):
            newProcs = 1
        value.value = newProcs
        
    ###########################################################################
    ## OnTKey
    ###########################################################################
    def OnTKey(self, event):
        if (self.timeView == None):
            self.timeView = TimeViewSampler.fromPixelSampler(self.engine.getPixelSampler())
            if (self.timeView == None):
                self.timeView = manta_new(TimeViewSampler(vectorStr()))
        if (event.ControlDown()):
            factor = 1.1
            if (event.ShiftDown()):
                factor = 1.0/factor
            self.engine.addTransaction("TimeView scale",
                                       manta_new(createMantaTransaction(self.timeView.changeScale, (factor,))))
        else:
            self.engine.addTransaction("TimeView toggle",
                                       manta_new(createMantaTransaction(self.toggleTimeView, (None,) )))

    def toggleTimeView(self, args):
        currentTimeView = TimeViewSampler.toggleTimeView(self.engine, self.timeView)
        if (self.timeView != currentTimeView):
            self.timeView = currentTimeView

    def OnIdle(self,event):
        self.updateFramerate(self.canvas.framerate);
    
    def updateFramerate(self, framerate):
        text = ""
        if (framerate > 1):
            text = "%3.1lf fps" % framerate            
        else:
            text = "%2.2lf fps - %3.1lf spf" % (framerate , 1.0/framerate);

        self.statusbar.SetStatusText(text, 0)
        

###############################################################################
# Helper Functions

def moveToMouse( window ) :
    window.Move( (wx.GetMouseState().GetX(),wx.GetMouseState().GetY()) )
    
def opj(path):
    # copied from demo.py
    """Convert paths to the platform-specific separator"""
    str = apply(os.path.join, tuple(path.split('/')))
    # HACK: on Linux, a leading / gets lost...
    if path.startswith('/'):
        str = '/' + str
    return str

###########################################################################
## usage
##      - prints out error message with supported command line options
###########################################################################
def usage():
    print "Usage: python test.py [options]"
    print "Where options contains one or more of:"
    print "-n --np=<threads>"
    
    
class MantaApp(wx.App) :
    def __init__(self,
                 initialize_callback_ = createDefaultScenePython,
                 num_workers = Thread.numProcessors(),
                 renderSize  = wx.Size(xres,yres),
                 redirect=False,
                 filename=None,
                 commandLine=[], # Unused
                 begin_rendering=True):
        wx.App.__init__(self, redirect, filename)

        # Create a manta frame with the given initialize_callback function.
        self.frame = MantaFrame( initialize_callback_, num_workers, renderSize )

        if (begin_rendering):
            
            # Display the frame and create GL context.
            self.frame.Show()

            # Begin rendering frames.            
            self.frame.StartEngine();
            




