""" File: Histogram.py
Description:  histogram control, and histogramGroup which is a histogram with various controls

"""

import sys, os, time, traceback, types
import wx
import wx.html
import TransferF
import wx.lib.foldpanelbar

from csafe       import *
from csafe_scene import setup
from wxManta     import moveToMouse, opj

import FloatSpin as FS

data = []
numBuckets = 1
hMin = 0.0
hMax = 1.0
dMin = 0.0
dMax = 0.0
lines = []
width = 10.0
height = 20.0
barWidth = 0.0
colors = []
measurementsWindow = []

class HistogramPanel(wx.Panel):
    def __init__(self, parent,histValues, dataMin, dataMax, width, height, transferF, scene, varIndex):
        self.scene = scene
        self.varIndex = varIndex
        self.paddingW = 20.0
        self.paddingH = 15.0
        wx.Panel.__init__(self, parent, -1, (0, 0) , (width+self.paddingW, height+self.paddingH) )
        
        self.parent = parent
        self.dragging = False
        self.selecting = False
        self.draggingLeft = False
        self.draggingRight = False
        self.dragWidth = 1.0
        self.cropMin = 0.0  #selected region from [0,1]
        self.cropMax = 1.0  
        #if (varIndex == 3):  #TODO: take this out!
        #       self.cropMin = -0.5
        #       self.cropMax = 1.5;
        self.colorMin = -99999.0
        self.colorMax = 99999.0
        self.colorDMin = dataMin
        self.colorDMax = dataMax
        self.dMin = dataMin
        self.dMax = dataMax  #the min and max of displayed data (zoomin in region)
        self.zoomDMin = dataMin #min data value of data to display (may not be an actual value contianed in data)
        self.zoomDMax = dataMax #max data value of data to display (may not be an actual value contianed in data)
        self.absoluteDMin = self.dMin  #absolute min of the data
        self.absoluteDMax = self.dMax
        self.zooms = []
        wx.EVT_PAINT(self, self.OnPaint)
        self.transferF = transferF
        self.SetValues(histValues, len(histValues), width, height)
        self.cropDMin = self.absoluteDMin
        self.cropDMax = self.absoluteDMax
        self.Update()
        self.Bind(wx.EVT_LEFT_DOWN, self.OnClick)
        self.Bind(wx.EVT_LEFT_UP, self.OnLeftUp)
        self.Bind(wx.EVT_RIGHT_DOWN, self.OnRightClick)
        self.Bind(wx.EVT_RIGHT_UP, self.OnRightUp)
        self.Bind(wx.EVT_MOTION, self.OnMotion)
        self.Bind( wx.EVT_MOUSEWHEEL, self.OnMouseWheel )
        self.SetBackgroundColour(wx.Colour(90,90,90))
        self.panning = False
        self.panning_pos = 0
        #self.SetForegroundColour(wx.Colour(255,255,255))
       
    def SetHistValues(self, histValues, dataMin, dataMax):
        self.zooms = []
        self.colorMin = -99999.0
        self.colorMax = 99999.0
        self.dMin = dataMin
        self.dMax = dataMax  #the min and max of displayed data
        self.zoomDMin = dataMin #min data value of data to display (may not be an actual value contianed in data)
        self.zoomDMax = dataMax #max data value of data to display (may not be an actual value contianed in data)
        self.absoluteDMin = dataMin  #absolute min of the data (ignoring zoom)
        self.absoluteDMax = dataMax
        self.colorDMin = dataMin  #scale colors by this minimum value
        self.colorDMax = dataMax  #scale colors by this maximum value
        self.zoomDMin = self.dMin   
        self.zoomDMax = self.dMax
        self.cropDMin = dataMin
        self.cropDMax = dataMax
        self.data = histValues
        self.numBuckets = len(histValues)
        self.Update()

    def OnMouseWheel(self, evt):
