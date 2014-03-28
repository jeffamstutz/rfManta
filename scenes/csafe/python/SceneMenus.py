import sys, os, time, traceback, types, re, glob
import wx
import SceneInfo
import Histogram
import TransferF
import wxManta
from manta import *
from csafe import *
import wx.lib.scrolledpanel as scrolled
import FloatSpin as FS

class AddRemoveFilesFrame(wx.Frame):
    def __init__(self, parent, id, title, scene):

       wx.Frame.__init__(self, parent, id, title, (wx.GetMouseState().GetX(),wx.GetMouseState().GetY()), (700, 700))
       self.scene = scene
       self.parent = parent;
       panel = scrolled.ScrolledPanel(self,-1, style= wx.TAB_TRAVERSAL)
       self.lb1 = wx.ListBox(panel, -1, (0, 0), (600, 200), [], wx.LB_EXTENDED)

       sizer = wx.BoxSizer(wx.VERTICAL)
       sizer.Add(wx.StaticText(panel,-1, "Particle data nrrds/nhdrs: "))
       sizer.Add(self.lb1, -1, wx.ALL|wx.ALIGN_CENTER, 5)
       hSizer1 = wx.BoxSizer(wx.HORIZONTAL)
       self.addParticlesButton = wx.Button(panel,-1, "Add Particle File(s)")
       self.removeButton = wx.Button(panel, -1,"Remove Particle File")
       if (len(self.scene.nrrdFiles) < 1):
         self.removeButton.Disable()
       hSizer1.Add(self.addParticlesButton, 0, wx.ALL, 3)
       hSizer1.Add(self.removeButton, 0, wx.ALL, 3)
       sizer.Add(hSizer1, 0, wx.ALL|wx.ALIGN_CENTER, 5)

       self.lb2 = wx.ListBox(panel, -1, (0,0), (600, 200), [], wx.LB_EXTENDED)
       sizer.Add(wx.StaticText(panel,-1, "Volume data nrrds/nhdrs: "))
       sizer.Add(self.lb2, -1, wx.ALL|wx.ALIGN_CENTER, 5)
       hSizer2 = wx.BoxSizer(wx.HORIZONTAL)
       
       self.addVolumeButton = wx.Button(panel,-1, "Add Volume File(s)")
       self.removeButton2 = wx.Button(panel, -1, "Remove Volume File")
       if (len(self.scene.nrrdFiles2) < 1):
         self.removeButton2.Disable()
       hSizer2.Add(self.addVolumeButton, 0, wx.ALL, 3)
       hSizer2.Add(self.removeButton2, 0, wx.ALL, 3)
       sizer.Add(hSizer2, 0, wx.ALL|wx.ALIGN_CENTER, 5)   

       self.okButton = wx.Button(panel, wx.ID_OK)
       self.cancelButton = wx.Button(panel, wx.ID_CANCEL)
       hSizer2 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer2.Add(self.okButton, 0, wx.ALL, 3)
       hSizer2.Add(self.cancelButton, 0, wx.ALL, 3)
       sizer.Add(hSizer2, 0, wx.ALL|wx.ALIGN_CENTER, 5)
       
       sizer.Layout()
       panel.SetSizer(sizer)
       panel.SetupScrolling()
       panel.Refresh()
       self.Refresh()
       
       self.Bind(wx.EVT_BUTTON, self.OnClickOk, self.okButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickAddParticles, self.addParticlesButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickAddVolume, self.addVolumeButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickCancel, self.cancelButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickRemove, self.removeButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickRemove2, self.removeButton2)
       self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

       self.lb1.AppendItems(self.scene.nrrdFiles)
       self.lb2.AppendItems(self.scene.nrrdFiles2)

       panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
       panel.SetFocus()

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Close()
        evt.Skip()

    def OnClickOk(self, evt):
      items = []
      #items = self.scene.nrrdFiles
      for i in range(self.lb1.GetCount()):
        print " appending to spheres: " + str(self.lb1.GetString(i))
        items.append(str(self.lb1.GetString(i)))
      self.scene.nrrdFiles = items
      items2 = []
      for i in range(self.lb2.GetCount()):
        print " appending to vols: " + str(self.lb2.GetString(i))
        items2.append(str(self.lb2.GetString(i)))
      self.scene.nrrdFiles2 = items2
      self.Close(True)
    
      print "self.scene.nrrdFiles2: "
      for i in range(len(self.scene.nrrdFiles2)):
        print str(self.scene.nrrdFiles2[i])
      print "end nrrdFiles2."

      if self.lb1.GetCount() != 0 or self.lb2.GetCount() != 0 :
          self.parent.generateMenuItem.Enable( True );

    def OnClickCancel(self, evt):
        self.Close(True)

    def OnClickRemove(self, evt):
        offset = 0
        for i in self.lb1.GetSelections():
            try:
                self.lb1.Delete(i - offset)
                offset+=1
                if self.lb1.GetCount() == 0:
                    self.removeButton.Disable();
            except:
                continue
    
    def OnClickRemove2(self, evt):
      offset = 0
      for i in self.lb2.GetSelections():
          try:
              self.lb2.Delete(i - offset)
              offset+=1
              if self.lb2.GetCount() == 0:
                  self.removeButton2.Disable();
          except:
              continue
        
    def OnClickAddParticles(self, evt):
        wildcard = "All files (*.*)|*.*|" \
                   "Nrrd File (*.nrrd)|*.nrrd|" \
       "Nrrd Header (*.nrhd)|*.nhdr" 
        dlg = wx.FileDialog(self, message="Choose a Particle file(s)",
           defaultDir=os.getcwd(), defaultFile="",wildcard=wildcard,
           style=wx.OPEN | wx.MULTIPLE | wx.CHANGE_DIR)

        dlg.SetFilterIndex( 2 ); # Default to only seeing nhdr files...

        # The FileDialog 'SetDirectory' apparently changes the directory of the whole program...
        #   so hold on to the original so it can be reset.
        orig_path = os.getcwd()

        # See if there is a location that the user wants the file dialog to default to...
        csafe_data_dir = os.environ.get( "CSAFE_DATA_DIR" )
        if csafe_data_dir:
            if os.path.isdir( csafe_data_dir ):
                dlg.SetDirectory( csafe_data_dir );
            else:
                print "WARNING: Environment variable CSAFE_DATA_DIR does not point to a valid directory."

        if dlg.ShowModal() == wx.ID_OK:

            # Enable the remove button for particles
            self.removeButton.Enable()

            paths = dlg.GetPaths()
            selected = self.lb1.GetSelections()
            index = int(self.lb1.GetCount())
            if len(selected) > 0:
                index = int(selected[0])
            for path in paths:
                # See if there is a corresponding volume file: (Search for t????? where ? are digits, eg: t00803)
                pattern = re.compile( "t\d+" )
                result = pattern.search( path )

                if result:
                    full_path = os.path.dirname( path )
                    timestep = result.group()
                    # Note, this is kinda hacky right now... assuming that volumes have .nrrd extension...
                    volume_file = glob.glob( full_path + "/*" + timestep + "*.nrrd" )

                    if len( volume_file ) == 1  : # If we found one (and only one) match.
                        if self.lb2.FindString( volume_file[0] ) == -1 : # Make sure it is not already in the list...
                            self.lb2.Insert( volume_file[0], index)
                            # Enable the remove button for volumes
                            self.removeButton2.Enable()

                self.lb1.Insert(path, index)
                index = index+1

        # Reset back to original path... sigh...
        os.chdir( orig_path )

        print "HERE: " + os.getcwd()
        
        dlg.Destroy()

    def OnClickAddVolume(self, evt):
        wildcard = "All files (*.*)|*.*|" \
           "Nrrd File (*.nrrd)|*.nrrd|" \
     "Nrrd Header (*.nrhd)|*.nhdr"

        dlg = wx.FileDialog(self, message="Choose a Volume file(s)",
           defaultDir=os.getcwd(), defaultFile="",wildcard=wildcard,
           style=wx.OPEN | wx.MULTIPLE | wx.CHANGE_DIR)

        # See if there is a location that the user wants the file dialog to default to...
        csafe_data_dir = os.environ.get( "CSAFE_DATA_DIR" )
        if csafe_data_dir:
            if os.path.isdir( csafe_data_dir ):
                dlg.SetDirectory( csafe_data_dir );

        if dlg.ShowModal() == wx.ID_OK:
            paths = dlg.GetPaths()
            selected = self.lb2.GetSelections()
            index = int(self.lb2.GetCount())
            if len(selected) > 0:
                index = int(selected[0])
            for path in paths:
                self.lb2.Insert(path, index)
                index = index+1

        dlg.Destroy()

    
    def OnCloseMe(self, evt):
        self.Close(True)

    def OnCloseWindow(self, evt):
        self.Destroy()

