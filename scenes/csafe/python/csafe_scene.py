#! /usr/bin/python

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

""" File: csafe_scene.py
Description:  runs csafe scene.
see http://code.sci.utah.edu/Manta/index.php/CSAFE for instructions

"""

# Import wxManta gui and some system modules.
#import wxversion
#wxversion.ensureMinimal("2.8")

import sys, os

##############################################################################
# Validate that we have a new enough python:

if float(sys.version[0:3]) < 2.4:
  print
  print "ERROR: Python version must be at least 2.4.  (You have " + sys.version[0:3] + ")"
  print
  if os.path.isfile( "/uufs/chpc.utah.edu/sys/pkg/python/std/bin/python" ) :
    print "Try using /uufs/chpc.utah.edu/sys/pkg/python/std/bin/python."
    print
  sys.exit()

##############################################################################

from manta import *
from csafe import *

import wxManta
from   wxManta import moveToMouse

import time, traceback, types
import wx
import wx.lib.scrolledpanel as scrolled

import getopt, re
import wx.lib.buttons
import time, traceback, types
import random
import wx
import Histogram
import TransferF
import SceneMenus
import SceneInfo
import Configuration
import Help

##########################################################################################

ZOOM_HELP_ID     = 208
COLORMAP_HELP_ID = 209


##########################################################################################

