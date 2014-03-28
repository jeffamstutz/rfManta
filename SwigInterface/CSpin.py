import wx;

class CSpin(wx.SpinCtrl):
    def __init__(self, parent, id=-1, label="", size=wx.DefaultSize, pos=wx.DefaultPosition, value= 0, min_val=0, max_val=100):
        wx.SpinCtrl.__init__(self,parent,id,label, size)
        self.SetRange(min_val,max_val)
        self.SetValue(value)
        self.Bind(wx.EVT_TEXT, self.OnText)
        self.Bind(wx.EVT_SPINCTRL, self.OnSpin)
        self.color_set = False

    def OnText(self, evt):
        if self.color_set:  
          self.SetBackgroundColour(wx.Colour(150,30,30))
        self.color_set = True
        evt.Skip()

    def OnSpin(self, evt):
        self.SetBackgroundColour(wx.Colour(255,255,255))
        evt.Skip()
        