class BBoxFrame(wx.Frame):
    def __init__(self, parent, id, title, scene):
       wx.Frame.__init__(self, parent, id, title, None, (700, 600))
       self.scene = scene
       panel = wx.Panel(self,-1)
       sizer = wx.BoxSizer(wx.VERTICAL)
       hSizer = wx.BoxSizer(wx.HORIZONTAL)
       hSizer.Add(wx.StaticText(self,-1,"minBound: "))
       self.minXSP = self.addSpinner(panel,scene.minX)
       self.minYSP = self.addSpinner(panel,scene.minY)
       self.minZSP = self.addSpinner(panel,scene.minZ)
       hSizer.Add(self.minXSP,0,wx.ALL,3)
       hSizer.Add(self.minYSP,0,wx.ALL,3)
       hSizer.Add(self.minZSP,0,wx.ALL,3)
       sizer.Add(hSizer, 0, wx.ALL|wx.ALIGN_LEFT, 10)

       hSizer2 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer2.Add(wx.StaticText(panel,-1,"maxBound: "))
       self.maxXSP = self.addSpinner(panel,scene.maxX)
       self.maxYSP = self.addSpinner(panel,scene.maxY)
       self.maxZSP = self.addSpinner(panel,scene.maxZ)
       hSizer2.Add(self.maxXSP,0,wx.ALL,3)
       hSizer2.Add(self.maxYSP,0,wx.ALL,3)
       hSizer2.Add(self.maxZSP,0,wx.ALL,3)
       sizer.Add(hSizer2,0, wx.ALL|wx.ALIGN_LEFT, 10)

       panel.SetSizerAndFit(sizer)
       vs3 = wx.BoxSizer(wx.VERTICAL)
       vs3.Add(panel,0, wx.EXPAND)
       self.SetSizerAndFit(vs3)
       self.SetAutoLayout(True)
       panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
       panel.SetFocus()

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Close()
        evt.Skip()
       

    def addSpinner(self, where, value, id=wx.ID_ANY):
        floatspin = FS.FloatSpin(where, id,
                                 increment=0.001, value=value,
                                 extrastyle=FS.FS_LEFT)
        floatspin.SetFormat("%g")
        floatspin.SetDigits(6)
        floatspin.Bind(FS.EVT_FLOATSPIN, self.OnFloatSpin)
        return floatspin

    def OnFloatSpin(self, event):
        self.scene.minX = self.minXSP.GetValue()
        self.scene.minY = self.minYSP.GetValue()
        self.scene.minZ = self.minZSP.GetValue()
        self.scene.maxX = self.maxXSP.GetValue()
        self.scene.maxY = self.maxYSP.GetValue()
        self.scene.maxZ = self.maxZSP.GetValue()
        min = Vector(float(self.scene.minX), float(self.scene.minY), float(self.scene.minZ))
        max = Vector(float(self.scene.maxX), float(self.scene.maxY), float(self.scene.maxZ))
        if (min[0]>= max[0]):
          min[0] = max[0] - 0.0001
          self.minXSP.SetValue(min[0])
        if (min[1]>= max[1]):
          min[1] = max[1] - 0.0001
          self.minYSP.SetValue(min[1])
        if (min[2]>= max[2]):
          min[2] = max[2] - 0.0001
          self.minZSP.SetValue(min[2])
        self.scene.test.setClippingBBox(min, max)
        self.scene.test.useClippingBBox(True)

        
