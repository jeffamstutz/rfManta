import wx
from manta import *
from pycallback import *

import FloatSpin as FS
import wx.lib.colourselect as csel

# Clamps values between 0 and 255
def clampZeroTo255(val):
    return max(0,min(255,val))

###############################################################################
###############################################################################
# LIGHT FRAME          LIGHT FRAME          LIGHT FRAME          LIGHT FRAME   
###############################################################################
###############################################################################        
class LightFrame(wx.Frame):
    def __init__(self, parent, engine):
        wx.Frame.__init__(self, parent=parent, title="Lights")

        self.engine = engine
        self.colorButtonRefs = {}
        panel= wx.Panel(self, -1)

        vsizer = wx.BoxSizer( wx.VERTICAL );

        hsizer = wx.BoxSizer( wx.HORIZONTAL );
        hsizer.Add( wx.StaticText( panel, -1, "Changes applied interactively" ), 0, wx.ALIGN_CENTER );
        vsizer.Add( hsizer );

        gbs = wx.GridBagSizer(5,7)
        
        lights = engine.getScene().getLights()
        for i in range(lights.numLights()):
            gbs.Add( self.addLight(panel, lights.getLight(i)), (i, 0), flag=wx.ALIGN_CENTER_VERTICAL )

        mlteButton = wx.Button(panel, -1, "Move Light To Eye")

        closeButton = wx.Button(panel, -1, "Close")
        panel.SetSizer(gbs)

        vsizer.Add( panel, 0, wx.EXPAND );

        hsizer = wx.BoxSizer( wx.HORIZONTAL );
        hsizer.Add( mlteButton,  0, wx.ALIGN_CENTER );
        hsizer.Add( closeButton, 0, wx.ALIGN_CENTER );
        
        vsizer.Add( hsizer, 0, wx.ALIGN_CENTER );
        self.SetSizerAndFit( vsizer );

        panel.Bind( wx.EVT_BUTTON, self.MoveLightToEye, mlteButton)
        panel.Bind( wx.EVT_BUTTON, self.OnClose, closeButton)
        panel.Bind( wx.EVT_CLOSE,  self.OnClose )
        panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        panel.SetFocus()

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Show(False)
        evt.Skip()

    def MoveLightToEye(self, event):
        point_light = PointLight.fromLight( self.engine.getScene().getLights().getLight(0) )

        data = self.engine.getCamera(0).getBasicCameraData()

        newPosition = Vector( data.eye.x(), data.eye.y(), data.eye.z() )

        print "new eye position is: " + str(data.eye.x()) + ", " + str(data.eye.y()) + ", " + str(data.eye.z())
        cbArgs = ( newPosition, )
        self.engine.addTransaction("Light Position",
                                   manta_new(createMantaTransaction(point_light.setPosition, cbArgs)))
        # FIXME: Update spinner with new values.
        #        Probably need to add this button every spinner frame...
        
    def addLight(self, where, light):
        point_light = PointLight.fromLight(light)
        if (point_light != None):
            return self.addPointLight(where, point_light)
        head_light = HeadLight.fromLight(light)
        if (head_light != None):
            return self.addHeadLight(where, head_light)

    def addHeadLight(self, where, light):
        offset = light.getOffset()

        panel = wx.Panel(where, -1)

        gbs = wx.GridBagSizer(5,1)
        gbs.Add( wx.StaticText(panel, -1, "Offset"), (0,0))
        spinnerOffset = self.addSpinner(panel, offset)
        spinnerOffset.light = light
        # Override the callback function
        spinnerOffset.Bind(FS.EVT_FLOATSPIN, self.OnOffsetSpin)
        gbs.Add( spinnerOffset, (0,1))
        
        colorWidgets = self.AddColorButton(light, panel)
        gbs.Add( colorWidgets["colorButton"], (0,2))
        gbs.Add( wx.StaticText(where, -1, "Color Scale"), (0,3))
        gbs.Add( colorWidgets["colorScaleSpinner"], (0,4) )

        panel.SetSizer(gbs)
        return panel

    def addPointLight(self, where, light):
        location = light.getPosition()

        panel = wx.Panel(where, -1)
        
        gbs = wx.GridBagSizer(7,1)
        gbs.Add( wx.StaticText(panel, -1, "Location"), (0,0), flag=wx.ALIGN_CENTER_VERTICAL )
        spinnerX = self.addSpinner(panel, location.x())
        spinnerY = self.addSpinner(panel, location.y())
        spinnerZ = self.addSpinner(panel, location.z())
        spinnerX.light = light
        spinnerX.X = spinnerX
        spinnerX.Y = spinnerY
        spinnerX.Z = spinnerZ
        spinnerY.light = light
        spinnerY.X = spinnerX
        spinnerY.Y = spinnerY
        spinnerY.Z = spinnerZ
        spinnerZ.light = light
        spinnerZ.X = spinnerX
        spinnerZ.Y = spinnerY
        spinnerZ.Z = spinnerZ
        gbs.Add( spinnerX, (0,1), flag=wx.ALIGN_CENTER_VERTICAL)
        gbs.Add( spinnerY, (0,2), flag=wx.ALIGN_CENTER_VERTICAL)
        gbs.Add( spinnerZ, (0,3), flag=wx.ALIGN_CENTER_VERTICAL)

        colorWidgets = self.AddColorButton(light, panel)
        gbs.Add( colorWidgets["colorButton"], (0,4), flag=wx.ALIGN_CENTER_VERTICAL)
        gbs.Add( wx.StaticText(where, -1, "Color Scale"), (0,5), flag=wx.ALIGN_CENTER_VERTICAL)
        gbs.Add( colorWidgets["colorScaleSpinner"], (0,6), flag=wx.ALIGN_CENTER_VERTICAL )

        panel.SetSizer(gbs)
        return panel

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
        spinner = event.GetEventObject()
        x = float(spinner.X.GetValue())
        y = float(spinner.Y.GetValue())
        z = float(spinner.Z.GetValue())
        newPosition = Vector(x,y,z)
        cbArgs = ( newPosition, )
        self.engine.addTransaction("Light Position",
                                   manta_new(createMantaTransaction(spinner.light.setPosition, cbArgs)))

    def OnOffsetSpin(self, event):
        # Pull out the new value
        spinner = event.GetEventObject()
        offset = float(spinner.GetValue())
        cbArgs = ( offset, )
        self.engine.addTransaction("Light Offset",
                                   manta_new(createMantaTransaction(spinner.light.setOffset, cbArgs)))
        
    def AddColorButton(self, light, where):
        color = light.getColor().convertRGB()

        # Because our colors can be more than one we need to do a
        # little bit of work.  The color selector can't deal with
        # floating point colors, and it must work with ranges [0,255].
        # In order to overcome this limitation, we are going to use a
        # scale.  If all your color components are less than 1, then
        # the scale will be 1, otherwise it will be scaled by the
        # maximum component.
        colorTupRaw = [ color.r(), color.g(), color.b() ]
        maxComponent = max(colorTupRaw)
        print "maxComponent = %g" % (maxComponent)
        if (maxComponent <= 1):
            colorScale = 1
        else:
            colorScale = maxComponent
        colorTup = map(lambda c:clampZeroTo255(int(c*255/colorScale)),
                       colorTupRaw)
        
        colorButton =  csel.ColourSelect(where, wx.ID_ANY, "Color",
                                                        tuple(colorTup))
        colorButton.Bind(csel.EVT_COLOURSELECT, self.OnSelectColor)
        # You have to use these refs, because the event you get in
        # your callback doesn't contain the actual colorButton object.
        # All you can be sure of is the ID.
        self.colorButtonRefs[colorButton.GetId()] = colorButton
        colorButton.light = light


        # Add the scale
        print "colorScale = %g, maxComponent = %g" % (colorScale,maxComponent)
        # I tried to make the ID the same as the colorButton and bind
        # it to the same callback, but wxMac didn't like it.  Thus,
        # the spinner now gets its own callback.
        colorScaleSpinner = FS.FloatSpin(where, wx.ID_ANY,
                                         min_val=1, increment=0.01,
                                         value=colorScale,
                                         extrastyle=FS.FS_LEFT)
        colorScaleSpinner.SetFormat("%g")
        colorScaleSpinner.SetDigits(5)
        colorScaleSpinner.Bind(FS.EVT_FLOATSPIN, self.OnColorScaleSpin)
        colorScaleSpinner.button = colorButton
        colorButton.spinner = colorScaleSpinner

        # Shoving multiple results in a dictionary will make things a little easier to read.
        results = {"colorButton":colorButton, "colorScaleSpinner":colorScaleSpinner}
        return results

    def UpdateColor(self, colorButton):
        color = colorButton.GetColour()
        colorScale = float(colorButton.spinner.GetValue()) / 255.0
        rgbColor = RGBColor(color.Red()   * colorScale,
                            color.Green() * colorScale,
                            color.Blue()  * colorScale)
        cbArgs = ( Color(rgbColor), )
        self.engine.addTransaction("Light Color",
                                   manta_new(createMantaTransaction(colorButton.light.setColor, cbArgs)))
        
    def OnColorScaleSpin(self, event):
        try:
            colorButton = event.GetEventObject().button
            self.UpdateColor(colorButton)
        except:
            wx.LogError("Button not found")
        
    def OnSelectColor(self, event):
        try:
            colorButton = self.colorButtonRefs[event.GetId()]
            self.UpdateColor(colorButton)
        except:
            wx.LogError("Button not found")

    def OnClose( self, event ):
        # Don't destroy... just remove it from the screen so it can be used later.
        self.Show( False )
