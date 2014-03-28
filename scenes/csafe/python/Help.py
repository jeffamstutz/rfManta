
import wx

zoom_help_string = \
  "The mouse wheel zooms in/out on the histogram.  Place the mouse \
cursor over the location you wish to be the center of the zoom, and \
then rotate the wheel."

colormap_help_string = \
  "Press the colormap button (looks like a wheel) to color the data by the given \
  colormap.  This will also place the colormap in the colormap editor at the bottom of \
  the screen."

def showZoomHelp() :
    wx.MessageBox( zoom_help_string, "Zoom Help", x=0, y=0 )

def showColormapHelp() :
    wx.MessageBox( colormap_help_string, "Colormap Help", x=0, y=0 )


