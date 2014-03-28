import wx;
import re;

import CSpin
from manta import *
from pycallback import *
from wx.lib.evtmgr import eventManager as EventManager;

import FloatSpin as FS

class ThreadsFrame(wx.Frame):
    def __init__(self, parent, engine):
        wx.Frame.__init__(self, parent=parent, title="Threads")
        
        panel = wx.Panel(self,-1)
        self.engine = engine;
        self.parent = parent;

        vsizer = wx.BoxSizer(wx.VERTICAL);

        # Rendering threads.
        hsizer = wx.BoxSizer(wx.HORIZONTAL);
        hsizer.Add(wx.StaticText(panel,-1,"Rendering threads: "), 0, wx.ALIGN_CENTER);
        spin = CSpin.CSpin(panel,-1, "", (30,50));
        spin.SetRange(1,1024);
        spin.SetValue(engine.numWorkers().value)
        self.Bind(wx.EVT_SPINCTRL, self.OnThreadsSpin, spin )
        hsizer.Add(spin, 0, wx.ALIGN_CENTER );
        hsizer.Add(wx.StaticText(panel,-1,"(%d cores)" % Thread.numProcessors()), 0, wx.ALIGN_CENTER);
        vsizer.Add(hsizer);

        self.Bind( wx.EVT_CLOSE, self.OnClose )

        # Rendering time.
        
        panel.SetSizerAndFit( vsizer );
        self.SetClientSize(panel.GetSize())
        panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        panel.SetFocus()
        self.color_set = 0

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Show(False)
        evt.Skip()

    # Change the number of workers.
    def OnThreadsSpin(self,event):
        np = event.GetInt()
        self.engine.addTransaction("processor count change",
                                   manta_new(createMantaTransaction(self.engine.changeNumWorkers, (np,)) ))        

    def OnClose( self, event ):
        # Don't destroy... just remove it from the screen so it can be used later.
        self.Show( False )