#        pos = float(evt.GetPosition().x - self.paddingW/2.0)/float(self.width)
        pos = 0.5
        delta = evt.GetWheelDelta()
        rot = evt.GetWheelRotation()/delta
        # zoom in if rot > 0, out if ro < 0
        dWidth = self.zoomDMax - self.zoomDMin
        zoomAmount = 0.75 # the smaller the more zooming
        if (rot > 0): 
          self.ZoomIn(pos,zoomAmount)
        
        if (rot < 0):
          self.ZoomOut(pos, zoomAmount)
    
    def ZoomIn(self, pos, zoom_amount):
      dWidth = self.zoomDMax - self.zoomDMin
      self.zoomDMax = (pos + zoom_amount*(1.0 - pos))*dWidth + self.zoomDMin
      self.zoomDMin = (pos - zoom_amount*pos)*dWidth + self.zoomDMin

      if (self.zoomDMin < self.absoluteDMin):
           self.zoomDMin = self.absoluteDMin
      if (self.zoomDMax > self.absoluteDMax):
           self.zoomDMax = self.absoluteDMax
            
      tempMin = self.cropDMin
      tempMax = self.cropDMax
      self.Update()
      self.cropDMin = tempMin
      self.cropDMax = tempMax
      self.UpdateCropToD()
      self.UpdateMeasurementsFrame()
	
        
    def ZoomOut(self, pos, zoom_amount):
      zoom_amount = 1.0/zoom_amount
      dWidth = self.zoomDMax - self.zoomDMin
      self.zoomDMax = (pos + zoom_amount*(1.0 - pos))*dWidth + self.zoomDMin
      self.zoomDMin = (pos - zoom_amount*pos)*dWidth + self.zoomDMin
      #self.zoomDMax += (pos + zoom_amount*(1.0 - pos))*dWidth
      #self.zoomDMin -= (pos - zoom_amount*pos)*dWidth

      if (self.zoomDMin < self.absoluteDMin):
	self.zoomDMin = self.absoluteDMin
      if (self.zoomDMax > self.absoluteDMax):
	 self.zoomDMax = self.absoluteDMax
	  
      tempMin = self.cropDMin
      tempMax = self.cropDMax
      self.Update()
      self.cropDMin = tempMin
      self.cropDMax = tempMax
      self.UpdateCropToD()
      self.UpdateMeasurementsFrame()
       
    def OnClick(self, evt):
        pos = float(evt.GetPosition().x - self.paddingW/2.0)/float(self.width)
        sideLen = 0.05
        if (abs(pos - float(self.cropMin)) <= float(sideLen)) and \
           (abs(pos - float(self.cropMin)) < abs(pos - float(self.cropMax))):
            self.draggingLeft = True
            self.draggingRight = False
            self.cropMin = pos
        elif (abs(pos - float(self.cropMax)) <= float(sideLen)):
            self.draggingLeft = False
            self.draggingRight = True
            self.cropMax = pos
        elif (pos >= self.cropMin and pos <= self.cropMax):
            self.dragging = True
            self.dragWidth = abs((float(self.cropMax) - float(self.cropMin)))
        else: # selecting
            self.selecting = True
            self.draggingLeft = False
            self.draggingRight = False
            self.cropMin = pos
        self.UpdateDisplayedValues()
        self.UpdateMeasurementsFrame()
    
    def OnLeftUp(self, evt):
        pos = float(evt.GetPosition().x - self.paddingW/2.0)/float(self.width)
            
        if self.cropMin > self.cropMax:
            temp = self.cropMax
            self.cropMax = self.cropMin
            self.cropMin = temp
            self.UpdateDisplayedValues()
            self.UpdateCropDMin()
            self.UpdateCropDMax()
            self.Refresh()
            self.UpdateMeasurementsFrame()
            
        self.dragging = False
        self.selecting = False
        self.draggingLeft = False
        self.draggingRight = False
        #self.scene.test.setClipMinMax(self.varIndex, self.cropDMin, self.cropDMax)
    
    def OnRightClick(self, evt):
      pos = float(evt.GetPosition().x - self.paddingW/2.0)/float(self.width)
      self.panning = True
      self.panning_pos = pos

    def OnRightUp(self, evt):
      self.panning = False

    def Pan(self, amount):
      dWidth = self.zoomDMax - self.zoomDMin
      pan = amount*dWidth
      if (amount > 0):
        if( self.zoomDMax + pan ) > self.absoluteDMax:
          pan = self.absoluteDMax - self.zoomDMax
      if (amount < 0):
        if( self.zoomDMin + pan ) < self.absoluteDMin:
          pan =  self.zoomDMin - self.absoluteDMin
      self.zoomDMax += pan
      self.zoomDMin += pan
      #self.zoomDMax += (pos + zoom_amount*(1.0 - pos))*dWidth
      #self.zoomDMin -= (pos - zoom_amount*pos)*dWidth

      if (self.zoomDMin < self.absoluteDMin):
        self.zoomDMin = self.absoluteDMin
      if (self.zoomDMax > self.absoluteDMax):
        self.zoomDMax = self.absoluteDMax
    
      tempMin = self.cropDMin
      tempMax = self.cropDMax
      self.Update()
      self.cropDMin = tempMin
      self.cropDMax = tempMax
      self.UpdateCropToD()
      self.UpdateMeasurementsFrame()
      
      
        
    def OnMotion(self, evt):
        #pdc = wx.PaintDC(self)
        #pdc.Clear()
        #dc = wx.GCDC(pdc)
        if (self.dMin == self.dMax):  # don't interact if no clipping range
          return
        pos = float(evt.GetPosition().x - self.paddingW/2.0)/float(self.width)
        update_clip = False
        if (self.panning):  
          self.Pan(pos - self.panning_pos)
          self.panning_pos = pos
        if self.selecting == True:
            self.cropMax = pos
            self.UpdateDisplayedValues()
            self.Refresh()
            update_clip = True
        if self.dragging == True:
            cropWidth = self.dragWidth
            self.cropMin = pos - cropWidth/2.0
            self.cropMax = pos + cropWidth/2.0
            if self.cropMin < 0.0:
                self.cropMin = 0
            if self.cropMax > 1.0:
                self.cropMax = 1.0
            self.UpdateDisplayedValues()
            self.UpdateCropDMin()
            self.UpdateCropDMax()
            self.Refresh()
            update_clip = True
        if self.draggingLeft == True:
            self.cropMin = pos
            self.UpdateDisplayedValues()
            self.UpdateCropDMin()
            self.Refresh()
            update_clip = True
        elif self.draggingRight == True:
            self.cropMax = pos
            self.UpdateDisplayedValues()
            self.UpdateCropDMax()
            self.Refresh()
            update_clip = True
        if (update_clip):
            self.UpdateMeasurementsFrame()
        if (update_clip and self.parent.group != 1):  # if not the volumedata
            index = self.varIndex;
            min = float(self.cropDMin - 0.00001)
            max = float(self.cropDMax + 0.00001)
            self.scene.test.setClipMinMax(index, min,max)    #TODO: should get rid of epsilon...
       
    def SetValues(self, datavalues, nbuckets, widthn, heightn):
        self.data = datavalues
        self.numBuckets = nbuckets
        self.width = widthn
        self.height = heightn

    def SetHistoWidth(self, width):
        self.width = width
        self.SetSize( (width + self.paddingW, self.height+self.paddingH))
        self.Update()
        
    #update dMin/Max, cropMin/Max based on zoomDMin/Max    
    def UpdateDisplayedValues(self):
        absoluteDWidth = self.absoluteDMax - self.absoluteDMin
        if absoluteDWidth == 0:
           absoluteDWidth = 1.0
        minBucket = int(float(self.zoomDMin/absoluteDWidth)*self.numBuckets )
        maxBucket = int(float(self.zoomDMax/absoluteDWidth)*self.numBuckets)
        if (self.numBuckets > 0):
           self.dMin = minBucket*(absoluteDWidth/self.numBuckets)
           self.dMax = maxBucket*(absoluteDWidth/self.numBuckets)
        else:
           self.dMin = self.dMax = 0.0
        

    def UpdateCropDMin(self):
        dWidth = self.dMax - self.dMin
        self.cropDMin = self.cropMin*dWidth + self.dMin # cropped data min
        if (self.parent.group == 1):
            self.colorDMin = self.cropDMin
            self.scene.test.setVolColorMinMax(self.colorDMin, self.colorDMax)
            self.scene.test.setVolClipMinMax(self.absoluteDMin, self.absoluteDMax, self.cropDMin, self.cropDMax)
            self.Update()

    def UpdateCropDMax(self):
        dWidth = self.dMax - self.dMin
        self.cropDMax = self.cropMax*dWidth + self.dMin # cropped data max
        if (self.parent.group == 1):
            self.colorDMax = self.cropDMax
            self.scene.test.setVolColorMinMax(self.colorDMin, self.colorDMax)
            self.scene.test.setVolClipMinMax(self.absoluteDMin, self.absoluteDMax, self.cropDMin, self.cropDMax)
            self.Update()
        
    def UpdateCropToD(self):
        dWidth = self.dMax - self.dMin
        if (dWidth == 0):
            self.cropMin = self.cropMax = 0
        else:
            self.cropMin = (self.cropDMin - self.dMin)/dWidth
            self.cropMax = (self.cropDMax - self.dMin)/dWidth

    def UpdateMeasurementsFrame(self):
        if self.scene.measurementsFrame != None and self.scene.measurementsFrame.parent.histogram == self:
           self.scene.measurementsFrame.SendValues(self.zoomDMin, self.zoomDMax, self.cropDMin, self.cropDMax, self.colorDMin, self.colorDMax)
        
        
    def Update(self):
        self.lines = []
        width = self.width
        height = self.height
        self.UpdateDisplayedValues()
        data = self.data
        
        numBuckets = self.numBuckets
        dMin = self.dMin
        dMax = self.dMax
        absoluteDMin = self.absoluteDMin
        absoluteDMax = self.absoluteDMax
        absWidth = float(absoluteDMax - absoluteDMin)
        if absWidth == 0:
           return
        dWidth = dMax - dMin        

        # lines.append( (0,0, 50, 50) )
        # function(lines, range(0.0, 1.0))

        start = float((dMin - absoluteDMin)/absWidth*float(numBuckets))
        if (numBuckets > 0):
           step = float((dWidth/absWidth)/float(width))*float(numBuckets)
        else:
           step = 0.0
        end = start + step
        if (numBuckets > 0):
           barWidth = self.barWidth = float(width)/float(numBuckets)
        else:
           barWidth = self.barWidth = float(width)
        
        blx = 0.0 + barWidth/2.0 + self.paddingW/2.0 # bottom left x
        bly = 0.0 + height - barWidth/2.0
        
        hMin = 99999.0 # min and max frequencies
        hMax = -999999.0
    
        
        #get the minimum and maximum frequencies in displayed range
        #for i in range(int(numBuckets)):
        #    frequency = 1.0
        #    filter(lines, range(start, end) )
        #    frequency = len( [x for x in data if (x >= start and x <= end)] )
        #    print '%f %f %f' % (frequency, start, end)
        #    frequency = data[i]
        #    if frequency < hMin:
        #        hMin = frequency
        #    if frequency > hMax:
        #        hMax = frequency
        #    start += step
        #    end += step

        colorWidth = float(self.colorDMax - self.colorDMin)
        if (colorWidth == 0.0):
            colorWidth = 1.0;
        croppedHeightValues = int(width)*[0]
        for i in range(0,int(width)):
            # frequency = len( [x for x in data if (x >= start and x <= end)] )
            start2 = int(start)
            end2 = int(end)
            if (start2 == end2):
                end2 += 1
            for j in range(start2,end2):
                if j >= 0 and j < numBuckets:
                    croppedHeightValues[i] += data[j]
            start += step
            end += step
        
        for i in range(0, int(width)):
            frequency = croppedHeightValues[i]
            if frequency < hMin:
                hMin = frequency
            if frequency > hMax:
                hMax = frequency
        if hMax > hMin:
            for i in range(0, int(width)):
                frequency = croppedHeightValues[i]
                barHeightNorm = float(frequency)/float(hMax)
                # print str(frequency) + " " + str(barHeightNorm)
                if float(barHeightNorm) > 1.0:
                    print "error"
                # print '%f %f %f %f' % (frequency, start, end, barHeightNorm)
                # colorPos = (( (float(i)/width)*(absoluteDMax-absoluteDMin)) + absoluteDMin- self.colorDMin)/colorWidth
                colorPos = (( (float(i)/width)*(self.zoomDMax-self.zoomDMin)) + self.zoomDMin- self.colorDMin)/colorWidth
                # print colorPos
                color = self.transferF.GetColor(colorPos)
                self.lines.append( (color, ( blx + i, bly, blx + i, (bly - barHeightNorm*height) ) ) )
        self.Refresh()
       
    def OnPaint(self, evt=None):
        pdc = wx.PaintDC(self)
        # pdc.Clear()
        oldwx = False
        try:
          dc = wx.GCDC(pdc)
        except:
          dc = pdc
          oldwx = True
        # gc = wx.GraphicsContext.Create(dc)       
        dc.SetPen(wx.Pen("BLACK", 1) )
        # dc.DrawRectangle( 0, 0, width, height) 
        
        self.DrawLines(dc)
        try:
          brushColor = wx.Colour(0,0,0,56)
        except:
          brushColor = wx.Colour(0,0,0)
        dc.SetBrush(wx.Brush( brushColor ) )
        dc.SetPen(wx.Pen("BLACK", 0) )
        cropMin = self.cropMin
        cropMax = self.cropMax
        if cropMin < 0.0:
            cropMin = 0.0
        if cropMax > 1.0:
            cropMax = 1.0
        cropWidth = (float(cropMax) - float(cropMin))*float(self.width)
        if (self.dMin != self.dMax):
            if oldwx == False:
              dc.DrawRectangle(float(cropMin)*float(self.width) + self.paddingW/2.0 , 0, cropWidth, self.height)
            else:
              penColor = wx.Colour(0,0,0)
              dc.SetPen(wx.Pen(penColor, 5))
              xpos = float(cropMin)*float(self.width) + self.paddingW/2.0
              dc.DrawLine( xpos, 0, xpos, self.height)
              dc.DrawLine( xpos, self.height/2, xpos+cropWidth, self.height/2)
              xpos += cropWidth
              dc.DrawLine( xpos, 0, xpos, self.height)
        
        # draw cropping texta
        dc.SetTextForeground(wx.Colour(255,255,255))
        self.SetForegroundColour(wx.Colour(255,0,0))
        dc.SetPen(wx.Pen(wx.Colour(255,255,255), 1))
        dc.SetBrush(wx.Brush(wx.Colour(255,255,255)))
        fontSize = 10
        if self.scene.biggify == True:
                fontSize = 12
        dc.SetFont(wx.Font(fontSize, wx.FONTFAMILY_DEFAULT, wx.NORMAL, wx.FONTWEIGHT_BOLD))

        if (self.dMin != self.dMax):
            string = ""
            if (self.scene.labels):
              string += "crop min: "
            string += str("%1.2g" % self.cropDMin)
            extent = dc.GetTextExtent(string)
            xpos = cropMin*self.width - extent[0]/2.0 + self.paddingW/2.0
            diff = xpos - self.paddingW/2.0
            if diff < 0:
                xpos -= diff
            ypos = self.height-extent[1]
            dc.DrawTextPoint(string, (xpos,ypos))
            string = ""
            if (self.scene.labels):
              string += "crop max: "
            string += str("%1.2g" % self.cropDMax)
            extent = dc.GetTextExtent(string)
            xpos = cropMax*self.width - extent[0]/2.0 + self.paddingW/2.0
            diff = xpos + extent[0] - (self.width + self.paddingW/2.0)
            if (diff > 0 ):
               xpos -= diff
            dc.DrawTextPoint(string, (xpos,ypos))

            # draw min/max text
            ypos = self.height
            if self.scene.biggify:
                  ypos = self.height
            string = ""
            if (self.scene.labels):
              string += "zoom min: "
            string += str("%1.2g" %self.dMin)
            extent = dc.GetTextExtent(string)
            xpos = self.paddingW/2.0
            dc.DrawTextPoint(string, (xpos,ypos))
            string = ""
            if (self.scene.labels):
              string += "zoom max: "
            string += str("%1.2g" % self.dMax)
            extent = dc.GetTextExtent(string)
            xpos = self.width - extent[0]/2.0 + self.paddingW/2.0
            diff = xpos + extent[0] - (self.width + self.paddingW/2.0)
            if (diff > 0 ):
               xpos -= diff
            dc.DrawTextPoint(string, (xpos,ypos))
        else:
            string = "Empty";
            extent = dc.GetTextExtent(string)
            dc.DrawTextPoint("Empty", (self.width/2.0 - extent[0]/2.0 + self.paddingW/2.0,self.height/2.0))


    def DrawLines(self, dc):

        # dc.BeginDrawing()
        for i in range(0, len(self.lines)):
            # pos = float(i)/float(len(self.lines))
            a = self.lines[i][0][3]*255.0
            r = self.lines[i][0][0]*255.0
            g = self.lines[i][0][1]*255.0
            b = self.lines[i][0][2]*255.0
            try:
                penColor = wx.Colour(r,g,b,a)
            except:
                penColor = wx.Colour(r,g,b)
            dc.SetPen(wx.Pen(penColor, self.barWidth))
            dc.DrawLine( self.lines[i][1][0], self.lines[i][1][1], self.lines[i][1][2], self.lines[i][1][3])
        
        # dc.DrawLineList(lines)
        # dc.DrawLine(0,0,50,50)
        # dc.EndDrawing()
 

