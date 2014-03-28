#
# Set a user defined background
# author: brownleeATcs.utah.edu
# Current options:
#    Constant Color
#
# TODO: currently there is a memory leak, can I delete the old background?
#
#

import wx;
import re;

from manta import *
from pycallback import *
from wx.lib.evtmgr import eventManager as EventManager;

import FloatSpin as FS
import wx.lib.colourselect as csel
import wx.lib.scrolledpanel as scrolled

###############################################################################
###############################################################################
# BACKGROUND FRAME     BACKGROUND FRAME    BACKGROUND FRAME    BACKGROUND FRAME
###############################################################################
###############################################################################

class BackgroundFrame(wx.Frame):
    def __init__(self, parent, engine):
        wx.Frame.__init__(self, parent=parent, title="Background")
        self.engine = engine
        panel = scrolled.ScrolledPanel(self, -1, style=wx.TAB_TRAVERSAL)
        self.vs = vs = wx.BoxSizer(wx.VERTICAL)
        box1_title = wx.StaticBox( panel, label="Constant Background" )
        self.box1 = box1 = wx.StaticBoxSizer( box1_title, wx.VERTICAL )

        self.constantPanel = ConstantBackgroundPanel(panel, engine)
        box1.Add(self.constantPanel, 0, wx.ALIGN_CENTRE|wx.ALL, 0)
        vs.Add(box1, 0, wx.ALIGN_CENTRE|wx.ALL, 0)
        hs = wx.BoxSizer(wx.HORIZONTAL)
        okButton = wx.Button(panel, wx.ID_OK)
        cancelButton = wx.Button(panel, wx.ID_CANCEL)
        applyButton = wx.Button(panel, -1, "Apply")
        hs.Add(okButton, 0, wx.ALIGN_CENTRE|wx.ALL, 0)
        hs.Add(applyButton, 0, wx.ALIGN_CENTRE|wx.ALL, 0)
        hs.Add(cancelButton, 0, wx.ALIGN_CENTRE|wx.ALL, 0)
        vs.Add(hs, 0, wx.ALIGN_CENTRE|wx.ALL, 0)
        self.Bind(wx.EVT_BUTTON, self.OnClickOK, okButton)
        self.Bind(wx.EVT_BUTTON, self.OnClickCancel, cancelButton)
        self.Bind(wx.EVT_BUTTON, self.OnClickApply, applyButton)
        panel.SetSizer(vs)
        panel.SetupScrolling()
        panel.Refresh()
        self.Refresh()
        panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        panel.SetFocus()

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Show(False)
        evt.Skip()
        
    def addSpinner(self, where, value, id=wx.ID_ANY):
        floatspin = FS.FloatSpin(where, id,
                                 increment=0.01, value=value,
                                 extrastyle=FS.FS_LEFT)
        floatspin.SetFormat("%g")
        floatspin.SetDigits(5)
        floatspin.Bind(FS.EVT_FLOATSPIN, self.OnFloatSpin)
        return floatspin
    
    def OnFloatSpin(self, event):
        self.ApplySettings()
        
    def OnClickOK(self, evt):
        self.ApplySettings()
        self.Show(False)

    def OnClickCancel(self, evt):
        self.Show(False)
        
    def OnClickApply(self, evt):
        self.ApplySettings()
        
    def ApplySettings(self):
        self.constantPanel.Apply()
    # self.engine.addTransaction("Apply background settings", manta_new(createMantaTransaction(self.constantPanel.Apply, ())))
    
    def OnCloseWindow(self, evt):
        self.Show(False)
                                            
                                            
class ConstantBackgroundPanel(wx.Panel):
    def __init__(self, parent, engine):
        wx.Panel.__init__(self, parent, -1, (0,0))
        self.engine = engine
        #TODO: memory leak, need callbacks to delete old background
        self.manta_object = manta_new(ConstantBackground(Color(RGBColor(0,0,0))))
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.color1Button = csel.ColourSelect(self, -1, "Background Color")
        sizer.Add(self.color1Button)
        self.SetSizer(sizer)
        self.SetAutoLayout(True)
        self.color1Button.Bind(csel.EVT_COLOURSELECT, self.OnSetColor1)
    def OnSetColor1(self, event):
        color = event.GetValue()
        self.manta_object.setValue(Color(RGBColor(color[0]/255.0, color[1]/255.0, color[2]/255.0)))

    def Apply(self):
        self.engine.getScene().setBackground(self.manta_object)
