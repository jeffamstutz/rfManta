import sys, os, time, traceback, types
import wx
import Histogram
import random

from manta       import *
from csafe       import *
from csafe_scene import setup
from wxManta     import opj

colors = [] # (pos, r, g, b, a)

class TransferF(wx.Object):
    def __init__(self, parent, colorsn, id, title="untitled", cmap = None):
        self.parent = parent
        self.colors = colorsn
        self.id = id
        if (cmap != None):
		self.colors = []
		num = cmap.GetNumSlices()
		for i in range(num):
			slice = cmap.GetSlice(i)
			c = slice.color.color
			a = float(slice.color.a)
			p = float(slice.value)
			c1 = float(SWIGIFYGetColorValue(c, 0))
			c2 = float(SWIGIFYGetColorValue(c,1))
			c3 = float(SWIGIFYGetColorValue(c,2))
			self.colors.append((float(p), float(c1), float(c2), float(c3), float(a)))
        empty = False
        if len(self.colors) < 1.0:
            empty = True
            self.colors.append((0.0, 0, 0, 0, 1))
            self.colors.append((1.0, 1, 1, 1, 1))
        if cmap == None:
            if empty == False:
                slices = manta_new(vector_ColorSlice())
                for i in range(len(colorsn)):
                    slices.push_back(manta_new(ColorSlice(float(colorsn[i][0]), manta_new(RGBAColor(float(colorsn[i][1]), float(colorsn[i][2]), float(colorsn[i][3]), float(colorsn[i][4]))))))
                cmap = manta_new(RGBAColorMap(slices))
            else:
                cmap = manta_new(RGBAColorMap(1))
        self.colors.sort()
        self.label = title
	self.cmap = cmap

    def Clone(self, t):
        if t == self:
            return
        self.colors = []
        for i in range(len(t.colors)):
          self.colors.append(t.colors[i])
        self.colors.sort()
        self.UpdateColorMap()

    def UpdateColorMap(self):
	self.parent.UpdateColorMap(self)    
   
    def GetLabel(self):
        return self.label
            
    def MoveColor(self, index, pos):
        c = (  pos, self.colors[index][1],self.colors[index][2], self.colors[index][3], self.colors[index][4] ) 
        # c = ( 0.5 , 1, 0, 0, 1)
        self.colors[index] = c
        # self.colors.sort()
            
    def AddColor(self, color, pos):
        if (len(color) == 3):
            self.colors.append( (  pos, color[0], color[1], color[2], 1.0 ) )
        elif (len(color) == 4):
                self.colors.append( (  pos, color[0], color[1], color[2], color[3] ) )
        else:
            blowuphorribly
        # self.colors.sort()
        
    def SetColor(self, index, color):
        pos = self.colors[index][0]
        if (len(color) == 3):
           c = (  pos, color[0], color[1], color[2], 1.0 ) 
        elif (len(color) == 4):
                c =  (  pos, color[0], color[1], color[2], color[3] ) 
        else:
            blowuphorribly
        
        self.colors[index] = c
        
        
    def GetColor(self, pos):  # color at position pos, in range [0,1]
        colors = []
        for i in range(len(self.colors)):
            colors.append( (self.colors[i][0], self.colors[i][1], self.colors[i][2], self.colors[i][3], self.colors[i][4] ) )
        colors.sort()
        
        if len(colors) < 1:
            return (0,0,0,1)
        index1 = 0
        index2 = 0
        for i in range(0, len(colors)):
            if (colors[i][0] <= pos):
                index1 = i
            if (colors[i][0] >= pos and index2 == 0):
                index2 = i
        if (pos < colors[0][0]):
            index2 = 0
        if (colors[len(colors) - 1][0] < pos):
            index2 = len(colors) - 1
        
        pos1 = colors[index1][0]
        pos2 = colors[index2][0]
        amt1 = amt2 = 0.5
        length =  pos2 - pos1
        if length > 0.0: 
            amt1 = (1.0 - (pos - pos1)/length)
            amt2 = (1.0 - (pos2 - pos)/length)
        if index1 == index2:
            amt1 = amt2 = 0.5
        a1 = colors[index1][4]
        a2 = colors[index2][4]
            
        color = (colors[index1][1]*amt1 + colors[index2][1]*amt2, colors[index1][2]*amt1 + colors[index2][2]*amt2, \
            colors[index1][3]*amt1 + colors[index2][3]*amt2, colors[index1][4]*amt1 + colors[index2][4]*amt2)
        return color
        
    def GetColorAtIndex(self, index):  # get the non interpolated color value
        colord = self.colors[index]
        return (colord[1], colord[2], colord[3], colord[4])
        
        
    def GetColorRGB(self, pos):
        color = self.GetColor(pos)
        return (color[0]*color[3]*255.0, color[1]*color[3]*255.0, color[2]*color[3]*255.0)
        
        # return ( ( 0, 0, 0, 0) )
        
    def RemoveColor(self, index):
        self.colors.pop(index)
        