"""
    \class window with controls
    \brief a window with controls for manipulating the scene, including histograms, menus, and playback controls
"""
class MyFrame(wx.Frame):
    def __init__(self, parent, ID, title):
        wx.Frame.__init__(self, parent, ID, title, 
        wx.Point(20,20), wx.Size(400, 450))
        self.panel = scrolled.ScrolledPanel(self,-1, style= wx.TAB_TRAVERSAL)
        self.scene = SceneInfo.Scene()
        self.volPosSizeFrame = None
        self.BBoxFrameMenuItem = None

        # CurrentHistogram is the histogram being edited.  currentParticleHistogram may also
        # be the currentHistogram, but always denotes the histogram currently chosen
        # for coloring the particles.
        self.scene.currentHistogram = None
        self.scene.currentParticleHistogram = None
        
        self.SetBackgroundColour(self.scene.bgColor)
        menuBar = wx.MenuBar()
        menuFile = wx.Menu()
        menuFile.Append(102, "&Save Configuration", "")
        menuFile.Append(103, "Save Configuration As...", "")
        menuFile.Append(104, "Load Configuration", "")
        menuFile.AppendSeparator()
        loadUDA = menuFile.Append(110, "Load UDA", "Uintah Dataset")
        loadUDA.Enable(False)
        self.addRemoveDataFilesMenu = menuFile.Append(201, "&Add/Remove NRRD Files", "Add and remove Nrrd files for the scene")
        menuFile.AppendSeparator()
        menuFile.Append(107, "Import NrrdList")
        menuFile.Append(108, "Export NrrdList")
        menuFile.AppendSeparator()
        menuFile.Append(106, "Import Transfer Function")
        menuFile.Append(105, "Export Transfer Function")
        menuFile.AppendSeparator()
        menuFile.Append(109, "&Quit", "")
        
        menuScene = wx.Menu()
        menuScene.Append(204, "Add &Histogram", "Add a histogram to the panel")
        menuScene.Append(202, "Scene Preferences")
        menuScene.Append(205, "Cutting Bounding Box")
        menuScene.Append(206, "Volume Position/Size")

        # Keep track of the generateMenu so it can be enabled/disabled.
        self.generateMenuItem = menuScene.Append(203, "&Generate")

        self.generateMenuItem.Enable( False );
        
        menuHelp = wx.Menu()

        # Keep track of toggleTooltipsMenuItem so it can be updated as needed.
        self.toggleTooltipsMenuItem = menuHelp.Append( 207, "Turn Off Tooltips" )
        menuHelp.AppendSeparator()
        menuHelp.Append(101, "&About", "")
        menuHelp.Append( ZOOM_HELP_ID, "Help on Zooming" )
        menuHelp.Append( COLORMAP_HELP_ID, "Help on Colormaps" )

        menuBar.Append(menuFile,  "File")
        menuBar.Append(menuScene, "Scene")
        menuBar.Append(menuHelp,  "Help")

        self.SetMenuBar(menuBar)

        self.Bind(wx.EVT_MENU, self.Menu101, id=101)
        self.Bind(wx.EVT_MENU, self.Menu102, id=102)
        self.Bind(wx.EVT_MENU, self.Menu103, id=103)
        self.Bind(wx.EVT_MENU, self.Menu104, id=104)
        self.Bind(wx.EVT_MENU, self.Menu105, id=105)
        self.Bind(wx.EVT_MENU, self.Menu106, id=106)
        self.Bind(wx.EVT_MENU, self.Menu201, id=201)
        self.Bind(wx.EVT_MENU, self.Menu202, id=202)
        self.Bind(wx.EVT_MENU, self.Menu107, id=107)
        self.Bind(wx.EVT_MENU, self.Menu108, id=108)
        self.Bind(wx.EVT_MENU, self.MenuQuit, id=109)
        self.Bind(wx.EVT_MENU, self.Menu203, id=203)
        self.Bind(wx.EVT_MENU, self.Menu204, id=204)
        self.Bind(wx.EVT_MENU, self.Menu110, id=110)
        self.Bind(wx.EVT_MENU, self.Menu205, id=205)
        self.Bind(wx.EVT_MENU, self.MenuVolPositionSize, id=206)
        self.Bind(wx.EVT_MENU, self.ToggleTooltips,      id=207)
        self.Bind(wx.EVT_MENU, self.ShowHelp,            id=ZOOM_HELP_ID)
        self.Bind(wx.EVT_MENU, self.ShowHelp,            id=COLORMAP_HELP_ID)
        self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        self.SetBackgroundColour(self.scene.bgColor)
        self.scenePropertiesFrame = None
        self.bboxFrame = None

    def ShowHelp( self, evt ) :
        id = evt.GetId()
        if( id == ZOOM_HELP_ID ):
            Help.showZoomHelp()
        elif( id == COLORMAP_HELP_ID ):
            Help.showColormapHelp()

    def ToggleTooltips( self, evt ):
        if setup.tooltipsOn == True :
            self.toggleTooltipsMenuItem.SetText( "Turn On Tooltips" )
        else :
            self.toggleTooltipsMenuItem.SetText( "Turn Off Tooltips" )
        setup.tooltipsOn = not setup.tooltipsOn
        wx.ToolTip.Enable( setup.tooltipsOn )

    def Menu205(self, evt):
        if( self.bboxFrame == None ) :
          self.bboxFrame = SceneMenus.BBoxFrame(self, -1, "Bounding Box", self.scene)

        moveToMouse( self.bboxFrame )
        self.bboxFrame.Raise()
        self.bboxFrame.Show(True)

    def MenuVolPositionSize(self, evt):
        if( self.volPosSizeFrame == None ) :
            self.volPosSizeFrame = SceneMenus.VolPositionSizeFrame( self, -1,
                                                                    "Volume Position and Size", self.scene )
        moveToMouse( self.volPosSizeFrame )
        self.volPosSizeFrame.Raise()
        self.volPosSizeFrame.Show(True)

    def Menu110(self, evt):
        self.SetFocus()
        dlg = wx.DirDialog(self, message="Open UDA folder", style=wx.OPEN|wx.CHANGE_DIR)
        if dlg.ShowModal() == wx.ID_OK:
            filename = str(dlg.GetPath())
            choices = []
            self.scene.test.readUDAHeader(filename)
            for i in range(self.scene.test.getUDANumVars()):
                choices.append(str(self.scene.test.getUDAVarName(i)))
            dlg2 = wx.SingleChoiceDialog(self, 'Name of Volume Value (must be CCVariable, float):', 'Vol', choices, wx.CHOICEDLG_STYLE)
            if dlg2.ShowModal() == wx.ID_OK:
                volName = str(dlg2.GetStringSelection())
                print "volume selected: " + str(volName)
                self.scene.test.loadUDA(filename, volName)
                t0 = TransferF.TransferF(self, [], 1, "x")
                dataMin = -1.1
                dataMax = 2.2
                min = SWIGIFYCreateDouble(0)
                max = SWIGIFYCreateDouble(100)
                dataMin = SWIGIFYGetDouble(min)
                dataMax = SWIGIFYGetDouble(max)
                histoGroup0 = Histogram.HistogramGroup(self.scene.frame.panel, self.scene, 0, "x")
                self.scene.frame.histoGroups.append(histoGroup0)
                histoGroup1 = Histogram.HistogramGroup(self.scene.frame.panel, self.scene, 1, "y")
                self.scene.frame.histoGroups.append(histoGroup1)
                histoGroup2 = Histogram.HistogramGroup(self.scene.frame.panel, self.scene, 2, "z")
                self.scene.frame.histoGroups.append(histoGroup2)
                
                self.scene.frame.LayoutWindow()
                self.BuildHistograms()
                self.slider.SetRange(1, 1)
                self.scene.mantaApp.frame.StartEngine()
            
    def Menu101(self, evt):
        info = wx.AboutDialogInfo()
        info.Name = "Manta - C-SAFE Particle/Volume Scene"
        info.Version = "0.1"
        info.Copyright = "(C) 2008 Steven G Parker"
        info.Description = "A program for running and modifying particle and volume datasets. By: Steve Parker, James Bigler, Carson Brownlee. MIT License."
        info.WebSite = ("http://code.sci.utah.edu/Manta/index.php/CSAFE", "Manta Home Page")

        # Then we call wx.AboutBox giving it that info object
        wx.AboutBox(info)

    def Menu102(self, evt):
        print "writing file: " + self.scene.sceneName
        if (self.scene.sceneName != "Untitled"):
            Configuration.WriteConfiguration(self.scene, self.scene.sceneName)
        else:
            self.Menu103(evt)
  
    def Menu103(self,evt):
        dlg = wx.FileDialog( self, message="Save file as...", defaultDir=setup.original_path,
                             defaultFile="", wildcard="*", style=wx.SAVE )
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.scene.sceneName = dlg.GetFilename()

            self.scene.sceneWD = dlg.GetDirectory()
            Configuration.WriteConfiguration(self.scene, path)

    def Menu104(self,evt):
        self.SetFocus()
        wildcard = "Config File (*.cfg)|*.cfg|" \
                   "All files (*.*)|*.*" \

        dlg = wx.FileDialog( self, message="Open Configuration File",
                             defaultDir=os.getcwd(),
                             defaultFile="",
                             wildcard=wildcard,
                             style=wx.OPEN|wx.CHANGE_DIR )
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
            Configuration.ReadConfiguration(self.scene, filename)
            self.scene.sceneWD = dlg.GetDirectory()
            self.scene.sceneName = dlg.GetFilename()
            print "filename: " + self.scene.sceneName

    ################ Export Transfer Function #################
    def Menu105(self,evt):
        self.log.write("export transfer function")

    ################ Import Transfer Function #################
    def Menu106(self,evt):
        self.log.write("import transfer function")
        
    ################ Import NRRDLIST #################
    def Menu107(self, e):
        dlg = wx.FileDialog(self, message="Open file",
        defaultDir=os.getcwd(),
        defaultFile="",
        wildcard="*",
        style=wx.OPEN|wx.CHANGE_DIR)
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
            Configuration.ReadNRRDList(self.scene, filename)
        
    ################ Export NRRDLIST #################
    def Menu108(self, e):
        dlg = wx.FileDialog(self, message="Save file as...", defaultDir=setup.original_path,
        defaultFile="", wildcard="*", style=wx.SAVE)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            Configuration.WriteNRRDList(self.scene, path)
        
    ################ Quit #################
    def MenuQuit(self, e):
        self.Close()

    ################ shutDown #################
    def shutDown(self):
        self.scene.mantaApp.frame.Destroy()
        self.Destroy()

    ################ OnCloseWindow #################
    def OnCloseWindow(self, evt):
        self.scene.mantaApp.frame.ShutdownEngine()
        self.scene.mantaApp.frame.Destroy()
        self.Destroy()

    ################ Add/Remove Files #################
    def Menu201(self, evt):
        frame = SceneMenus.AddRemoveFilesFrame(self, -1, "Add/Remove NRRD Files", self.scene)
        frame.Show(True)

    ################ Scene Properties #################
    def Menu202(self, evt):
      if (self.scenePropertiesFrame == None):
        self.scenePropertiesFrame = SceneMenus.ScenePropertiesFrame(self, -1, "Scene Preferences", self.scene)
      self.scenePropertiesFrame.Show(True)
      moveToMouse( self.scenePropertiesFrame )
      self.scenePropertiesFrame.Raise()

    ################ Generate Scene #################
    def Menu203(self, evt):
        self.BuildScene()

    ################ Add Histogram #################
    def Menu204(self, evt):
        group = 0
        name = "untitled"
        index = 0
        frame = SceneMenus.AddHistogramFrame(self,-1, "Add Histogram", self.scene, group, index, name)
        frame.Show(True)
        print "Added histogram: "
        print group
        print index
        print name




    def BuildScene(self):
        self.scene.mantaApp.frame.thread_menu.Enable(False)
        
        info = []
        for i in range(len(self.histoGroups)):
          hist = self.histoGroups[i].histogram
          zoomMin = hist.zoomDMin
          zoomMax = hist.zoomDMax
          cropMin = hist.cropDMin
          cropMax = hist.cropDMax
          colorMin = hist.colorDMin
          colorMax = hist.colorDMax
          absMin = hist.absoluteDMin
          absMax = hist.absoluteDMax
          item = [zoomMin, zoomMax, cropMin, cropMax, colorMin, colorMax, absMin, absMax]
          info.append(item)
        self.generateMenuItem.Enable( False )
        self.addRemoveDataFilesMenu.Enable( False )

        self.test.setRidx(int(self.scene.ridx))
        self.test.setRadius(float(self.scene.radius))
        self.test.clearSphereNrrds();
        self.test.clearVolNrrds();
        print "loading volume nrrds: "
        for i in range(len(self.scene.nrrdFiles2)):
            print str(self.scene.nrrdFiles2[i])
        print "end volume list."
        for i in range(len(self.scene.nrrdFiles2)):
            self.test.addVolNrrd(self.scene.nrrdFiles2[i])
        for  i in range(0, len(self.scene.nrrdFiles)):
            self.test.addSphereNrrd(self.scene.nrrdFiles[i])
        #self.test.loadSphereNrrds()
        self.test.reloadData()

        #self.test.loadVolNrrds()

        if self.scene.autoBuildHistograms:
            if len(self.scene.nrrdFiles2) > 0:
                filenamesplit = self.scene.nrrdFiles2[0].rsplit("_",1)
                name = filenamesplit[0].split(".nrrd")[0]
                names = name.split("/")
                if name == "":
                  name = "volume"
                name = names[len(names)-1]
                index = 0
                found = False
                for i in range(len(self.histoGroups)):
                    if index == self.histoGroups[i].varIndex and self.histoGroups[i].group == 1:
                       found = True
                if found == False:
                  hist = Histogram.HistogramGroup(self.panel,self.scene,index,name,0)
                  hist.group = 1
                  hist.varIndex = 0
                  self.histoGroups.append(hist)
            for i in range(self.test.getNumKeyValuePairs()):
                key = self.test.getKey(i)
                if key.find(".") > 0:
                    name = key.strip(" index:=")
                    index = int(self.test.getValue(i))
                    found = False
                    for i in range(len(self.histoGroups)):
                      if index == self.histoGroups[i].varIndex and self.histoGroups[i].group == 0:
                        found = True
                    if found == False:
                      hist = Histogram.HistogramGroup(self.panel,self.scene,index,name,3)
                      self.histoGroups.append(hist)
                    

          
        self.LayoutWindow()
        self.BuildHistograms()
        self.scene.numFrames = len(self.scene.nrrdFiles)
        if (len(self.scene.nrrdFiles2) > self.scene.numFrames):
          self.scene.numFrames = len(self.scene.nrrdFiles2)
        self.slider.SetRange(1, self.scene.numFrames)
        self.slider.SetValue(1)
        self.numFramesTxt.SetLabel(" / " + str(self.scene.numFrames))
        
        
        if self.scene.currentParticleHistogram == None:
          histo = None
          for j in range(len(self.histoGroups)):
            if self.histoGroups[j].group == 0:
              histo = self.histoGroups[j]
              break
          self.scene.currentParticleHistogram = histo
          if (self.scene.currentHistogram == None):
            self.scene.currentHistogram = histo

        if self.scene.currentVolumeHistogram == None:
          histo = None
          for j in range(len(self.histoGroups)):
            if self.histoGroups[j].group == 1:
              histo = self.histoGroups[j]
              break
          self.scene.currentVolumeHistogram = histo

        if self.scene.currentVolumeHistogram != None:
          self.scene.currentVolumeHistogram.OnClickColor(None)
            
        if self.scene.currentParticleHistogram != None:
          self.scene.currentParticleHistogram.OnClickColor(None)

        self.scene.mantaApp.frame.StartEngine( self )
        
        if self.scene.numFrames > 1:
           self.forwardB.Enable(True)
           self.backB.Enable(True)
           self.playB.Enable(True)
           self.slider.Enable(True)
           
        for i in range(len(info)):
          print "hist values comp: " + str(self.histoGroups[i].histogram.absoluteDMin) + " " + str(info[i][6]) + " " + str(self.histoGroups[i].histogram.absoluteDMax) + " " + str(info[i][7])
          if abs(float(self.histoGroups[i].histogram.absoluteDMin) - float(info[i][6])) < .000001 and abs(float(self.histoGroups[i].histogram.absoluteDMax) - float(info[i][7])) < .000001 and (float(info[i][7]) != 0.0 and float(self.histoGroups[i].histogram.absoluteDMax) != 0.0) and (float(info[i][6]) != 0.0 and float(self.histoGroups[i].histogram.absoluteDMin) != 0.0):
            self.histoGroups[i].SendValues(info[i][0],info[i][1],info[i][2],info[i][3],info[i][4],info[i][5])
        self.scene.loaded = True
        self.scene.mantaApp.frame.OnAutoView(None)
        if self.scene.currentVolumeHistogram != None:
          self.test.setVolCMap(self.scene.currentVolumeHistogram.transferF.cmap)
        self.scene.mantaApp.frame.thread_menu.Enable(True)

    def BuildHistograms(self):
        for i in range(len(self.histoGroups)):
            index = self.histoGroups[i].varIndex
            hist = self.histoGroups[i]
            if (hist.group == 0 and index >= self.test.getNumVars()) or (hist.group == 1 and index > 0):
                print "ERROR: histogram " + hist.title + " index " + str(index) + " out of range"
                continue
            print "building histogram: " + self.histoGroups[i].title
            vol = False
            if self.histoGroups[i].group == 1:
                vol = True
            histValues1Ptr = SWIGIFYCreateIntArray(self.scene.histogramBuckets)
            histValues = []
            min = SWIGIFYCreateFloat(0)
            max = SWIGIFYCreateFloat(100)
            cmin = SWIGIFYCreateFloat(0)
            cmax = SWIGIFYCreateFloat(100)
            if (vol == False):
                self.test.getHistogram(self.histoGroups[i].varIndex,self.scene.histogramBuckets , histValues1Ptr, min, max)
            else:
                self.test.getVolHistogram(self.scene.histogramBuckets, histValues1Ptr,min, max)
            dataMin = SWIGIFYGetFloat(min)
            dataMax = SWIGIFYGetFloat(max)
            print "dataMin/max: " + str(dataMin) + " " + str(dataMax)
            for j in range(self.scene.histogramBuckets):
                histValues.append(SWIGIFYGetIntArrayValue(histValues1Ptr, j))
                # print histValues[j]
            self.histoGroups[i].SetValues(histValues, dataMin, dataMax)
            if vol == False:
                self.test.getSphereDataMinMax(self.histoGroups[i].varIndex, cmin, cmax)
            else:
                self.test.getVolDataMinMax(cmin, cmax)
            dataCMin = SWIGIFYGetFloat(cmin)
            dataCMax = SWIGIFYGetFloat(cmax)
            self.histoGroups[i].SetCMinMax(dataCMin, dataCMax)
            SWIGIFYDestroyIntArray(histValues1Ptr)

    def UpdateColorMap(self, transferF):
        cmap = transferF.cmap
        t = transferF
        slices = vector_ColorSlice()
        t.colors.sort()
        for i in range(0, len(t.colors)):
            slices.push_back(ColorSlice(t.colors[i][0],
                                        RGBAColor(Color(RGBColor(t.colors[i][1], 
                                                                 t.colors[i][2], t.colors[i][3])), t.colors[i][4])))
        if (cmap != None):
            cmap.SetColors(slices)
        self.volCMap.scaleAlphas(float(self.scene.stepSize))
        self.scene.test.updateSphereCMap()

    def OnClick(self, evt):
        None

    def OnKeyDown(self, event):
        None

    def OnClickColor(self, evt):
        dlg = wx.ColourDialog(self)
        dlg.GetColourData().SetChooseFull(True)
        
        if dlg.ShowModal() == wx.ID_OK:
            color = dlg.GetColourData()
            
        dlg.Destroy()

    def OnClickVisible(self, evt):
        if self.visible:
            self.sizer.ShowItems(False)
            self.sizer.Show(self.visibilityB, True, True)
            self.visible = False
        else:
            self.sizer.ShowItems(True)
            self.visible = True

    def InitializeScene(self,frame, engine):
        self.SetBackgroundColour(self.scene.bgColor)
    
    
        # Create a scene object.  (fix this: read from config file if available)
        scene  = manta_new( Scene() )
        eye    = manta_new( Vector(0.340429, 0.161851, -0.441882) )
        lookat = manta_new( Vector( 0.0411403, 0.0475211, 0.0508046) )
        up     = manta_new( Vector(-0.0893698, 0.980804, 0.173311) )
        fov    = 12.0039
        # TODO: be able to set these in GUI
        data = manta_new(BasicCameraData(eye,lookat,up,fov,fov))
        engine.getCamera(0).setBasicCameraData(data)
        # scene.setBackground(manta_new(ConstantBackground(ColorDB.getNamedColor("SkyBlue3").scaled(0.5))))
        scene.setBackground(manta_new(ConstantBackground(Color(RGBColor(0,0,0)))))
        engine.setShadowAlgorithm(manta_new(NoShadows()))
        # engine.selectShadowAlgorithm("noshadows")

        # Create the checker textures.
        checker1 = manta_new( CheckerTexture_Color(Color(RGBColor(.6,.6,.6)),
                                                   Color(RGBColor(0,0,0)),
                                                   Vector(1,0,0),
                                                   Vector(0,1,0)) )

        constant_color1 = manta_new(Constant_Color(Color(RGBColor(.6,.6,.6))))
        checker2 = manta_new(CheckerTexture_ColorComponent(0.2, 0.5, Vector(1,0,0),Vector(0,1,0)))

        # Create the floor shader.
        plane_matl = manta_new(Phong(checker1, constant_color1, 32, checker2))

        # Create a group for the scene.
        world = manta_new(Group())

        # Add the floor
        floor = manta_new(Parallelogram( plane_matl, Vector(-20,-20,0),
                                         Vector(40,0,0), Vector(0,40,0)) )
        uniformmap = manta_new(UniformMapper())
        floor.setTexCoordMapper(uniformmap)
        world.add(floor)
        material = manta_new(Lambertian(Color(RGBColor(1,0,0))))
        world.add(manta_new(Sphere(material, Vector(0,0,1.2), 1.0)))
        scene.setObject(world)

        self.colorMap1 = colorMap1 = manta_new(RGBAColorMap())
        self.volCMap = manta_new(RGBAColorMap(2));


        minBound = Vector(self.scene.volumeMinBound[0], self.scene.volumeMinBound[1], self.scene.volumeMinBound[2])
        maxBound = Vector(self.scene.volumeMaxBound[0], self.scene.volumeMaxBound[1], self.scene.volumeMaxBound[2])
        self.test = manta_new(CDTest(scene, engine, minBound, maxBound));
        self.test.initScene();
        self.test.setVolCMap(self.volCMap);
        self.scene.test = self.test
        self.scene.isPlaying = True
        self.scene.engine = engine

        histValues1Ptr = SWIGIFYCreateIntArray(self.scene.histogramBuckets)
        dataMin1 = -1.1
        dataMax1 = 2.2
        min = SWIGIFYCreateDouble(0)
        max = SWIGIFYCreateDouble(100)
        dataMin1 = SWIGIFYGetDouble(min)
        dataMax1 = SWIGIFYGetDouble(max)
        print "datamin: " + str(dataMin1)  + " datamax: " + str(dataMax1)

        scene.getRenderParameters().maxDepth = 5

        engine.setScene( scene )

        ################
        self.sphereVolCMaps = []
        for j in range(8):
            self.sphereVolCMaps.append(manta_new(RGBAColorMap(2)))
        self.defaultTransferF = TransferF.TransferF(self, [], 1, "empty", manta_new(RGBAColorMap(2)))
        self.transferFunctions = []
        if self.scene.readConfiguration == False:
            id = 0
            self.t0 = TransferF.TransferF(self, [], id, "volume", manta_new(RGBAColorMap(2)))
            self.transferFunctions.append(self.t0)
            self.transferFunctions.append(TransferF.TransferF(self, [], id+1,   "InvRainbowIso", manta_new(RGBAColorMap(0))))
            self.transferFunctions.append(TransferF.TransferF(self, [], id+2, "InvRainbow",    manta_new(RGBAColorMap(1))))
            self.transferFunctions.append(TransferF.TransferF(self, [], id+3, "Rainbow",       manta_new(RGBAColorMap(2))))
            self.transferFunctions.append(TransferF.TransferF(self, [], id+4, "InvGrayscale",  manta_new(RGBAColorMap(3))))
            self.transferFunctions.append(TransferF.TransferF(self, [], id+5, "InvBlackBody",  manta_new(RGBAColorMap(4))))
            self.transferFunctions.append(TransferF.TransferF(self, [], id+6, "BlackBody",     manta_new(RGBAColorMap(5))))
            self.transferFunctions.append(TransferF.TransferF(self, [], id+7, "GreyScale",     manta_new(RGBAColorMap(6))))

        self.tPanel = tPanel = TransferF.TransferFGroup(self.panel, 300, 100, self.defaultTransferF, "empty", self.scene)
        self.scene.tPanel = tPanel
        tPanel.SetBackgroundColour(self.scene.bgColor)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnKeyDown)

        self.vs = vs = wx.BoxSizer( wx.VERTICAL )
        data = []
        for i in range(0, 1000):
            data.append(( random.random() + random.random())/2.0 + 0.0)
        color = self.scene.bgColor
        self.scene.histoVS = hvs = wx.BoxSizer(wx.VERTICAL)
        self.scene.frame = self
        self.histoGroups = []
        vs.Add(hvs,0,wx.ALIGN_TOP|wx.ALL,1)
        vs.Add(tPanel,0,wx.ALIGN_TOP|wx.ALL,1)

        path = setup.csafe_scene_path
        self.backBmp = wx.Bitmap(path+"images/back.png.32x32", wx.BITMAP_TYPE_PNG)
        self.playBmp = wx.Bitmap(path+"images/play.png.32x32", wx.BITMAP_TYPE_PNG)
        self.pauseBmp = wx.Bitmap(path+"images/pause.png.32x32", wx.BITMAP_TYPE_PNG)
        self.forwardBmp = wx.Bitmap(path+"images/forward.png.32x32", wx.BITMAP_TYPE_PNG)

        self.playB = wx.BitmapButton(self.panel, -1, self.pauseBmp, (40,40), style=wx.NO_BORDER)
        self.playB.SetToolTip( wx.ToolTip( "Press to start playback." ) )
        self.playB.SetBackgroundColour(self.scene.bgColor)
        self.playB.Enable(False)
        
        self.forwardB = wx.BitmapButton(self.panel, -1, self.forwardBmp, (20,20), style=wx.NO_BORDER)
        self.forwardB.SetToolTip( wx.ToolTip( "Forward" ) )
        self.forwardB.Enable(False)

        self.backB = wx.BitmapButton(self.panel, -1, self.backBmp, (20,20), style=wx.NO_BORDER)
        self.backB.SetToolTip( wx.ToolTip( "Backward" ) )
        self.backB.Enable( False )

        self.slider = wx.Slider(self.panel, 100, 0, 0, 0, (30, 60), (250, -1), wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | wx.SL_LABELS )
        self.slider.SetTickFreq(1,1)
        self.slider.Enable(False)
        self.numFramesTxt = wx.StaticText(self.panel, -1, str(" / " + str(self.scene.numFrames)), (30,60))
        self.Bind(wx.EVT_SLIDER, self.OnSlider, self.slider)
        self.Bind(wx.EVT_TIMER, self.OnTimer)
        self.timer = wx.Timer(self)
        self.timer.Start(500)

        vs.Layout()

        self.panel.SetBackgroundColour(self.scene.bgColor)
        self.panel.SetSizer(vs)
        self.SetBackgroundColour(self.scene.bgColor)
        self.LayoutWindow()
        # TODO: values hardcoded for now
        # self.scene.ridx = 6
        self.scene.cidx = 4
        self.volCMap.scaleAlphas(0.00125)

    def OnTimer(self, evt):
        frame = self.scene.test.getFrame()
        self.slider.SetValue(frame+1)

    def OnResize(self, evt):
        try:
            size = evt.GetSize()
            self.panel.SetSize(size)
            for i in range(len(self.histoGroups)):
                self.histoGroups[i].SetHistoWidth(size[0]-50)
        except:
            None

    def LayoutWindow(self):
        size = self.GetSize()
        self.vs = vs = wx.BoxSizer( wx.VERTICAL )
        self.scene.histoVS = hvs = wx.BoxSizer(wx.VERTICAL)
        for i in range(len(self.histoGroups)):
            hvs.Add(self.histoGroups[i], 0,wx.ALIGN_TOP|wx.ALIGN_CENTER, 5)
        hvs.Add(self.tPanel, 0, wx.ALIGN_TOP,5 )
        vs.Add(hvs,0,wx.ALIGN_TOP|wx.ALIGN_CENTER,5)

        hvs2 = wx.BoxSizer(wx.HORIZONTAL)
        # animCtrlTitle = wx.StaticBox(self.panel, -1, "")
        # animCtrlSizer = wx.StaticBoxSizer(animCtrlTitle, wx.HORIZONTAL)
        hvs2.Add(self.backB,0,wx.ALIGN_CENTER,2)
        hvs2.Add(self.playB,0,wx.ALIGN_CENTER,2)
        hvs2.Add(self.forwardB,0,wx.ALIGN_CENTER,2)
        vs.Add(hvs2,0,wx.ALIGN_BOTTOM|wx.ALIGN_CENTER)
        hs = wx.BoxSizer(wx.HORIZONTAL)
        hs.Add(self.slider, 0, wx.ALIGN_BOTTOM|wx.ALIGN_CENTER, 10)
        hs.Add(self.numFramesTxt, 0, wx.ALIGN_CENTER, 1)
        vs.Add(hs, 0, wx.ALIGN_BOTTOM|wx.ALIGN_CENTER, 1)
        #TODO: can't get the staticbox to friggin work with the buttons