class VolPositionSizeFrame(wx.Frame):
    def __init__(self, parent, id, title, scene):
       wx.Frame.__init__(self, parent, id, title, None)
       self.scene = scene
       panel = wx.Panel(self,-1)
       sizer = wx.BoxSizer(wx.VERTICAL)
       hSizer = wx.BoxSizer(wx.HORIZONTAL)
       hSizer.Add(wx.StaticText(panel,-1,"Volume Minimum Bound: "))
       min = self.scene.volumeMinBound
       max = self.scene.volumeMaxBound
       size = [ scene.volumeMaxBound[0]-scene.volumeMinBound[0],
               scene.volumeMaxBound[1]-scene.volumeMinBound[1],
               scene.volumeMaxBound[2]-scene.volumeMinBound[2] ]
       position = [scene.volumeMinBound[0] + size[0]/2.0,
                   scene.volumeMinBound[1] + size[1]/2.0,
                   scene.volumeMinBound[2] + size[2]/2.0 ]
               
       self.minXSP = self.addSpinner(panel,min[0])
       self.minYSP = self.addSpinner(panel,min[1])
       self.minZSP = self.addSpinner(panel,min[2])
       hSizer.Add(self.minXSP,0,wx.ALL,3)
       hSizer.Add(self.minYSP,0,wx.ALL,3)
       hSizer.Add(self.minZSP,0,wx.ALL,3)
       sizer.Add(hSizer, 0, wx.ALL|wx.ALIGN_LEFT, 10)

       hSizer2 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer2.Add(wx.StaticText(panel,-1,"Volume Maximum Bound: "))
       self.maxXSP = self.addSpinner(panel,max[0])
       self.maxYSP = self.addSpinner(panel,max[1])
       self.maxZSP = self.addSpinner(panel,max[2])
       hSizer2.Add(self.maxXSP,0,wx.ALL,3)
       hSizer2.Add(self.maxYSP,0,wx.ALL,3)
       hSizer2.Add(self.maxZSP,0,wx.ALL,3)
       sizer.Add(hSizer2,0, wx.ALL|wx.ALIGN_LEFT, 10)

       hSizer3 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer3.Add(wx.StaticText(panel,-1,"Volume Position: "))
       self.posXSP = self.addSpinner(panel,position[0])
       self.posYSP = self.addSpinner(panel,position[1])
       self.posZSP = self.addSpinner(panel,position[2])
       hSizer3.Add(self.posXSP,0,wx.ALL,3)
       hSizer3.Add(self.posYSP,0,wx.ALL,3)
       hSizer3.Add(self.posZSP,0,wx.ALL,3)
       sizer.Add(hSizer3,0, wx.ALL|wx.ALIGN_LEFT, 10)

       hSizer4 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer4.Add(wx.StaticText(panel,-1,"Volume Size: "))
       self.sizeXSP = self.addSpinner(panel,size[0])
       self.sizeYSP = self.addSpinner(panel,size[1])
       self.sizeZSP = self.addSpinner(panel,size[2])
       hSizer4.Add(self.sizeXSP,0,wx.ALL,3)
       hSizer4.Add(self.sizeYSP,0,wx.ALL,3)
       hSizer4.Add(self.sizeZSP,0,wx.ALL,3)
       sizer.Add(hSizer4,0, wx.ALL|wx.ALIGN_LEFT, 10)

       sizer.Layout()
       panel.SetSizerAndFit(sizer)
       vs3 = wx.BoxSizer(wx.VERTICAL)
       vs3.Add(panel,0, wx.EXPAND)
       self.SetSizerAndFit(vs3)
       self.SetAutoLayout(True)

       panel.SetFocus()
       panel.Bind( wx.EVT_CLOSE, self.OnCloseWindow )
       panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Show(False)
        evt.Skip()


    def OnCloseWindow( self, evt ):
        self.Show( False )


    def addSpinner(self, where, value, id=wx.ID_ANY):
        floatspin = FS.FloatSpin(where, id,
                                 increment=0.001, value=value,
                                 extrastyle=FS.FS_LEFT)
        floatspin.SetFormat("%g")
        floatspin.SetDigits(6)
        floatspin.Bind(FS.EVT_FLOATSPIN, self.OnFloatSpin)
        return floatspin

    def OnFloatSpin(self, event):
        if (event.GetId() == self.minXSP.GetId() or event.GetId() == self.maxXSP.GetId()
            or event.GetId() == self.minYSP.GetId() or event.GetId() == self.maxYSP.GetId()
            or event.GetId() == self.minZSP.GetId() or event.GetId() == self.maxZSP.GetId() ):
          minX = self.minXSP.GetValue()
          minY = self.minYSP.GetValue()
          minZ = self.minZSP.GetValue()
          maxX = self.maxXSP.GetValue()
          maxY = self.maxYSP.GetValue()
          maxZ = self.maxZSP.GetValue()
          self.scene.volumeMinBound = [minX, minY, minZ]
          self.scene.volumeMaxBound = [maxX, maxY, maxZ]

          self.posXSP.SetValue(minX + (maxX-minX)/2.0)
          self.posYSP.SetValue(minY + (maxY-minY)/2.0)
          self.posZSP.SetValue(minZ + (maxZ-minZ)/2.0)

          self.sizeXSP.SetValue(maxX-minX)
          self.sizeYSP.SetValue(maxY-minY)
          self.sizeZSP.SetValue(maxZ-minZ)
          
        else :
          posX = self.posXSP.GetValue()
          posY = self.posYSP.GetValue()
          posZ = self.posZSP.GetValue()
          sizeX = self.sizeXSP.GetValue()
          sizeY = self.sizeYSP.GetValue()
          sizeZ = self.sizeZSP.GetValue()

          minX = posX - sizeX/2.0
          maxX = posX + sizeX/2.0
          minY = posY - sizeY/2.0
          maxY = posY + sizeY/2.0
          minZ = posZ - sizeZ/2.0
          maxZ = posZ + sizeZ/2.0
          self.scene.volumeMinBound = [minX, minY, minZ]
          self.scene.volumeMaxBound = [maxX, maxY, maxZ]
          self.minXSP.SetValue(minX)
          self.minYSP.SetValue(minY)
          self.minZSP.SetValue(minZ)
          self.maxXSP.SetValue(maxX)
          self.maxYSP.SetValue(maxY)
          self.maxZSP.SetValue(maxZ)

        min = self.scene.volumeMinBound
        max = self.scene.volumeMaxBound
        pos = Vector(float(min[0] + (max[0] - min[0])/2.0),
                     float(min[1] + (max[1] - min[1])/2.0),
                     float(min[2] + (max[2] - min[2])/2.0))
        size = Vector(float(max[0] - min[0]),
                     float(max[1] - min[1]),
                     float(max[2] - min[2]))
        self.scene.test.setVolumePositionSize(pos, size)
        event.Skip()