class TransferFPanel(wx.Panel):
    def __init__(self, parent, width, height, transferF, scene, updateFunction=None):

        path = setup.csafe_scene_path
        self.scene = scene
        self.backgroundIMG = wx.Image(opj(path+'images/bckgrnd.png'), wx.BITMAP_TYPE_PNG).ConvertToBitmap()
        self.mouse_value = 0.0
        self.paddingW = 20.0
        self.paddingH = 20.0
        self.transferF = transferF
        self.width = width
        self.height = height
        self.parentC = parent
        self.zoomMin = 0.0  # zoom into min value, [0,1]
        self.zoomMax = 1.0
        self.zoomDMin = 0.0  # data value of zoomMin/max
        self.zoomDMax = 1.0
        self.absoluteDMin = 0.0  # min/max data values
        self.absoluteDMax = 1.0
        self.updateFunction = updateFunction
        panel = wx.Panel.__init__(self, parent, -1, (0, 0), (width + self.paddingW, height + self.paddingH) )
        wx.EVT_PAINT(self, self.OnPaint)
        self.histogramGroup = None
        self.Update()
        self.Bind(wx.EVT_LEFT_DOWN, self.OnClick)
        self.Bind(wx.EVT_LEFT_UP, self.OnLeftUp)
        self.Bind(wx.EVT_RIGHT_UP, self.OnRightClick)
        self.Bind(wx.EVT_MOTION, self.OnMotion)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        # self.SetFocus()
        self.colorSelectorWidth = 10.0
        self.selected = None
        self.dSelected = None
        self.SetBackgroundColour(wx.Colour(90,90,90))
        self.Bind( wx.EVT_MOUSEWHEEL, self.OnMouseWheel )

    def OnMouseWheel(self, evt):
        pos = float(evt.GetPosition().x - self.paddingW/2.0)/float(self.width)
        delta = evt.GetWheelDelta()
        rot = evt.GetWheelRotation()/delta
        # zoom in if rot > 0, out if ro < 0
        zoomRange = self.zoomMax - self.zoomMin
        zoomAmount = 0.75 # the smaller the more zooming
        if (rot > 0):