#        vs.Add( animCtrlSizer, 0, wx.ALIGN_CENTRE|wx.ALL, 5 )
        #if (self.scene.mantaFrame != None):
        #    vs.Add(self.scene.mantaFrame.panel, 0,wx.ALIGN_BOTTOM)
        
        self.Bind(wx.EVT_BUTTON, self.OnClickBack, self.backB)
        self.Bind(wx.EVT_BUTTON, self.OnClickPlay, self.playB)
        self.Bind(wx.EVT_BUTTON, self.OnClickForward, self.forwardB)
        vs.AddSpacer((0,10))
        vs.Layout()
        self.panel.SetSizer(vs)
        self.panel.SetupScrolling()
        self.panel.Refresh()
        self.Refresh()

    def OnSlider(self, e):
        if (int(self.slider.GetValue()) > 0):
            self.test.gotoFrame(int(self.slider.GetValue()) - 1)

    def OnClickBack(self, evt):
        self.test.backAnimation()

    def OnClickPlay(self, evt):
        if self.scene.isPlaying:
            self.test.pauseAnimation()
            self.scene.isPlaying = False
            self.playB.SetBitmapFocus(self.playBmp)
            self.playB.SetBitmapLabel(self.playBmp)
            self.playB.SetBitmapDisabled(self.playBmp)
            self.timer.Stop()
        else:
            self.test.resumeAnimation()
            self.scene.isPlaying = True
            self.playB.SetBitmapFocus(self.pauseBmp)
            self.playB.SetBitmapLabel(self.pauseBmp)
            self.playB.SetBitmapDisabled(self.pauseBmp)
            self.timer.Start(500)

    def OnClickForward(self, evt):
        self.test.forwardAnimation()