###################################################################
####################### Histogram Group ###########################
###################################################################       
class HistogramGroup(wx.Panel):
    def __init__(self, parent, scene, varIndex, title = "No Name", transferFID = 0):
        wx.Panel.__init__(self, parent, -1, (0, 0) , (300.0, 40.0) )
        self.scene = scene
        self.varIndex = varIndex
        self.transferF = self.scene.frame.transferFunctions[transferFID]
        self.transferFID = transferFID
        self.transferFPanel = self.scene.tPanel
        self.title = title
        self.parent = parent
        self.datavalues = []
        for i in range(0, self.scene.histogramBuckets):
            self.datavalues.append(5)
        self.dataMin = 0
        self.dataMax = 0
        self.numbuckets = self.scene.histogramBuckets
        self.width = 300.0
        self.height = 40.0
        self.group = 0 # 0 = spheres, 1 = volume
        
        self.CreateElements()
        self.SetBackgroundColour(scene.bgColor)
        #self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Deselect()


#    def __init__(self, parent, dataMin, dataMax, width, height, transferF, transferFPanel, scene, varIndex, title = "No Name"):
#        wx.Panel.__init__(self, parent, -1, (0, 0) , (width, height) )
#       self.scene = scene
#       self.varIndex = varIndex
#        self.transferF = transferF
#       self.transferFID = transferF.id
#        self.transferFPanel = transferFPanel
#        self.title = title
#        self.parent = parent
#        self.datavalues = []
#        for i in range(0, self.scene.histogramBuckets):
#           self.datavalues.append(5)
#       self.dataMin = dataMin
#       self.dataMax = dataMax
#        self.numbuckets = self.scene.histogramBuckets
#        self.width = width
#        self.height = height
#       self.group = 0 #0 = spheres, 1 = volume
#        
#        self.CreateElements()
#        self.SetBackgroundColour(self.scene.bgColor)


    def Select(self):
        self.box1_title.SetBackgroundColour(self.scene.selectedBGColor)
        self.histogram.SetBackgroundColour(self.scene.selectedBGColor)
        
    def Deselect(self):
        self.box1_title.SetBackgroundColour(self.scene.bgColor)
        self.histogram.SetBackgroundColour(self.scene.bgColor)

    def SetHistoWidth(self, width):
        None
       # self.width = width
       # self.histogram.SetHistoWidth(width-50)
       # self.panel.SetSize( (width, 40.0) )
       # self.SetSize( (width, 40.0) )
        #self.gbs.AddSpace( (100, 0),1, wx.EXPAND )
        #self.gbs.Layout()
        #self.gbs.Fit(self.histogram)
        #self.gbs.SetSize( (width, 40) )
        #self.LayoutElements()

    #def OnResize(self, evt):
       # size = evt.GetSize()
       # self.SetSize( (size[0], self.height) )
        

    def SetTransferF(self, transferF):
        if (transferF != self.transferF):
            self.transferF = transferF
            self.transferFID = transferF.id
            if self.group == 0:
                self.scene.test.setSphereCMap(transferF.cmap)
            else:
                self.scene.test.setVolCMap(transferF.cmap)
            self.histogram.transferF = transferF
    
    def SetValues(self, histValues, dataMin, dataMax):  
        self.dataMin = dataMin
        self.dataMax = dataMax
        self.datavalues = histValues
        self.histogram.SetHistValues(histValues, dataMin, dataMax)
        
    def SetCMinMax(self, min, max):
        zoomMin = self.histogram.zoomDMin
        zoomMax = self.histogram.zoomDMax
        cropMin = self.histogram.cropDMin
        cropMax = self.histogram.cropDMax
        self.SendValues(zoomMin, zoomMax, cropMin, cropMax, float(min), float(max))
    
    def OnClick(self, evt):
        None
        
    def CreateElements(self):
        path = setup.csafe_scene_path

        self.vs = vs = wx.BoxSizer( wx.VERTICAL )
        box1_title = wx.StaticBox( self, -1, self.title + " (" + str( self.varIndex ) + ")" )

        box1_title.SetForegroundColour( wx.WHITE ) # Make label readable!
        self.box1_title = box1_title

        self.histogram = HistogramPanel( self, self.datavalues, self.dataMin, self.dataMax, self.width, self.height,
                                         self.transferF, self.scene, self.varIndex )
        self.box1 = box1 = wx.StaticBoxSizer( box1_title, wx.VERTICAL )
        # grid1 = wx.FlexGridSizer( 2, 2, 0, 0 )
        gbs = wx.GridBagSizer(1,3)
        #gbs = wx.BoxSizer(wx.HORIZONTAL)
        self.sizer = box1
        
        # wx.EVT_LEFT_DOWN(self.histogram, self.OnClick)
        # self.SetFocus()
        # self.histogram.Bind(wx.EVT_LEFT_DOWN, self.OnClick)
        #gbs.Add(self.histogram, 1, wx.EXPAND|wx.ALIGN_LEFT, 10)
        gbs.Add(self.histogram,(0, 0), (2, 2) )
        if self.scene.histogramBMPLoaded == False:
                self.scene.bmpVis = wx.Bitmap(opj(path+'images/eye.png.8x8'), wx.BITMAP_TYPE_PNG)
                self.scene.bmpColor = wx.Bitmap(opj(path+'images/color.png.8x8'), wx.BITMAP_TYPE_PNG)
                self.scene.bmpRuler = wx.Bitmap(opj(path+'images/ruler.png.8x8'), wx.BITMAP_TYPE_PNG)
                self.scene.bmpZoomIn = wx.Bitmap(opj(path+'images/zoomin.png.8x8'), wx.BITMAP_TYPE_PNG)
                self.scene.bmpZoomOut = wx.Bitmap(opj(path+'images/zoomout.png.8x8'), wx.BITMAP_TYPE_PNG)
                self.scene.histogramBMPLoaded = True
        
        self.bmpVis = self.scene.bmpVis
        self.bmpColor = self.scene.bmpColor
        self.bmpRuler = self.scene.bmpRuler
        self.bmpZoomIn = self.scene.bmpZoomIn
        self.bmpZoomOut = self.scene.bmpZoomOut

        size = 10
        visibilityB = wx.BitmapButton(self, -1, self.bmpVis, (8,8), style=wx.NO_BORDER)
        visibilityB.SetToolTip( wx.ToolTip( "Iconify this color map." ) )

        self.visibilityB = visibilityB
        colorB = wx.BitmapButton(self, -1, self.bmpColor, (0,0), style=wx.NO_BORDER)
        colorB.SetToolTip( wx.ToolTip( "Color data using this color map. (Also opens this color map in color map editor, below.)" ) )

        self.colorB = colorB
        self.rulerB = wx.BitmapButton(self, -1, self.bmpRuler, (0,0), style=wx.NO_BORDER)
        self.rulerB.SetToolTip( wx.ToolTip( "Press to set clipping range explicitly." ) )
        self.rulerB.SetBackgroundColour(self.scene.bgColor)

        self.zoomInB = wx.BitmapButton(self, -1, self.bmpZoomIn, (0,0), style=wx.NO_BORDER)
        self.zoomInB.SetToolTip( wx.ToolTip( "Zoom In" ) )

        self.zoomOutB = wx.BitmapButton(self, -1, self.bmpZoomOut, (0,0), style=wx.NO_BORDER)
        self.zoomOutB.SetToolTip( wx.ToolTip( "Zoom Out" ) )

        self.Bind(wx.EVT_BUTTON, self.OnClickVisible, visibilityB)
        self.Bind(wx.EVT_BUTTON, self.OnClickColor, colorB)
        self.Bind(wx.EVT_BUTTON, self.OnClickMeasurements, self.rulerB)
        self.Bind(wx.EVT_BUTTON, self.OnClickZoomIn, self.zoomInB)
        self.Bind(wx.EVT_BUTTON, self.OnClickZoomOut, self.zoomOutB)
        
        
        vs2 = wx.BoxSizer( wx.VERTICAL )
        # vs3 = wx.BoxSizer( wx.VERTICAL )
        # box1_title2 = wx.StaticBox( self, -1, "" )
        # box2 = wx.StaticBoxSizer( box1_title2, wx.VERTICAL )
        vs2.Add( visibilityB, 0, wx.ALIGN_CENTRE|wx.ALL, 0, 10)
        space = (0,0)
        vs2.AddSpacer(space)
        vs2.Add( colorB, 0, wx.ALIGN_CENTRE|wx.ALL, 0, 10)
        vs2.AddSpacer(space)
        vs2.Add( self.rulerB, 0)
        vs2.AddSpacer(space)
        vs3 = wx.BoxSizer(wx.VERTICAL)
        vs3.Add( self.zoomInB, 0)
        vs3.AddSpacer(space)
        vs3.Add( self.zoomOutB , 0)
        vs2.Layout()
        vs3.Layout()
        
        vsH = wx.BoxSizer(wx.HORIZONTAL)
        vsH.Add(vs2, 0, wx.ALIGN_CENTER,0)
        vsH.Add(vs3, 0, wx.ALIGN_CENTER,0)  
        # vs3.Layout()
        gbs.Add(vsH, (0, 2), (2, 1))
        #gbs.Add(vsH, 1, wx.ALIGN_RIGHT, 10)
        # gbs.Add(vs3, (0, 3), (2, 1))
        
        
        gbs.Layout()
        
        box1.Add( gbs, 0, wx.ALIGN_CENTRE, 0 )
        self.box1 = box1
        vs.Add( box1, 0, wx.ALIGN_CENTRE, 0 )
        # vs.Add(grid1, 0, wx.ALIGN_CENTRE|wx.ALL, 5 )
        self.SetSizer( vs )
        vs.Fit( self )
        gbs.Fit(self.histogram)
        self.vs = vs
        self.visible = True
        self.gbs = gbs
        self.vsH = vsH
        self.vs2 = vs2
        self.vs3 = vs3
   #     self.SetAutoLayout(True)

    def LayoutElements(self):

        self.vs = vs = wx.BoxSizer( wx.VERTICAL )
        box1_title = wx.StaticBox( self, -1, self.title + " (" + str( self.varIndex ) + ")" )

        box1_title.SetForegroundColour( wx.WHITE ) # Make label readable!
        self.box1 = box1 = wx.StaticBoxSizer( box1_title, wx.VERTICAL )
        # grid1 = wx.FlexGridSizer( 2, 2, 0, 0 )
        gbs = wx.GridBagSizer(1,3)
        self.sizer = box1
        
        # wx.EVT_LEFT_DOWN(self.histogram, self.OnClick)
        # self.SetFocus()
        # self.histogram.Bind(wx.EVT_LEFT_DOWN, self.OnClick)
        
        gbs.Add(self.histogram,(0, 0), (2, 2) )

        
        
        vs2 = wx.BoxSizer( wx.VERTICAL )
        # vs3 = wx.BoxSizer( wx.VERTICAL )
        # box1_title2 = wx.StaticBox( self, -1, "" )
        # box2 = wx.StaticBoxSizer( box1_title2, wx.VERTICAL )
        vs2.Add( self.visibilityB, wx.ALIGN_CENTRE|wx.ALL, 0, 10)
        space = (0,0)
        vs2.AddSpacer(space)
        vs2.Add( self.colorB, wx.ALIGN_CENTRE|wx.ALL, 0, 10)
        vs2.AddSpacer(space)
        vs2.Add( self.rulerB)
        vs2.AddSpacer(space)
        vs3 = wx.BoxSizer(wx.VERTICAL)
        vs3.Add( self.zoomInB)
        vs3.AddSpacer(space)
        vs3.Add( self.zoomOutB )
        vs2.Layout()
        vs3.Layout()
        
        vsH = wx.BoxSizer(wx.HORIZONTAL)
        vsH.Add(vs2, wx.ALIGN_CENTER|wx.ALL,0)
        vsH.Add(vs3, wx.ALIGN_CENTER|wx.ALL,0)  
        # vs3.Layout()
        gbs.Add(vsH, (0, 2), (2, 1))
        # gbs.Add(vs3, (0, 3), (2, 1))
        
        
        gbs.Layout()
        
        box1.Add( gbs, 0, wx.ALIGN_CENTRE|wx.ALL, 0 )
        self.box1 = box1
        vs.Add( box1, 0, wx.ALIGN_CENTRE|wx.ALL, 0 )
        # vs.Add(grid1, 0, wx.ALIGN_CENTRE|wx.ALL, 5 )
        self.SetSizer( vs )
        vs.Fit( self )
        self.vs = vs
        self.visible = True
        self.gbs = gbs
        self.vsH = vsH
        self.vs2 = vs2
        self.vs3 = vs3
        self.SetAutoLayout(True)
    
    def OnClickMeasurements(self, evt):
        if (self.scene.measurementsFrame == None):
          self.scene.measurementsFrame = MeasurementsFrame( self, -1, self.title + " Measurements",
                                                    self.histogram.zoomDMin, self.histogram.zoomDMax,
                                                    self.histogram.cropDMin, self.histogram.cropDMax,
                                                    self.histogram.colorDMin, self.histogram.colorDMax, self.group)
          moveToMouse( self.scene.measurementsFrame )
        self.scene.measurementsFrame.Show(True)
        self.scene.measurementsFrame.Raise()
        self.scene.measurementsFrame.SetParent(self)
        self.histogram.UpdateMeasurementsFrame()

    def SendValues(self, zoomMin, zoomMax, cropMin, cropMax, colorMin, colorMax):  # sent from MeasurementsFrame
        if (self.histogram.parent.group == 1):
            colorMin = cropMin
            colorMax = cropMax
        self.histogram.zooms.append( (self.histogram.zoomDMin, self.histogram.zoomDMax))
        self.histogram.zoomDMin = zoomMin
        self.histogram.zoomDMax = zoomMax
        self.histogram.colorDMin = colorMin
        self.histogram.colorDMax = colorMax
        self.histogram.Update()
        min = 0
        max = 0
        adWidth = self.histogram.absoluteDMax - self.histogram.absoluteDMin
        dWidth = self.histogram.dMax - self.histogram.dMin
        if dWidth == 0:
           dWidth = 1.0
        self.histogram.cropMin = (cropMin - self.histogram.dMin)/dWidth
        self.histogram.cropMax = (cropMax - self.histogram.dMin)/dWidth
        if self.histogram.cropMin < 0.0:
            self.histogram.cropMin = 0.0
        if self.histogram.cropMin > 1.0:
            self.histogram.cropMin = 1.0
        if self.histogram.cropMax < 0.0:
            self.histogram.cropMax = 0.0
        if self.histogram.cropMax > 1.0:
            self.histogram.cropMax = 1.0
            
        self.histogram.cropDMin = cropMin # cropped data min
        self.histogram.cropDMax = cropMax # cropped data max
        if (self.histogram.parent.group == 0):
            if self.transferFPanel.transferFPanel.histogramGroup == self:
                self.scene.test.setSphereColorMinMax(self.histogram.varIndex, colorMin, colorMax)
            self.scene.test.setClipMinMax(self.histogram.varIndex, cropMin,cropMax)
        else:
                self.scene.test.setVolColorMinMax(float(colorMin), float(colorMax))
                self.scene.test.setVolClipMinMax(self.histogram.absoluteDMin, self.histogram.absoluteDMax, cropMin, cropMax)
        
    def OnClickZoomIn(self, evt):
        self.histogram.ZoomIn(.5, .75)
        
    def OnClickZoomOut(self, evt):
        self.histogram.ZoomOut(.5, .75)
        
    def Update(self):
        self.histogram.Update()
        
    def OnClickColor(self, evt):
        # self.transferFPanel.SetLabel(self.transferF.GetLabel())

        self.transferFPanel.transferFPanel.SetHistogramGroup(self)
        self.transferFPanel.SetTransferF(self.transferF)
        self.transferFPanel.SetUpdateFunction(self.Update())
        if self.group == 0:
            self.scene.test.setCidx(self.varIndex)        
            if (self.transferF != None):
                self.scene.test.setSphereCMap(self.transferF.cmap)
                self.scene.test.setSphereColorMinMax(self.varIndex,  self.histogram.colorDMin,  self.histogram.colorDMax)
                
        if( evt != None ) : #  evt == None if called in csafe_scene for initialization.
            self.scene.currentHistogram = self
            if (self.group == 0 ): # group 0 == Particles.
                self.scene.currentParticleHistogram = self

    def OnClickVisible(self, evt):
        if self.visible:
            path = setup.csafe_scene_path
            self.sizer.ShowItems(False)
            # self.sizer.Show(self.visibilityB, True, True)
            # self.sizer.Show(self.historgram, False, True)
            # self.sizer.Show(self.colorB, False, True)
            self.visible = False

            box1_title = wx.StaticBox( self, -1, self.title )
            box1_title.SetForegroundColour( wx.WHITE ) # Make label readable!

            box = wx.StaticBoxSizer( box1_title, wx.HORIZONTAL )
            sizer = wx.BoxSizer( wx.HORIZONTAL )
            box.AddSpacer((100,0))
            bmpVis = wx.Bitmap(opj(path+'images/eye.png.8x8'))

            visibilityB = wx.BitmapButton(self, -1, bmpVis, (8,8), (12, 12))
            visibilityB.SetToolTip( wx.ToolTip( "Un-iconfiy this color map." ) )

            box.Add(visibilityB, wx.ALIGN_CENTRE|wx.ALL, 0, 0)
            self.Bind(wx.EVT_BUTTON, self.OnClickVisible, visibilityB)
            # p = wx.Panel(self.parent, -1)
            # text = wx.StaticText(self.parent, -1, "warg?", (20, 10))

            # box.Add(text, wx.ALIGN_CENTRE|wx.ALL, 0, 0 )
            box.AddSpacer((100,0))
            # box.Layout()
            sizer.Add(box)
            sizer.Layout()
            self.SetSizer(box)
            box.Fit(self)
            # sizer.Fit(self)
            # self.vs.SetDimension(0,0,0,0)
            # self.vs.Clear(False)
            # self.vs.Add( self.visibilityB, wx.ALIGN_CENTRE|wx.ALL, 0, 10)
            # self.vs.Clear()
            # self.vs.Fit(self)
            # self.vs.SetSize(100, 100)
            # self.vs.Layout()
            # self.SetMaxSize((100, 10))
            self.scene.frame.panel.GetSizer().Layout()
            self.parent.Refresh()
        else:
            # self.SetSizer(self.vs)
            # self.vs.Fit(self)
            # self.vs.Clear(False)
            # self.vs.Add(self.box1)
            # self.sizer.ShowItems(True)
            # self.visible = True
            # self.SetSizer( vs )
            # vs.Fit( self )
            # self.vs.Layout()
            self.GetSizer().ShowItems(False)
            self.GetSizer().Clear()
            self.CreateElements()
            self.sizer.ShowItems(True)
            self.vs.Layout()
            self.vs.Fit(self)
            self.parent.GetSizer().Layout()
            self.parent.Refresh()
            self.transferFPanel.transferFPanel.SetHistogramGroup(self)
            
            