#            self.zooms.append( (self.zoomDMin, self.zoomDMax))
            self.zoomMin = (pos-zoomAmount*pos)*zoomRange + self.zoomMin
            self.zoomMax = (pos+zoomAmount*(1.0-pos))*zoomRange + self.zoomMin
        if (rot < 0):
            self.zoomMin -= (pos-zoomAmount*pos)*zoomRange
            self.zoomMax += (pos+zoomAmount*(1.0-pos))*zoomRange
        
        if (self.zoomMin < 0.0):
           self.zoomMin = 0.0
        if (self.zoomMax > 1.0):
           self.zoomMax = 1.0
            
        self.Update()
        
    def SetUpdateFunction(self, function):
        self.updateFunction = function
        
    def SetTransferF(self, transferF):
        self.transferF = transferF
        self.Update()
        
        
    def OnKeyDown(self, evt):
        if evt.GetKeyCode() == wx.WXK_DELETE and self.dSelected != None:
            self.transferF.RemoveColor(self.dSelected)
            self.Update()
        self.UpdateHistogram()
    
    def OnRightClick(self, evt):
        zoomRange = self.zoomMax-self.zoomMin
        x = evt.GetPosition().x - self.paddingW/2.0
        y = evt.GetPosition().y - self.paddingH/2.0
        pos = float(x)/float(self.width)*zoomRange + self.zoomMin
        # did they click on a color picker?
        clicked = False
        index = -1
        for i in range(len(self.transferF.colors)):
            colorx = (self.transferF.colors[i][0] - self.zoomMin)/zoomRange
            if abs(x - colorx*self.width) < self.colorSelectorWidth/2.0:
                clicked = True 
                index = i
        
        if clicked:
            c = self.transferF.colors[index]
            color = wx.ColourData()
            color.SetColour(wx.Colour(c[1]*255, c[2]*255, c[3]*255))
            dlg = wx.ColourDialog(self, color)
            dlg.GetColourData().SetChooseFull(True)
            
            if dlg.ShowModal() == wx.ID_OK:
		originalC = self.transferF.GetColorAtIndex(index)
                data = dlg.GetColourData()
                c = data.GetColour().Get()
                color = []
                color.append(c[0])
                color.append(c[1])
                color.append(c[2])
		color.append(originalC[3])
                color[0] /= 255.0
                color[1] /= 255.0
                color[2] /= 255.0
                self.transferF.SetColor(index, color)
                self.Update()
                self.Refresh()
                
            dlg.Destroy()
            
        else: # add a new one
            dlg = wx.ColourDialog(self)
            dlg.GetColourData().SetChooseFull(True)
            data = dlg.GetColourData()
            
            if dlg.ShowModal() == wx.ID_OK:
                data = dlg.GetColourData()
                c = data.GetColour().Get()
                color = []
                color.append(c[0])
                color.append(c[1])
                color.append(c[2])
		color.append(1.0 - float(y)/float(self.height))
                color[0] /= 255.0
                color[1] /= 255.0
                color[2] /= 255.0
                self.transferF.AddColor( color, pos)
                self.Update()
                
            dlg.Destroy()
        self.UpdateHistogram()
            
    
    def SetHistogramGroup(self, histo):
        if (histo.group == 0): # group 0 == Particles.
          if self.scene.currentParticleHistogram != None:
            self.scene.currentParticleHistogram.Deselect()
          histo.Select()
        else:
          if self.scene.currentVolumeHistogram != None:
              self.scene.currentVolumeHistogram.Deselect()
          histo.Select()
        self.histogramGroup = histo
        
    def UpdateHistogram(self):
        self.transferF.UpdateColorMap()
        for i in range(len(self.scene.frame.histoGroups)):
            histoGroup = self.scene.frame.histoGroups[i]
            if histoGroup.transferF == self.transferF:
              histoGroup.Update()
    
    def OnClick(self, evt):
        zoomRange = self.zoomMax-self.zoomMin
        x = evt.GetPosition().x - self.paddingW/2.0
        if self.selected == None:
            index = -1
            for i in range(len(self.transferF.colors)):
                colorx = (self.transferF.colors[i][0] - self.zoomMin)/zoomRange
                if abs(x - colorx*self.width) < self.colorSelectorWidth/2.0:
                    clicked = True 
                    index = i
            if index >= 0:
                self.selected = index
                self.dSelected = index
            else:
                self.dSelected = None
                self.selected = None
            self.Refresh()
        
    def OnLeftUp(self, evt):
        self.selected = None
        self.UpdateHistogram()
        
    def OnMotion(self, evt):
        zoomRange = self.zoomMax-self.zoomMin
        x = evt.GetPosition().x - self.paddingW/2.0
	if (x < 0.0):
	    x = 0.0
	if x > self.width:
	    x = self.width
        zoomDRange = self.zoomDMax - self.zoomDMin
        self.mouse_value = float(x)/float(self.width)*float(zoomDRange)+float(self.zoomDMin)
        self.Refresh()
        y = evt.GetPosition().y - self.paddingH/2.0
	if y < 0.0:
	    y = 0.0
	if y > self.height:
	    y = self.height

        # Determine if this is a Particle historgram... and if so, don't allow modification of the
        # opacity.
        if self.histogramGroup != None and self.histogramGroup.group == 0:
            y = 0

        if self.selected != None:
            pos = (float(x) / float(self.width))*zoomRange + self.zoomMin
            self.transferF.MoveColor(self.selected, pos)
            colord = self.transferF.GetColorAtIndex(self.selected)
            a = float(self.height - y)/float(self.height)
            if a > 1.0:
                a = 1.0
            elif a < 0.0:
                a = 0.0

            color = (colord[0], colord[1], colord[2], a)

            self.transferF.SetColor(self.selected, color)
            self.Update()
        
    def Update(self):
        if (self.histogramGroup != None):
          histo = self.histogramGroup.histogram
          self.absoluteDMin = histo.colorDMin
          self.absoluteDMax = histo.colorDMax
          absoluteRange = self.absoluteDMax - self.absoluteDMin
          self.zoomDMin = self.absoluteDMin + self.zoomMin*absoluteRange
          self.zoomDMax = self.absoluteDMin + self.zoomMax*absoluteRange
        width = self.width - 2.0
        height = self.height - 2.0
        self.barWidth = 1.0
        
        blx = 1.0 + self.barWidth/2.0 + self.paddingW/2.0 # bottom left x
        bly = 0.0 + self.height + self.barWidth/2.0 - 2.0 + self.paddingH/2.0

        self.lines = []
        zoomRange = self.zoomMax - self.zoomMin
        for i in range(0, int(width)):
            color = self.transferF.GetColor( (float(i)/float(width))*zoomRange +self.zoomMin )
            self.lines.append( (color, ( blx + i*self.barWidth, bly, blx + i*self.barWidth, (bly - height) ) ) )
        self.parentC.Refresh()
        self.Refresh()
        if (self.updateFunction != None):
            self.updateFunction()
            
    def AddNewColor(self):
        dlg = wx.ColourDialog(self)
        dlg.GetColourData().SetChooseFull(True)
        data = dlg.GetColourData()
        
        if dlg.ShowModal() == wx.ID_OK:
            data = dlg.GetColourData()
            c = data.GetColour().Get()
            color = []
            color.append(c[0])
            color.append(c[1])
            color.append(c[2])
            color[0] /= 255.0
            color[1] /= 255.0
            color[2] /= 255.0
            self.transferF.AddColor( color, random.random())
            self.Update()
            
        dlg.Destroy()
        self.UpdateHistogram()
    
    def DeleteSelected(self):
        if self.dSelected != None:
            self.transferF.RemoveColor(self.dSelected)
            self.Update()
        self.UpdateHistogram()
    
    def ChooseColorSelected(self):
        if self.dSelected != None:
            c = self.transferF.colors[self.dSelected]
            color = wx.ColourData()
            color.SetColour(wx.Colour(c[1]*255, c[2]*255, c[3]*255))
            dlg = wx.ColourDialog(self)
            dlg.GetColourData().SetChooseFull(True)
            
            if dlg.ShowModal() == wx.ID_OK:
                data = dlg.GetColourData()
                c = data.GetColour().Get()
                color = []
                color.append(c[0])
                color.append(c[1])
                color.append(c[2])
                color[0] /= 255.0
                color[1] /= 255.0
                color[2] /= 255.0
                self.transferF.SetColor(self.dSelected, color)
                self.Update()
                self.Refresh()
                
            dlg.Destroy()
            self.UpdateHistogram()
        
    def OnPaint(self, evt=None):
        pdc = wx.PaintDC(self)
	try:
          dc = wx.GCDC(pdc)
	except:
	  dc = pdc
       
	lines = self.lines
        colors = self.transferF.colors
        dc.SetPen(wx.Pen('BLACK', 2) )
        left = self.paddingW/2.0
        top = self.paddingH/2.0
	originalSize = self.GetClientSize()
	dc.SetClippingRegion(self.paddingW/2.0,self.paddingH/2.0 ,self.width-1, self.height)
	imgWidth = self.backgroundIMG.GetWidth()
        imgHeight = self.backgroundIMG.GetHeight()
        for x in range(int(self.paddingW/2) + 1, int(self.width ), int(imgWidth)):
                for y in range(int(self.paddingH/2.0) +2, int(self.height) , int(imgHeight)):
                        dc.DrawBitmap(self.backgroundIMG,x,y, True)
        # IS THIS THE BROKEN LINE? ->
	# dc.SetClippingRegion(0-self.paddingW/2.0,0-self.paddingH/2.0,originalSize.width, originalSize.height)
        try:
	   dc = wx.GCDC(pdc)
        except:
           dc = pdc
        numPixels = self.width
        for i in range(0, len(lines)):
            a = lines[i][0][3]*255.0
            r = lines[i][0][0]*255.0
            g = lines[i][0][1]*255.0
            b = lines[i][0][2]*255.0
	    try:
	      penColor = wx.Colour(r,g,b,a)
	    except:
	      penColor = wx.Colour(r,g,b)
            dc.SetPen(wx.Pen(penColor, self.barWidth + 1) )
            dc.DrawLine( lines[i][1][0], lines[i][1][1], lines[i][1][2], lines[i][1][3])
        zoomRange = self.zoomMax-self.zoomMin
        for i in range(len(colors)):
            colorx = (colors[i][0]-self.zoomMin)/zoomRange
            if (colorx < 0.0 or colorx > 1.0):
                continue
            dc.SetPen(wx.Pen('GRAY', 2) )
            color = self.transferF.GetColor( colors[i][0] )
	    try:
               dc.SetBrush(wx.Brush( color, wx.SOLID ) )
	    except:
               dc.SetBrush(wx.Brush( (color[0], color[1], color[2]), wx.SOLID))
            if i == self.dSelected:
               dc.SetBrush(wx.Brush( (128,128,128), wx.SOLID ) )
            recWidth = self.colorSelectorWidth
            x = colorx*self.width - recWidth/2.0 + left
            y = self.height - recWidth + top
            dc.DrawRectangle(x,y - color[3]*self.height + recWidth/2.0, recWidth, recWidth)
        dc.SetTextForeground(wx.Colour(0,0,0))
        self.SetForegroundColour(wx.Colour(255,0,0))
        dc.SetPen(wx.Pen(wx.Colour(255,255,255), 1))
        dc.SetBrush(wx.Brush(wx.Colour(255,255,255)))
        fontSize = 10
        if self.scene.biggify == True:
                fontSize = 12
        dc.SetFont(wx.Font(fontSize, wx.FONTFAMILY_DEFAULT, wx.NORMAL, wx.FONTWEIGHT_BOLD))
        string = ""
        if (self.scene.labels == True):
          string += "zoom min: "
        string += str("%1.2g" % self.zoomDMin)
        extent = dc.GetTextExtent(string)
        #xpos = extent[0]/2.0 - self.paddingW/2.0
        xpos = self.paddingW/2.0
        diff = xpos - self.paddingW/2.0
        #if diff < 0:
        #    xpos -= diff
        ypos = self.height - 2
        dc.DrawTextPoint(string, (xpos,ypos))
        string = ""
        if (self.scene.labels == True):
          string += "zoom max: "
        string += str("%1.2g" % self.zoomDMax)
        extent = dc.GetTextExtent(string)
        xpos = self.width - extent[0]/2.0 + self.paddingW/2.0
        diff = xpos + extent[0] - (self.width + self.paddingW/2.0)
        if (diff > 0 ):
           xpos -= diff
        dc.DrawTextPoint(string, (xpos,ypos))
        
        # draw min/max text
        ypos += extent[1]
        if self.scene.biggify:
              ypos -= 2
        string = ""
        if (self.scene.labels == True):
          string += "color min: "
        string += str("%1.2g" %self.absoluteDMin)
        extent = dc.GetTextExtent(string)
        #xpos = extent[0]/2.0 + self.paddingW/2.0
        xpos = self.paddingW/2.0
        dc.DrawTextPoint(string, (xpos,ypos))
        string = ""
        if (self.scene.labels == True):
          string += "color max: "
        string += str("%1.2g" % self.absoluteDMax)
        extent = dc.GetTextExtent(string)
        xpos = self.width - extent[0]/2.0 + self.paddingW/2.0
        diff = xpos + extent[0] - (self.width + self.paddingW/2.0)
        if (diff > 0 ):
           xpos -= diff
        dc.DrawTextPoint(string, (xpos,ypos))

        ypos = self.paddingH/2.0 - extent[1]
        string = ""
        if (self.scene.labels == True):
          string += "mouse value: "
        string += str("%1.4g" % self.mouse_value)
        extent = dc.GetTextExtent(string)
        xpos = self.paddingW/2.0 + self.width/2.0 - extent[0]/2.0
        if self.scene.biggify:
            ypos += 2
        dc.DrawTextPoint(string, (xpos,ypos))