class Setup(wx.Object):
    def __init__(self):
        self.csafe_scene_path = sys.path[0] + "/"
        self.original_path = os.getcwd()
        self.num_workers = None
        self.cfg = ""
        self.nrrdlist = ""
        self.generate = False
        self.uda = ""
        self.udaVol = ""
        self.imagetype = None
        self.shadows = None
        self.imagetraverser = None
        self.loadbalancer = None
        self.pixelsampler = None
        self.renderer = None
        self.size = (512,512)
        self.tooltipsOn = True
        
setup  = Setup()

def initialize_scene( frame, engine ):
    None


def usage( badArgs=[] ):

    if len( badArgs) != 0 :
        print ""
        print "Error parsing command line.  The following argument(s) where not recognized:"
        print ""
        for arg in badArgs :
            print "  " + arg
    print ""
    print "Usage: python test.py [options]"
    print ""
    print "Where options contains one or more of:"
    print ""
    print "  --res=WxH           : resolution of render window"
    print "  --np=<int>          : the number of threads to use"
    print "  --cfg=<string>      : load a configuration file"
    print "  --nrrdlist=<string> : load a nrrdlist"
    print "  --g                 : automatically/immediately generate scene when starting up"
    print ""
    sys.exit( 2 )

def main():
    global setup
    
    # Parse command line options. Note these have to conform to getopt
    # So -np would be parsed as -n <space> p. Use --np=<threads> instead.

    try:
        opts, args = getopt.getopt(sys.argv[1:],
                                   "n:s:r:",
                                   ["cfg=",
                                    "g",
                                    "help",
                                    "imagetraverser=",
                                    "imagetype=",
                                    "loadbalancer=",
                                    "np=",
                                    "nrrdlist=",
                                    "pixelsampler=",
                                    "renderer=",
                                    "res=",
                                    "scene=",
                                    "shadows="
                                    ] )

    except getopt.GetoptError, error:
        usage( [error.msg] )

    if len( args ) > 0 :
        usage( args )

    for o, a in opts:

        if o in ("-n", "--np"):
            if (a == "nprocs"):
                setup.num_workers = Thread.numProcessors(),
            else:
                try:
                    setup.num_workers = int(a)
                except ValueError:
                    usage( [a] )

        elif o in ("--imagetype"):
            setup.imagetype = str(a)
            
        elif o in ("--shadows"):
            setup.shadows = str(a)            

        elif o in ("--imagetraverser"):
            setup.imagetraverser = str(a)

        elif o in ("--loadbalancer"):
            setup.loadbalancer = str(a)

        elif o in ("--imagetraverser"):
            setup.imagetraverser = str(a)

        elif o in ("--pixelsampler"):
            setup.pixelsampler = str(a)            

        elif o in ("--renderer"):
            setup.renderer = str(a)

        elif o in ("-r", "--res"):
            m = re.match("^(\d+)x(\d+)$", a );
            if (m):
                setup.size = (int(m.group(1)), int(m.group(2)))
            else:
                usage( ["Bad resolution string:", a] )
        elif o in ("-h", "--help"):
            usage();

        # Add additional command line args here.
        elif o in ("--cfg"):
            setup.cfg = str(a)
        elif o in ("--nrrdlist"):
            setup.nrrdlist = str(a)
        elif o in ("--uda"):
            setup.uda = str(a.split()[0])
            setup.udaVol = str(a.split()[1])
        elif o in ("--g"):
            setup.generate = True
        else:
            usage( [o] )
        
    ###########################################################################
    # Create the application.
    if (setup.num_workers == None):
      setup.num_workers = 4
    app = wxManta.MantaApp( initialize_scene,
                            setup.num_workers, renderSize=setup.size, begin_rendering=False)

    factory = Factory(app.frame.engine, True)

    if (setup.imagetype):
        if (not factory.selectImageType( setup.imagetype )):
            print "Invalid image type, choices:"
            for key in factory.listImageTypes():
                print key
            sys.exit(2)
        
    if (setup.shadows):
        if (not factory.selectShadowAlgorithm( setup.shadows )):
            print "Invalid shadow algorithm, choices:"
            for key in factory.listShadowAlgorithms():
                print key
            sys.exit(2)
    
    if (setup.imagetraverser):
        if (not factory.selectImageTraverser( setup.imagetraverser )):
            print "Invalid image traverser, choices:"
            for key in factory.listImageTraversers():
                print key
            sys.exit(2)
            
    if (setup.loadbalancer):
        if (not factory.selectLoadBalancer( setup.loadbalancer )):
            print "Invalid load balancer, choices:"
            for key in factory.listLoadBalancers():
                print key
            sys.exit(2)
            
    if (setup.pixelsampler):
        if (not factory.selectPixelSampler( setup.pixelsampler )):
            print "Invalid pixel sampler, choices:"
            for key in factory.listPixelSamplers():
                print key
            sys.exit(2)
            
    if (setup.renderer):
        if (not factory.selectRenderer( setup.renderer )):
            print "Invalid renderer, choices:"
            for key in factory.listRenderers():
                print key        
            sys.exit(2)


    frame1 = MyFrame(None, -1, "C-SAFE Particle/Volume Visualizer")
    frame1.scene.mantaFrame = app.frame
    frame1.Show(True)
    frame1.InitializeScene(app.frame, app.frame.engine)
    frame1.LayoutWindow()
    frame1.scene.mantaApp = app

    if (setup.cfg != ""):
        Configuration.ReadConfiguration(frame1.scene, setup.cfg)
        frame1.scene.sceneName = setup.cfg
        frame1.scene.sceneWD = setup.original_path

        if( len( frame1.scene.nrrdFiles ) > 0 or len( frame1.scene.nrrdFiles ) > 0 ) :
            frame1.generateMenuItem.Enable( True );
            
    if (setup.nrrdlist != ""):
        Configuration.ReadNRRDList(frame1.scene, setup.nrrdlist)
    if (setup.uda != ""):
        print "UDA Loading currently disabled, use NRRDs instead"
        if False:
            frame1.scene.test.readUDAHeader(str(setup.uda))
            frame1.scene.test.loadUDA(str(setup.uda), str(setup.udaVol))
            t0 = TransferF.TransferF(frame1, [], 1, "x")
            histoGroup0 = Histogram.HistogramGroup(frame1.scene.frame.panel, frame1.scene, 0, "x")
            frame1.scene.frame.histoGroups.append(histoGroup0)
            histoGroup1 = Histogram.HistogramGroup(frame1.scene.frame.panel, frame1.scene, 1, "y")
            frame1.scene.frame.histoGroups.append(histoGroup1)
            histoGroup2 = Histogram.HistogramGroup(frame1.scene.frame.panel, frame1.scene, 2, "z")
            frame1.scene.frame.histoGroups.append(histoGroup2)

            frame1.scene.frame.LayoutWindow()
            frame1.BuildHistograms()
            frame1.slider.SetRange(1, 1)
            frame1.scene.mantaApp.frame.StartEngine()

    frame1.scene.numThreads = setup.num_workers
    frame1.scene.engine.changeNumWorkers(frame1.scene.numThreads)
    if (setup.generate == True):
        frame1.BuildScene()
    
    if( ( frame1.scene.currentParticleHistogram != None ) and
        ( frame1.scene.currentHistogram != frame1.scene.currentParticleHistogram ) ) :
        # Update the particle historgram to the one read in from the configure file.
        frame1.scene.currentParticleHistogram.OnClickColor( evt=None ) # FIX ME... Is there a better way to do this? (OnClickColor does too much, I think)
        frame1.scene.currentParticleHistogram.transferF.UpdateColorMap()

    if( frame1.scene.currentHistogram != None ) :
        # Update the current historgram to the one read in from the configure file.
        frame1.scene.currentHistogram.OnClickColor( evt=None ) # FIX ME... Is there a better way to do this?
        frame1.scene.currentHistogram.transferF.UpdateColorMap()

    ###########################################################################
    # Perform any additional setup

    #  initialize_scene2(app.frame, app.frame.engine, app)
    # Start rendering.
    app.MainLoop()

###############################################################################

if __name__ == "__main__":

    main()
    