class ScenePropertiesFrame(wx.Frame):
    def __init__(self, parent, id, title, scene):
       wx.Frame.__init__(self, parent, id, title, None, (700, 600))
       self.scene = scene
       self.parent = parent;
       panel = scrolled.ScrolledPanel(self,-1, style= wx.TAB_TRAVERSAL)

       sizer = wx.BoxSizer(wx.VERTICAL)
       self.frameText = wx.StaticText(panel,-1, "Go to time: ")
       self.frameButton = wx.Button(panel, -1,"Set")
       self.frameTcl = wx.TextCtrl(panel, -1, "", size=(125, -1))       
       hSizer2 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer2.Add(self.frameText, 0, wx.ALL, 3)
       hSizer2.Add(self.frameButton, 0, wx.ALL, 3) 
       hSizer2.Add(self.frameTcl, 0, wx.ALL, 3)
       sizer.Add(hSizer2, 0, wx.ALL|wx.ALIGN_CENTER, 5)
       
       self.frameText2 = wx.StaticText(panel,-1, "Go to frame: ")
       self.frameButton2 = wx.Button(panel, -1,"Set")
       self.frameTcl2 = wx.TextCtrl(panel, -1, "", size=(125, -1))       
       hSizer8 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer8.Add(self.frameText2, 0, wx.ALL, 3)
       hSizer8.Add(self.frameButton2, 0, wx.ALL, 3) 
       hSizer8.Add(self.frameTcl2, 0, wx.ALL, 3)
       sizer.Add(hSizer8, 0, wx.ALL|wx.ALIGN_CENTER, 5)
       
       if scene.clipFrames == False:
          scene.endFrame = len(scene.nrrdFiles) -1
          if len(scene.nrrdFiles2) > len(scene.nrrdFiles):
            scene.endFrame = len(scene.nrrdFiles2)-1
       if scene.endFrame < 0:
          scene.endFrame = 0
       self.frameText3 = wx.StaticText(panel,-1, "Start/End frames: ")
       self.frameButton3 = wx.Button(panel, -1,"Set")
       self.frameTcl3 = wx.TextCtrl(panel, -1, str(scene.startFrame), size=(125, -1))
       self.frameTcl3_2 = wx.TextCtrl(panel, -1, str(scene.endFrame), size=(125, -1))
       hSizer9 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer9.Add(self.frameText3, 0, wx.ALL, 3)
       hSizer9.Add(self.frameButton3, 0, wx.ALL, 3) 
       hSizer9.Add(self.frameTcl3, 0, wx.ALL, 3)
       hSizer9.Add(self.frameTcl3_2, 0, wx.ALL, 3)
       sizer.Add(hSizer9, 0, wx.ALL|wx.ALIGN_CENTER, 5)
       
       self.checkBox = wx.CheckBox(panel, -1, "No skipping timesteps")
       self.checkBox.SetValue(self.scene.lockFrames)
       sizer.Add(self.checkBox,0,wx.ALL|wx.ALIGN_CENTER,5)
       
       self.checkBox2 = wx.CheckBox(panel, -1, "Loop Animation")
       self.checkBox2.SetValue(self.scene.loop)
       sizer.Add(self.checkBox2,0,wx.ALL|wx.ALIGN_CENTER,5)

       hSizer3 = wx.BoxSizer(wx.HORIZONTAL)
       self.durText = wx.StaticText(panel, 0, "Set duration (seconds): ")
       self.durButton = wx.Button(panel,-1, "Set")
       duration = self.scene.test.getDuration()
       self.durTcl = wx.TextCtrl(panel, 0, str(duration), size=(125, -1))
       hSizer3.Add(self.durText, 0, wx.ALL, 3)
       hSizer3.Add(self.durButton, 0, wx.ALL, 3)
       hSizer3.Add(self.durTcl, 0, wx.ALL, 3) 
       sizer.Add(hSizer3, 0 , wx.ALL|wx.ALIGN_CENTER,5 )
       
       hSizer10 = wx.BoxSizer(wx.HORIZONTAL)
       self.repeatText = wx.StaticText(panel, 0, "Repeat last frame (additional to duration) (seconds): ")
       self.repeatButton = wx.Button(panel,-1, "Set")
       repeat = self.scene.repeatLastFrame
       self.repeatTcl = wx.TextCtrl(panel, 0, str(repeat), size=(125, -1))
       hSizer10.Add(self.repeatText, 0, wx.ALL, 3)
       hSizer10.Add(self.repeatButton, 0, wx.ALL, 3)
       hSizer10.Add(self.repeatTcl, 0, wx.ALL, 3) 
       sizer.Add(hSizer10, 0 , wx.ALL|wx.ALIGN_CENTER,5 )
       
       self.biggifyCB = wx.CheckBox(panel, -1, "Make text bigger")
       self.biggifyCB.SetValue(self.scene.biggify)
       sizer.Add(self.biggifyCB,0,wx.ALL|wx.ALIGN_CENTER,5)

       self.Bind(wx.EVT_CHECKBOX, self.OnClickBigger, self.biggifyCB)
       
       self.labelsCB = wx.CheckBox(panel, -1, "Show labels")
       self.labelsCB.SetValue(self.scene.labels)
       sizer.Add(self.labelsCB,0,wx.ALL|wx.ALIGN_CENTER,5)

       self.Bind(wx.EVT_CHECKBOX, self.OnClickLabels, self.labelsCB)

       self.showSpheresCB = wx.CheckBox(panel, -1, "Show Sphere Data")
       self.showSpheresCB.SetValue(self.scene.showSpheres)
       sizer.Add(self.showSpheresCB,0,wx.ALL|wx.ALIGN_CENTER,5)
       
       self.showVolumeCB = wx.CheckBox(panel, -1, "Show Volume Data")
       self.showVolumeCB.SetValue(self.scene.showVolume)
       sizer.Add(self.showVolumeCB,0,wx.ALL|wx.ALIGN_CENTER,5)

       self.autoHistoCB = wx.CheckBox(panel, -1, "Automatically Add Histograms From NRRDS")
       self.autoHistoCB.SetValue(self.scene.autoBuildHistograms)
       sizer.Add(self.autoHistoCB,0,wx.ALL|wx.ALIGN_CENTER,5)

       self.Bind(wx.EVT_CHECKBOX, self.OnClickAutoHisto, self.autoHistoCB)
       self.Bind(wx.EVT_CHECKBOX, self.OnClickShowSpheres, self.showSpheresCB)
       self.Bind(wx.EVT_CHECKBOX, self.OnClickShowVolume, self.showVolumeCB)

       hSizer4 = wx.BoxSizer(wx.HORIZONTAL)
       self.ridxText = wx.StaticText(panel, -1, "Radius Index (-1 to use one radius): ")
       self.ridxTcl = wx.TextCtrl(panel,-1,str(scene.ridx),size=(125,-1))
       hSizer4.Add(self.ridxText,0,wx.ALL,3)
       hSizer4.Add(self.ridxTcl,0,wx.ALL,3)
       sizer.Add(hSizer4,0,wx.ALL|wx.ALIGN_CENTER,5)         

       hSizer5 = wx.BoxSizer(wx.HORIZONTAL)
       if (self.scene.loaded == True):
          self.radText = wx.StaticText(panel, -1, "Default Radius (Can't change when scene is loaded): ")
          self.radText.SetForegroundColour(wx.Colour(200, 0, 0))
       else:
          self.radText = wx.StaticText(panel, -1, "Default Radius: ")
       self.radTcl = wx.TextCtrl(panel,-1,str(scene.radius),size=(125,-1))
       if (self.scene.loaded == True):
          self.radTcl.Enable(False)
       hSizer5.Add(self.radText,0,wx.ALL,3)
       hSizer5.Add(self.radTcl,0,wx.ALL,3)
       sizer.Add(hSizer5,0,wx.ALL|wx.ALIGN_CENTER,5)

       hSizer6 = wx.BoxSizer(wx.HORIZONTAL)
       self.cidxText = wx.StaticText(panel, -1, "Color Index: ")
       self.cidxTcl = wx.TextCtrl(panel,-1,str(scene.cidx),size=(125,-1))
       hSizer6.Add(self.cidxText,0,wx.ALL,3)
       hSizer6.Add(self.cidxTcl,0,wx.ALL,3)
       sizer.Add(hSizer6,0,wx.ALL|wx.ALIGN_CENTER,5)

       aoCtrlTitle = wx.StaticBox(panel, -1, "Ambient Occlusion")
       aoCtrlSizer = wx.StaticBoxSizer(aoCtrlTitle, wx.VERTICAL)
       self.aoCB = wx.CheckBox(panel, -1, "Use Ambient Occlusion")
       self.aoCB.SetValue(self.scene.useAO)
       aoCtrlSizer.Add(self.aoCB,0,wx.ALL|wx.ALIGN_CENTER,5)
       
       hSizer7 = wx.BoxSizer(wx.HORIZONTAL)
       self.aoCutoffTcl = wx.TextCtrl(panel,-1,str(scene.aoCutoff),size=(125,-1))
       hSizer7.Add(wx.StaticText(panel, -1, "Cutoff Distance: "),0,wx.ALL,3)
       hSizer7.Add(self.aoCutoffTcl,0,wx.ALL,3)
       aoCtrlSizer.Add(hSizer7,0,wx.ALL|wx.ALIGN_CENTER,5)
       
       hSizer8 = wx.BoxSizer(wx.HORIZONTAL)
       self.aoNumTcl = wx.TextCtrl(panel,-1,str(scene.aoNum),size=(125,-1))
       hSizer8.Add(wx.StaticText(panel, -1, "Number of Rays: "),0,wx.ALL,3)
       hSizer8.Add(self.aoNumTcl,0,wx.ALL,3)
       aoCtrlSizer.Add(hSizer8,0,wx.ALL|wx.ALIGN_CENTER,5)
       sizer.Add(aoCtrlSizer,0,wx.ALL|wx.ALIGN_CENTER, 5)

       

       hSizer9 = wx.BoxSizer(wx.HORIZONTAL)
       self.volMinTcl = wx.TextCtrl(panel,-1,str(scene.forceDataMin),size=(125,-1))
       self.volMaxTcl = wx.TextCtrl(panel,-1,str(scene.forceDataMax),size=(125,-1))
       hSizer9.Add(wx.StaticText(panel, -1, "Force Volume Min/Max: "),0,wx.ALL,3)
       hSizer9.Add(self.volMinTcl,0,wx.ALL,3)
       hSizer9.Add(self.volMaxTcl)
       sizer.Add(hSizer9,0,wx.ALL|wx.ALIGN_CENTER, 5)

       hSizer11 = wx.BoxSizer(wx.HORIZONTAL)
       self.stepSizeTcl = wx.TextCtrl(panel,-1,str(scene.stepSize),size=(125,-1))
       hSizer11.Add(wx.StaticText(panel, -1, "Step Size: "),0,wx.ALL,3)
       hSizer11.Add(self.stepSizeTcl,0,wx.ALL,3)
       sizer.Add(hSizer11,0,wx.ALL|wx.ALIGN_CENTER, 5)
        

       self.okButton = wx.Button(panel, wx.ID_OK)
       self.cancelButton = wx.Button(panel, wx.ID_CANCEL)
       hSizer10 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer10.Add(self.okButton, 0, wx.ALL,3)
       hSizer10.Add(self.cancelButton, 0, wx.ALL,3)
       sizer.Add(hSizer10, 0, wx.ALL|wx.ALIGN_CENTER, 5)

       panel.SetSizerAndFit(sizer)

       panel.SetupScrolling()
       vs3 = wx.BoxSizer(wx.VERTICAL)
       vs3.Add(panel,0, wx.EXPAND)
       self.SetSizerAndFit(vs3)
       self.SetAutoLayout(True)

       self.Bind(wx.EVT_BUTTON, self.OnClickTime, self.frameButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickGoToFrame, self.frameButton2)
       self.Bind(wx.EVT_BUTTON, self.OnClickClipFrames, self.frameButton3)
       self.Bind(wx.EVT_BUTTON, self.OnClickDuration, self.durButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickRepeatLastFrame, self.repeatButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickOK, self.okButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickCancel, self.cancelButton)
       self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
       self.Bind(wx.EVT_CHECKBOX, self.OnClickLockFrames, self.checkBox)
       self.Bind(wx.EVT_CHECKBOX, self.OnClickLoop, self.checkBox2)
       self.Bind(wx.EVT_CHECKBOX, self.OnClickAO, self.aoCB)
       panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
       panel.SetFocus()

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Close()
        evt.Skip()

    def OnClickBigger(self, evt):
      self.scene.biggify = evt.IsChecked()
      self.scene.frame.LayoutWindow()
  
    def OnClickLabels(self, evt):
        self.scene.labels = evt.IsChecked()
        self.scene.frame.LayoutWindow()

    def OnClickShowSpheres(self, evt):
      self.scene.showSpheres = evt.IsChecked()
      self.scene.test.setVisibility(self.scene.showSpheres, self.scene.showVolume)

    def OnClickShowVolume(self, evt):
      self.scene.showVolume = evt.IsChecked()
      self.scene.test.setVisibility(self.scene.showSpheres, self.scene.showVolume)

    def OnClickAutoHisto(self, evt):
      self.scene.autoBuildHistograms = evt.IsChecked()
   
    def OnClickAO(self, evt):
        self.scene.test.useAO(evt.IsChecked())
        self.scene.useAO = evt.IsChecked()
       
    def OnClickRepeatLastFrame(self, evt):
        self.scene.repeatLastFrame = float(self.repeatTcl.GetValue())
        self.scene.test.repeatLastFrame(self.scene.repeatLastFrame)
       
    def OnClickLoop(self, evt):
        self.scene.loop = evt.IsChecked()
        self.scene.test.loopAnimations(self.scene.loop)
    
    def OnClickLockFrames(self, evt):
        self.scene.lockFrames = evt.IsChecked()
        self.scene.test.lockFrames(self.scene.lockFrames)
       
    def OnClickClipFrames(self, evt):
        self.scene.startFrame = int(self.frameTcl3.GetValue())
        self.scene.endFrame = int(self.frameTcl3_2.GetValue())
        self.scene.clipFrames = True
        self.scene.test.clipFrames(self.scene.startFrame, self.scene.endFrame)
        
    def OnClickGoToFrame(self, evt):
        frame = int(self.frameTcl2.GetValue())
        self.scene.test.gotoFrame(frame)

    def ApplySettings(self):
      self.scene.radius = float(self.radTcl.GetValue())
      self.scene.ridx = float(self.ridxTcl.GetValue())
      self.scene.cidx = float(self.cidxTcl.GetValue())
      self.scene.aoCutoff = float(self.aoCutoffTcl.GetValue())
      self.scene.aoNum = int(float(self.aoNumTcl.GetValue()))
      self.scene.test.setAOVars(self.scene.aoCutoff, self.scene.aoNum)
      self.scene.forceDataMin = float(self.volMinTcl.GetValue())
      self.scene.forceDataMax = float(self.volMaxTcl.GetValue())
      self.scene.stepSize = float(self.stepSizeTcl.GetValue())
      self.scene.test.setStepSize(float(self.scene.stepSize))
      if (self.scene.forceDataMin != self.scene.forceDataMax):
          self.scene.test.setVolColorMinMax(float(self.scene.forceDataMin), float(self.scene.forceDataMax))

    def OnClickOK(self, evt):
        self.ApplySettings()
        self.Close(True)

    def OnClickCancel(self, evt):
      self.Close(True)  

    def OnClickTime(self, evt):
      time = float(self.frameTcl.GetValue())
      self.scene.test.gotoTime(time)  

    def OnClickDuration(self, evt):
      duration = float(self.durTcl.GetValue())
      self.scene.test.setDuration(duration)
      self.scene.duration = duration

    def OnCloseMe(self, evt):
      self.Close(True)

    def OnCloseWindow(self, evt):
      self.Show(False)


