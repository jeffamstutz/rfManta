
import wx;
import wx.py;

class MantaShellFrame(wx.Frame):
    def __init__(self, parent, engine):
        wx.Frame.__init__(self, parent=parent, title="Manta Shell", size=(512,512))

        self.frame  = parent;
        self.engine = engine;

        # Add a pycrust window.
        vsizer = wx.BoxSizer( wx.VERTICAL );
        self.crust = wx.py.crust.Crust( self, -1 );
        vsizer.Add(self.crust,1,wx.EXPAND);
        self.SetSizer( vsizer );

        # Setup locals in the shell.
        self.crust.shell.interp.locals['frame']  = self.frame
        self.crust.shell.interp.locals['engine'] = self.engine

    