class TransferFGroup(wx.Panel):
    def __init__(self, parent, width, height, transferF, title, scene):

        path = setup.csafe_scene_path

	self.parentC = parent
        self.height = height
        self.width = width
        self.transferF = transferF
        wx.Panel.__init__(self, parent, -1, (0, 0) , (width, height) )
        self.vs = vs = wx.BoxSizer( wx.VERTICAL )
        self.box1_title = wx.StaticBox( self, -1, title )

        self.box1_title.SetForegroundColour( wx.WHITE ) # Make label readable!
        
        self.transferFPanel = TransferFPanel(self, width, height, transferF, scene)
        box1 = self.box1 = wx.StaticBoxSizer( self.box1_title, wx.VERTICAL )
        self.gbs = gbs = wx.GridBagSizer(5,5)
        self.sizer = box1
        self.scene = scene
        
        gbs.Add(self.transferFPanel,(0, 0), (5, 2) )
        
        bmpNew = wx.Bitmap(opj(path+'images/new_16x16.png'))
        bmpDel = wx.Bitmap(opj(path+'images/delete_16x16.png'))
        bmpMod = wx.Bitmap(opj(path+'images/color_16x16.png'))

        self.newColorB = wx.BitmapButton(self, -1, bmpNew, (0,0), style=wx.NO_BORDER)
        self.newColorB.SetToolTip( wx.ToolTip( "Press to choose a new color for chosen color map position." ) )

        self.delColorB = wx.BitmapButton(self, -1, bmpDel, (0,0), style=wx.NO_BORDER)
        self.delColorB.SetToolTip( wx.ToolTip( "delcolorb: fix me" ) )

        self.modifyColorB = wx.BitmapButton(self, -1, bmpMod, (0,0), style=wx.NO_BORDER)
        self.modifyColorB.SetToolTip( wx.ToolTip( "Modify Colormap Node (Make sure you select a gray square first.)" ) )
        
        self.presetsB = wx.BitmapButton(self, -1, bmpMod, (0,0), style=wx.NO_BORDER)
        self.presetsB.SetToolTip( wx.ToolTip( "Choose Colormap Preset" ) )

        gbs.Add( self.newColorB, (0, 2) )
        gbs.Add( self.delColorB, (1, 2) )
        gbs.Add( self.modifyColorB, (2, 2) )
        gbs.Add( self.presetsB, (3,2) )
        
        
        gbs.Layout()
        
        box1.Add( gbs, -1, wx.ALIGN_CENTRE|wx.ALL, 5 )
        vs.Add( box1, -1, wx.ALIGN_CENTRE|wx.ALL, 5 )
        #vs.Add(grid1, 0, wx.ALIGN_CENTRE|wx.ALL, 5 )
        self.SetSizer( vs )
        vs.Fit( self )
        self.visible = True
        
        self.Bind(wx.EVT_BUTTON, self.OnClickNew, self.newColorB)
        self.Bind(wx.EVT_BUTTON, self.OnClickDelete, self.delColorB)
        self.Bind(wx.EVT_BUTTON, self.OnClickModify, self.modifyColorB)
        self.Bind(wx.EVT_BUTTON, self.OnClickPresets, self.presetsB)

        
        self.box1_title.SetBackgroundColour(self.scene.bgColor)
        self.transferFPanel.SetBackgroundColour(self.scene.bgColor)
        
    def OnClickPresets(self, evt):
        
        menu = wx.Menu()
        self.ids = []
        for i in range (len(self.scene.frame.transferFunctions)):
            popupID = wx.NewId()
            menu.Append(popupID, self.scene.frame.transferFunctions[i].label)
            self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
            self.ids.append(popupID)
        self.puNewID = wx.NewId()
        self.copyID = wx.NewId()
        copy_menu = wx.Menu()
        menu.AppendMenu(self.copyID, "Copy From...", copy_menu)
        self.copy_ids = []
        
        popupID = wx.NewId()
        copy_menu.Append(popupID, "Default: InvRainbowIso")
        self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
        self.copy_ids.append(popupID)
        
        popupID = wx.NewId()
        copy_menu.Append(popupID, "Default: InvRainbow")
        self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
        self.copy_ids.append(popupID)
        
        popupID = wx.NewId()
        copy_menu.Append(popupID, "Default: Rainbow")
        self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
        self.copy_ids.append(popupID)
        
        popupID = wx.NewId()
        copy_menu.Append(popupID, "Default: InvGrayscale")
        self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
        self.copy_ids.append(popupID)

        
        popupID = wx.NewId()
        copy_menu.Append(popupID, "Default: InvBlackBody")
        self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
        self.copy_ids.append(popupID)

        
        popupID = wx.NewId()
        copy_menu.Append(popupID, "Default: BlackBody")
        self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
        self.copy_ids.append(popupID)

        
        popupID = wx.NewId()
        copy_menu.Append(popupID, "Default: GreyScale")
        self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
        self.copy_ids.append(popupID)
        
        for i in range (len(self.scene.frame.transferFunctions)):
            popupID = wx.NewId()
            copy_menu.Append(popupID, self.scene.frame.transferFunctions[i].label)
            self.Bind(wx.EVT_MENU, self.OnPopUp, id=popupID)
            self.copy_ids.append(popupID)
        menu.Append(self.puNewID, "New...")
        self.Bind(wx.EVT_MENU, self.OnPopUp, id=self.puNewID)
        self.PopupMenu(menu)
        menu.Destroy()
    
    def OnPopUp(self, evt):
        if evt.GetId() == self.puNewID:
            dlg = wx.TextEntryDialog(self, 'Set Name of new TF:', 'TF', 'TF')
            dlg.SetValue('Untitled')
            if dlg.ShowModal() == wx.ID_OK:
                name = dlg.GetValue()
                colors = []
                index = len(self.scene.frame.transferFunctions)
                slices = manta_new(vector_ColorSlice());
                for i in range(len(self.transferF.colors)):
                    c = self.transferF.colors[i]
                    slices.push_back(ColorSlice(c[0],RGBAColor(Color(RGBColor(c[1],c[2],c[3])), c[4])))
                    colors.append(c)
                cmap = self.scene.frame.sphereVolCMaps.append(manta_new(RGBAColorMap(slices, 64)))
                t = TransferF(self.scene.frame, colors, index, name, cmap)
                self.scene.frame.transferFunctions.append(t)
                self.SetTransferF(t)
        else:
            for i in range(len(self.ids)):
                if evt.GetId() == self.ids[i]:
                    self.SetTransferF(self.scene.frame.transferFunctions[i])
            for i in range(len(self.copy_ids)):
                if evt.GetId() == self.copy_ids[i]:
                    #copy transfer function to current transfer function
                    if i < 7:
                        temp = TransferF(self, [], -1, "GreyScale",     manta_new(RGBAColorMap(i)))
                        self.transferF.Clone(temp)
                    else:
                        self.transferF.Clone(self.scene.frame.transferFunctions[i- 7])
        self.transferFPanel.histogramGroup.SetTransferF(self.transferF)
        self.transferFPanel.UpdateHistogram()
        self.transferFPanel.Update()
        
    def OnClickNew(self, evt):
        self.transferFPanel.AddNewColor()
    
    def OnClickDelete(self, evt):
        self.transferFPanel.DeleteSelected()
        
    def OnClickModify(self, evt):
        self.transferFPanel.ChooseColorSelected()
        
    def SetLabel(self, label):
        self.box1_title.SetLabel(label)
        
    def SetTransferF(self, transferF):
        self.transferF = transferF
        self.transferFPanel.SetTransferF(transferF)
        label = transferF.label
        if self.transferFPanel.histogramGroup != None:
          label  = str(label) + str(" : ") + str(self.transferFPanel.histogramGroup.title)
          self.SetLabel(label)
          if self.transferFPanel.histogramGroup != None:
              self.transferFPanel.histogramGroup.SetTransferF(transferF)
        
    def SetUpdateFunction(self, function):
        None