class AddHistogramFrame(wx.Frame):
    def __init__(self, parent, id, title, scene, group, index, name):
       wx.Frame.__init__(self, parent, id, "Add Histogram", None, (700, 500))
       self.transferFID = 0
       self.scene = scene
       self.parent = parent
       self.group = group
       self.index = index
       self.name = name
       panel = wx.Panel(self, -1) 

       sizer = wx.BoxSizer(wx.VERTICAL)

       hSizer1 = wx.BoxSizer(wx.HORIZONTAL)

       #  'Particle' and 'Volume' must remain in this order... or other things have to be fixed (EvtChoice).
       list = ['Particle','Volume']
       text1 = wx.StaticText(panel, -1, "Variable type: ")
       self.ch = wx.Choice(panel,-1,(100, 50), choices=list)
       hSizer1.Add(text1, 0, wx.ALL,3)
       hSizer1.Add(self.ch,0,wx.ALL,3)
       self.Bind(wx.EVT_CHOICE,self.EvtChoice,self.ch)
       sizer.Add(hSizer1,0,wx.ALL|wx.ALIGN_CENTER,5)
       self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

       hSizer2 = wx.BoxSizer(wx.HORIZONTAL)
       self.text2 = text2 = wx.StaticText(panel,-1,"Index in NRRD: ")
       self.indexSP = wx.SpinCtrl(panel, -1, "", (30,50))
       self.indexSP.SetRange(0, 100)
       self.indexSP.SetValue(0)
       hSizer2.Add(text2, 0, wx.ALL,3)
       hSizer2.Add(self.indexSP, 0, wx.ALL,3)
       sizer.Add(hSizer2, 0, wx.ALL|wx.ALIGN_CENTER,5)

       hSizer3 = wx.BoxSizer(wx.HORIZONTAL)
       text3 = wx.StaticText(panel,-1, "Name: ")
       self.nameTC = wx.TextCtrl(panel, -1, name, size=(180,-1))
       hSizer3.Add(text3,0,wx.ALL,3)
       hSizer3.Add(self.nameTC,0,wx.ALL,3)
       sizer.Add(hSizer3,0,wx.ALL|wx.ALIGN_CENTER,5)

       hSizer5 = wx.BoxSizer(wx.HORIZONTAL)
       text4 = wx.StaticText(panel, -1, "Select Transfer Function: ")
       list2 = []
       for i in range(len(self.scene.frame.transferFunctions)):
              list2.append(self.scene.frame.transferFunctions[i].label)
       list2.append('New Transfer Function...')
       self.transferCH = wx.Choice(panel, -1, (100,50), choices=list2)
       hSizer5.Add(text4, 0, wx.ALL, 3)
       hSizer5.Add(self.transferCH,0,wx.ALL,3)
       sizer.Add(hSizer5,0,wx.ALL|wx.ALIGN_CENTER,5)
       self.Bind(wx.EVT_CHOICE, self.EvtChoiceTF, self.transferCH)
       
       
       self.okButton = wx.Button(panel, wx.ID_OK)
       self.cancelButton = wx.Button(panel, wx.ID_CANCEL)
       hSizer4 = wx.BoxSizer(wx.HORIZONTAL)
       hSizer4.Add(self.cancelButton, 0, wx.ALL, 3)
       hSizer4.Add(self.okButton,0,wx.ALL,3)
       sizer.Add(hSizer4, 0, wx.ALL|wx.ALIGN_CENTER, 5)

       
        
       panel.SetSizerAndFit(sizer)

       self.selfSizer = selfSizer = wx.BoxSizer()
       selfSizer.Add(panel, 1, wx.EXPAND)
       self.SetSizerAndFit(self.selfSizer)
       self.SetAutoLayout(True)

       self.Bind(wx.EVT_BUTTON, self.OnClickOK, self.okButton)
       self.Bind(wx.EVT_BUTTON, self.OnClickCancel, self.cancelButton)
       panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
       panel.SetFocus()

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Close()
        evt.Skip()

    def EvtChoiceTF(self, e):
      print "choice"
      if e.GetString() == 'New Transfer Function...':
        print "dlg"
        dlg = wx.TextEntryDialog(self, 'Transfer Function name: ', 'Choose Name', 'Untitled')
        if dlg.ShowModal() == wx.ID_OK:
          name = dlg.GetValue()
          colors = []
          histValues1 = []
          for i in range(0,100):
            histValues1.append(5)
          self.scene.frame.transferFunctions.append(TransferF.TransferF(self, colors, len(self.scene.frame.transferFunctions), name, manta_new(RGBAColorMap(1))))
          self.transferCH.Insert(name, self.transferCH.GetCount()-1)
        dlg.Destroy()
        self.transferFID = self.transferCH.GetCount()-2
        self.transferCH.SetSelection(self.transferFID)
      else:
        self.transferFID = self.transferCH.GetCurrentSelection()
    
    def OnClickOK(self, evt):
      self.name = self.nameTC.GetValue()
      self.index = self.indexSP.GetValue()
      dataMin = -1.1
      dataMax = 2.2
      min = SWIGIFYCreateDouble(0)
      max = SWIGIFYCreateDouble(100)
      dataMin = SWIGIFYGetDouble(min)
      dataMax = SWIGIFYGetDouble(max)
      histValues1 = []
      for i in range(0,100):
          histValues1.append(5)
      colors = []
      #t = TransferF.TransferF(self, colors, self.transferFID, self.name, manta_new(CDColorMap(1)))
      histoGroup = Histogram.HistogramGroup(self.scene.frame.panel, self.scene, self.index, self.name, self.transferFID)
      color = wx.Colour(90,90,90)
      histoGroup.group = self.group
      histoGroup.SetBackgroundColour(color)
      self.scene.frame.histoGroups.append(histoGroup)
      #self.scene.histoVS.Add(histoGroup,0,wx.ALIGN_CENTER,1)       
            #self.scene.frame.vs.Layout()
      self.scene.frame.LayoutWindow()
      self.Close(True)

    def OnClickCancel(self, evt):
        self.Close(True)

    def EvtChoice(self, e):
        self.group = e.GetSelection()
        if self.group == 0:
           self.text2.Show(True)
           self.indexSP.Show(True)
        else:
           self.text2.Show(False)
           self.indexSP.Show(False)
         
        self.SetSizerAndFit(self.selfSizer)
     
    def OnCloseMe(self, evt):
        self.Close(True)

    def OnCloseWindow(self, evt):
        self.Destroy()