class MeasurementsFrame(wx.Frame):
    def __init__(self, parent, ID, title,  zoomMin, zoomMax, cropMin, cropMax, colorMin, colorMax, group, pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.DEFAULT_FRAME_STYLE):
      
        self.cancelling = False
        wx.Frame.__init__(self, parent, ID, title, pos, size, style)
        self.parent = parent
        self.panel = panel = wx.Panel(self, -1)
        self.vs = vs = wx.BoxSizer( wx.VERTICAL )
        gbs = wx.GridBagSizer(6,6)
        self.group = group
        self.zoomText = wx.StaticText(panel, -1, "Zoom into data values: ", (20, 10))
        spin = self.zoomMinFS = self.addSpinner(panel,zoomMin);
        self.Bind(FS.EVT_FLOATSPIN, self.OnZoomMinFS, self.zoomMinFS )
        spin = self.zoomMaxFS = self.addSpinner(panel,zoomMax);
        self.Bind(FS.EVT_FLOATSPIN, self.OnZoomMaxFS, self.zoomMaxFS )
        self.cropText = wx.StaticText(panel, -1, "Crop displayed data values: ", (20, 10))
        spin = self.cropMinFS = self.addSpinner(panel,cropMin);
        self.Bind(FS.EVT_FLOATSPIN, self.OnCropMinFS, self.cropMinFS )
        spin = self.cropMaxFS = self.addSpinner(panel,cropMax);
        self.Bind(FS.EVT_FLOATSPIN, self.OnCropMaxFS, self.cropMaxFS )
        
        #if (group == 0): # Particles
        self.colorText = wx.StaticText(panel, -1, "Rescale colormap: ", (20, 10))
        spin = self.colorMinFS =self. addSpinner(panel,colorMin);
        self.Bind(FS.EVT_FLOATSPIN, self.OnColorMinFS, self.colorMinFS )
        spin = self.colorMaxFS = self.addSpinner(panel,colorMax);
        self.Bind(FS.EVT_FLOATSPIN, self.OnColorMaxFS, self.colorMaxFS )
        self.button = wx.Button(panel, -1, "OK")
        #self.cancelB = wx.Button(panel, -1, "Cancel")
        self.resetB = wx.Button(panel, -1, "Reset")
        self.Bind(wx.EVT_BUTTON, self.OnReset, self.resetB)

        
        gbs.Add(self.zoomText, (1,0))
        gbs.Add(self.zoomMinFS, (1,1))
        gbs.Add(self.zoomMaxFS, (1,2))
        gbs.Add(self.cropText, (2,0))
        gbs.Add(self.cropMinFS, (2,1))
        gbs.Add(self.cropMaxFS, (2,2))
        
        if (group == 1):
            self.colorText.Show(False)
            self.colorMinFS.Show(False)
            self.colorMaxFS.Show(False)
        gbs.Add(self.colorText, (3,0))
        gbs.Add(self.colorMinFS, (3,1))
        gbs.Add(self.colorMaxFS, (3,2))
        
        vs2 = wx.BoxSizer( wx.HORIZONTAL )
        
        vs2.Add(self.button)
        #vs2.AddSpacer(15)
        #vs2.Add(self.cancelB)
        vs2.AddSpacer((0,15))
        vs2.Add(self.resetB)
        vs2.Layout()
        
        vs.Add(gbs, 0, wx.ALIGN_CENTRE, border=15 )
        vs.AddSpacer((0,15))
        vs.Add(vs2, 0, wx.ALIGN_CENTRE|wx.ALIGN_BOTTOM, 1 )
        vs.AddSpacer((0,15))
        
        vs.Layout()
        
        panel.SetSizerAndFit(vs)

        self.vs3 = vs3 = wx.BoxSizer(wx.VERTICAL)
        vs3.Add(panel,0, wx.EXPAND, border=15)
        self.SetSizerAndFit(vs3)
        self.SetAutoLayout(True)
        

        self.Bind(wx.EVT_BUTTON, self.OnOK, self.button)
        #self.Bind(wx.EVT_BUTTON, self.OnCancel, self.cancelB)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        self.Bind(wx.EVT_SHOW, self.OnShowWindow)
        panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        panel.SetFocus()
        
        self.zoomMin = self.zoomMinFS.GetValue()
        self.zoomMax = self.zoomMaxFS.GetValue()
        self.cropMin = self.cropMinFS.GetValue()
        self.cropMax = self.cropMaxFS.GetValue()
        self.colorMin = self.colorMinFS.GetValue()
        self.colorMax = self.colorMaxFS.GetValue()
        
        
    def OnShowWindow(self, evt):
      None
      #  self.cancelling = False

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
        return floatspin

    def UpdateIncrement(self, fs):
        val = fs.GetValue()
        fs.SetIncrement(abs(val*0.01))

    def OnZoomMinFS(self, evt):
        val = self.zoomMinFS.GetValue()
        val2 = self.zoomMaxFS.GetValue()
        if (val > val2):
            val = val2
            self.zoomMinFS.SetValue(val2);
        self.ApplyValues()
        self.UpdateIncrement(self.zoomMinFS)

    def OnZoomMaxFS(self, evt):
        val = self.zoomMaxFS.GetValue()
        val2 = self.zoomMinFS.GetValue()
        if (val < val2):
            val = val2
            self.zoomMaxFS.SetValue(val2);
        self.ApplyValues()
        self.UpdateIncrement(self.zoomMaxFS)

    def OnCropMinFS(self, evt):
        val = self.cropMinFS.GetValue()
        val2 = self.cropMaxFS.GetValue()
        if (val > val2):
            val = val2
            self.cropMinFS.SetValue(val2);
        self.ApplyValues()
        self.UpdateIncrement(self.cropMinFS)

    def OnCropMaxFS(self, evt):
        val = self.cropMaxFS.GetValue()
        val2 = self.cropMinFS.GetValue()
        if (val < val2):
            val = val2
            self.cropMaxFS.SetValue(val2);
        self.ApplyValues()
        self.UpdateIncrement(self.cropMaxFS)
        
    def OnColorMinFS(self, evt):
        val = self.colorMinFS.GetValue()
        val2 = self.colorMaxFS.GetValue()
        if (val > val2):
            val = val2
            self.colorMinFS.SetValue(val2);
        self.ApplyValues()
        self.UpdateIncrement(self.colorMinFS)

    def OnColorMaxFS(self, evt):
        val = self.colorMaxFS.GetValue()
        val2 = self.colorMinFS.GetValue()
        if (val < val2):
            val = val2
            self.colorMaxFS.SetValue(val2);
        self.ApplyValues()
        self.UpdateIncrement(self.colorMaxFS)
        
    def OnCancel(self, evt):
        print "cancel"
        self.zoomMinFS.SetValue(self.zoomMin)
        self.zoomMaxFS.SetValue(self.zoomMax)
        self.cropMinFS.SetValue(self.cropMin)
        self.cropMaxFS.SetValue(self.cropMax)
        self.colorMinFS.SetValue(self.colorMin)
        self.colorMaxFS.SetValue(self.colorMax)
        self.cancelling = True
        self.Show( False )

    def ApplyValues(self):
        zoomMin = self.zoomMinFS.GetValue()
        zoomMax = self.zoomMaxFS.GetValue()
        cropMin = self.cropMinFS.GetValue()
        cropMax = self.cropMaxFS.GetValue()
        if (self.group == 0):
            colorMin = self.colorMinFS.GetValue()
            colorMax = self.colorMaxFS.GetValue()
        else:
            colorMin = cropMin
            colorMax = cropMax
      #  if self.cancelling == False:
        self.parent.SendValues(float(zoomMin), float(zoomMax), float(cropMin), float(cropMax), float(colorMin), float(colorMax))
        
        self.zoomMin = self.zoomMinFS.GetValue()
        self.zoomMax = self.zoomMaxFS.GetValue()
        self.cropMin = self.cropMinFS.GetValue()
        self.cropMax = self.cropMaxFS.GetValue()
        self.colorMin = self.colorMinFS.GetValue()
        self.colorMax = self.colorMaxFS.GetValue()

    #who to send values to.  Should have SendValues function
    def SetParent(self,parent):
        self.parent = parent
        self.group = parent.group
        self.SetTitle(parent.title)
        if (self.group == 0):
          self.colorText.Show(True)
          self.colorMinFS.Show(True)
          self.colorMaxFS.Show(True)
        else:
          self.colorText.Show(False)
          self.colorMinFS.Show(False)
          self.colorMaxFS.Show(False)
        self.vs.Layout()
        
        self.panel.SetSizerAndFit(self.vs)

        self.SetSizerAndFit(self.vs3)
        self.SetAutoLayout(True)
        

    def SendValues(self, zoomMin, zoomMax, cropMin, cropMax, colorMin, colorMax):
        self.zoomMinFS.SetValue(zoomMin)
        self.zoomMaxFS.SetValue(zoomMax)
        self.cropMinFS.SetValue(cropMin)
        self.cropMaxFS.SetValue(cropMax)
        if (self.group == 0):
            self.colorMinFS.SetValue(colorMin)
            self.colorMaxFS.SetValue(colorMax)
        self.ApplyValues()

    def OnReset(self, evt):
        zoomMin = self.parent.histogram.absoluteDMin
        zoomMax = self.parent.histogram.absoluteDMax
        cropMin = zoomMin
        cropMax = zoomMax
        colorMin = zoomMin
        colorMax = zoomMax
        self.zoomMinFS.SetValue(zoomMin)
        self.zoomMaxFS.SetValue(zoomMax)
        self.cropMinFS.SetValue(cropMin)
        self.cropMaxFS.SetValue(cropMax)
        if (self.group == 0):
            self.colorMinFS.SetValue(colorMin)
            self.colorMaxFS.SetValue(colorMax)
        self.ApplyValues()
        
    def OnOK(self, event):
        self.ApplyValues()
        self.Show( False )

    def OnCloseWindow(self, event):
        self.Show(False)
        
