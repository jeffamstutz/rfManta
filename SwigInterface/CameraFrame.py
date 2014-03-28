import wx;
import re;

from manta import *
from pycallback import *
from wx.lib.evtmgr import eventManager as EventManager;

import FloatSpin as FS

###############################################################################
###############################################################################
# CAMERA FRAME          CAMERA FRAME          CAMERA FRAME          CAMERA FRAME
###############################################################################
###############################################################################

class CameraFrame(wx.Frame):
    def __init__(self, parent, engine):
        wx.Frame.__init__(self, parent=parent, title="Camera")
        panel = wx.Panel(self,-1)        
       
        data = engine.getCamera(0).getBasicCameraData()
        eye = data.eye
        lookat = data.lookat
        up = data.up
        hfov = data.hfov
        vfov = data.vfov

        self.engine = engine
        self.parent = parent
        
        vsizer = wx.BoxSizer(wx.VERTICAL)
        gsizer = wx.GridBagSizer(6,4);
        
        self.channelST = wx.StaticText(panel, -1, "Channel: ")
        self.channelSP = wx.SpinCtrl(panel, -1, "", (30,50))
        self.channelSP.SetRange(0,100)
        self.channelSP.SetValue(0)
        gsizer.Add(self.channelST, (0,0))
        gsizer.Add(self.channelSP, (0,1))

        self.eyeST = wx.StaticText(panel, -1, "eye: " )
        self.eyeXSP = self.addSpinner(panel, eye.x())
        self.eyeYSP = self.addSpinner(panel, eye.y())
        self.eyeZSP = self.addSpinner(panel, eye.z())
        gsizer.Add(self.eyeST, (1,0))
        gsizer.Add(self.eyeXSP, (1,1))
        gsizer.Add(self.eyeYSP, (1,2))
        gsizer.Add(self.eyeZSP, (1,3))
        
        self.lookatST =  wx.StaticText(panel, -1, "lookat: ")
        self.lookXSP = self.addSpinner(panel, lookat.x())
        self.lookYSP = self.addSpinner(panel, lookat.y())
        self.lookZSP = self.addSpinner(panel, lookat.z())
        # self.lookZTCL = wx.TextCtrl(panel, -1, str(lookat.z()), size=(125,-1))	
        gsizer.Add(self.lookatST, (2,0))
        gsizer.Add(self.lookXSP, (2,1))
        gsizer.Add(self.lookYSP, (2,2))
        gsizer.Add(self.lookZSP, (2,3))
        
        self.upST = wx.StaticText(panel, -1, "up: ")
        self.upXSP = self.addSpinner(panel, up.x())
        self.upYSP = self.addSpinner(panel, up.y())
        self.upZSP = self.addSpinner(panel, up.z())
        gsizer.Add(self.upST, (3,0))
        gsizer.Add(self.upXSP, (3,1))
        gsizer.Add(self.upYSP, (3,2))
        gsizer.Add(self.upZSP, (3,3))
        
        gsizer.Add(wx.StaticText(panel, -1, "fov horiz: "), (4,0))
        self.fovXSP = self.addSpinner(panel, hfov)
        gsizer.Add(self.fovXSP, (4,1))

        gsizer.Add(wx.StaticText(panel, -1, "vert: "), (4,2))
        self.fovYSP = self.addSpinner(panel, vfov)
        gsizer.Add(self.fovYSP, (4,3))
        vsizer.Add(gsizer, 0, wx.ALIGN_CENTER )

        vsizer.Add(wx.StaticText(panel,-1,"Enter camera description:"), 0, wx.ALIGN_LEFT );

        self.stringTCL = wx.TextCtrl(panel, -1, "camera string", size=(-1,50), style=wx.TE_MULTILINE)
        self.ComputeString()
        self.stringB = wx.Button(panel, -1, "Set")
        vsizer.Add(self.stringTCL, 1, wx.EXPAND )
        vsizer.Add(self.stringB, 0, wx.ALIGN_RIGHT)

        hsizer = wx.BoxSizer( wx.HORIZONTAL );
        self.okButton = wx.Button(panel, wx.ID_OK)
        self.applyButton = wx.Button(panel, wx.ID_APPLY)
        hsizer.Add(self.okButton, 0, wx.ALIGN_CENTER)
        hsizer.Add(self.applyButton, 0, wx.ALIGN_CENTER)
        
        closeButton = wx.Button(panel, wx.ID_CLOSE)
        hsizer.Add(closeButton, 0, wx.ALIGN_CENTER|wx.ALL, 0)
        self.Bind(wx.EVT_BUTTON, self.OnCloseWindow, closeButton)
        vsizer.Add(hsizer, 0, wx.ALIGN_CENTER|wx.EXPAND);

        #vsizer.Add(panel,0,wx.EXPAND)
        panel.SetSizerAndFit(vsizer);
        self.SetClientSize(panel.GetSize())
        
        self.Bind(wx.EVT_BUTTON, self.OnClickOK, self.okButton)        
        self.Bind(wx.EVT_BUTTON, self.OnClickApply, self.applyButton)
        self.Bind(wx.EVT_BUTTON, self.OnClickSet, self.stringB)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        # Double bind mouse events in the Manta canvas.
        EventManager.Register( self.OnManipulation, wx.EVT_MOTION, self.parent.canvas );
        EventManager.Register( self.OnManipulation, wx.EVT_MOUSEWHEEL, self.parent.canvas );

        self.SetAutoLayout(True)
        panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        panel.SetFocus()

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Show(False)
        evt.Skip()

    def OnManipulation(self,event):

        data = self.engine.getCamera(0).getBasicCameraData();
        self.UpdateBasicCameraData(data);
        
    def UpdateBasicCameraData(self,data):

        eye = data.eye
        lookat = data.lookat
        up = data.up
        hfov = data.hfov
        vfov = data.vfov

        self.eyeXSP.SetValue( eye.x() );
        self.eyeYSP.SetValue( eye.y() );
        self.eyeZSP.SetValue( eye.z() );

        self.lookXSP.SetValue( lookat.x() );
        self.lookYSP.SetValue( lookat.y() );
        self.lookZSP.SetValue( lookat.z() );

        self.upXSP.SetValue( up.x() );
        self.upYSP.SetValue( up.y() );
        self.upZSP.SetValue( up.z() );

        self.fovXSP.SetValue( hfov );
        self.fovYSP.SetValue( vfov );
        
        self.ComputeString()
        
    def OnClickSet(self, evt):

        print "Note this still doesn't match the standard basic camera format"
        
        p = re.compile('(-?\d+\.?\d*)')
        s = p.findall(self.stringTCL.GetValue())
        print s
        try:
            self.eyeXSP.SetValue(float(s[0]))
            self.eyeYSP.SetValue(float(s[1]))
            self.eyeZSP.SetValue(float(s[2]))
            self.lookXSP.SetValue(float(s[3]))
            self.lookYSP.SetValue(float(s[4]))
            self.lookZSP.SetValue(float(s[5]))
            self.upXSP.SetValue(float(s[6]))
            self.upYSP.SetValue(float(s[7]))
            self.upZSP.SetValue(float(s[8]))
            self.fovXSP.SetValue(float(s[9]))
            self.fovYSP.SetValue(float(s[10]))
        except ValueError:
            print "incorrect string value"
            
    def ComputeString(self):
        
        ex = float(self.eyeXSP.GetValue())
        ey = float(self.eyeYSP.GetValue())
        ez = float(self.eyeZSP.GetValue())
        
        lx = float(self.lookXSP.GetValue()) #float(self.lookXTCL.GetValue())
        ly = float(self.lookYSP.GetValue())
        lz = float(self.lookZSP.GetValue())
        
        ux = float(self.upXSP.GetValue())
        uy = float(self.upYSP.GetValue())
        uz = float(self.upZSP.GetValue())
        hfov = float(self.fovXSP.GetValue())
        vfov = float(self.fovYSP.GetValue())
        
        s = '-eye %f %f %f -lookat %f %f %f -up %f %f %f -hfov %f -vfov %f' % (ex,ey,ez,lx,ly,lz,ux,uy,uz,hfov,vfov)
        self.stringTCL.SetValue(s)
        
        
    def addSpinner(self, where, value, id=wx.ID_ANY):
        floatspin = FS.FloatSpin(where, id,
                                 increment=0.01, value=value,
                                 extrastyle=FS.FS_LEFT)
        floatspin.SetFormat("%g")
        floatspin.SetDigits(5)
        floatspin.Bind(FS.EVT_FLOATSPIN, self.OnFloatSpin)
        return floatspin
    
    def OnFloatSpin(self, event):
        # Pull out the new value
        # spinner = event.GetEventObject()
        # x = float(spinner.GetValue())
        self.ApplySettings()
        self.ComputeString()
        
    def OnClickOK(self, evt):
        self.ApplySettings()
        self.Show(False)
        
    def OnClickApply(self, evt):
        self.ApplySettings()
        
    def ApplySettings(self):

        channel = int(self.channelSP.GetValue())
        ex = float(self.eyeXSP.GetValue())
        ey = float(self.eyeYSP.GetValue())
        ez = float(self.eyeZSP.GetValue())
        
        lx = float(self.lookXSP.GetValue()) #float(self.lookXTCL.GetValue())
        ly = float(self.lookYSP.GetValue())
        lz = float(self.lookZSP.GetValue())
        
        ux = float(self.upXSP.GetValue())
        uy = float(self.upYSP.GetValue())
        uz = float(self.upZSP.GetValue())
        
        eye = manta_new(Vector(ex,ey,ez))
        lookat = manta_new(Vector(lx,ly,lz))
        up = manta_new(Vector(ux, uy,uz))
        fovx = float(self.fovXSP.GetValue())
        fovy = float(self.fovYSP.GetValue())
        
        data = manta_new(BasicCameraData(eye, lookat, up, fovx, fovy))

        # Send a transaction to manta to update the camera.
        cbArgs = ( data, )
        self.engine.addTransaction("Set basic camera data",
                                   manta_new(createMantaTransaction(self.engine.getCamera(channel).setBasicCameraData, cbArgs)))
        
        
    def OnCloseWindow(self, evt):

        # I don't think we want to do this, as we aren't killing the window anymore...
        #   Disconnect from events...
        #   EventManager.DeregisterListener( self.OnManipulation );
        
        self.Show( False )
        
                                            
                                            
