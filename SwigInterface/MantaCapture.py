import wx
import sys
import time

import threading
import math
import wx.lib.colourselect as csel
import os
import re
import shutil
import math

from manta import *
from pycallback import *

import FloatSpin as FS

###############################################################################
# Manta Image Sequence Capture Frame.
# This class ports functionality of FMantaCapture dialog to wxManta.

class MantaCapturePanel(wx.Panel):
    def __init__(self, parent, engine, channel=0 ):

        wx.Panel.__init__(self, parent )

        self.engine  = engine
        self.channel = channel

        # GUI Components.
        self.prefix_field = wx.TextCtrl(self, -1, "manta", size=(125,-1))

        type_list = ['png','nrrd']
        self.type_box     = wx.ComboBox( self, -1, "", (-1,-1), (-1, -1), type_list, wx.CB_DROPDOWN )
        self.type_box.SetSelection(0)
        self.skip_spinner = wx.SpinCtrl( self, -1, "", (30,50));
        self.skip_spinner.SetValue(1)
        self.timestamp_check = wx.CheckBox( self, -1, "Use timestamp for filename." )

        # Arrange gui components.
        vsizer = wx.BoxSizer(wx.VERTICAL);
        hsizer = wx.BoxSizer(wx.HORIZONTAL);
        hsizer.Add( wx.StaticText(self,-1,"Image sequence prefix:"), wx.ALIGN_CENTER )
        hsizer.Add( self.prefix_field, wx.ALIGN_CENTER )
        hsizer.Add( self.type_box, wx.ALIGN_CENTER )
        vsizer.Add( hsizer, wx.ALIGN_CENTER );

        hsizer = wx.BoxSizer(wx.HORIZONTAL);
        hsizer.Add( wx.StaticText(self,-1,"Record every:"), wx.ALIGN_CENTER )
        hsizer.Add( self.skip_spinner, wx.ALIGN_CENTER )
        hsizer.Add( wx.StaticText(self,-1,"frames."), wx.ALIGN_CENTER )
        vsizer.Add( hsizer, wx.ALIGN_CENTER );

        vsizer.Add( self.timestamp_check, wx.ALIGN_CENTER )

        self.SetSizerAndFit(vsizer)
        
    ###########################################################################
    # Gui interactions.
    def OnStartButton( self, event ):

        # Look up arguments
        prefix    = self.prefix_field.GetValue()
        file_type = self.type_box.GetString(self.type_box.GetSelection())
        skip      = self.skip_spinner.GetValue()
        use_timestamp = self.timestamp_check.GetValue()

        # Send a transaction to manta.
        cbArgs = ( self.channel, prefix, file_type, skip, use_timestamp )
        self.engine.addTransaction("Capture Start",
                                   manta_new(createMantaTransaction(self.MantaStart, cbArgs)))
        
        # Disable UI.
        self.skip_spinner.Disable()
        
    def OnStopButton( self, event ):

        # Send a transaction to Manta.
        cbArgs = ( self.channel, )
        self.engine.addTransaction("Capture Stop",
                                   manta_new(createMantaTransaction(self.MantaStop, cbArgs)))
        # Enable UI.
        self.skip_spinner.Enable()

    def GetPrefix( self ):
        return self.prefix_field.GetValue()

    def GetType( self ):
        self.type_box.GetString(self.type_box.GetSelection())
        
    def OnCaptureButton( self, event ):

        # Look up arguments
        prefix    = self.GetPrefix()
        file_type = self.GetType()
        skip      = self.skip_spinner.GetValue()

        # Send a transaction to manta.
        cbArgs = ( self.channel, prefix, file_type, skip )
        self.engine.addTransaction("Capture Start",
                                   manta_new(createMantaTransaction(self.MantaStart, cbArgs)),
                                   TransactionBase.CONTINUE )
        
        cbArgs = ( self.channel, )
        self.engine.addTransaction("Capture Stop",
                                   manta_new(createMantaTransaction(self.MantaStop, cbArgs)),
                                   TransactionBase.CONTINUE )


    ###########################################################################
    # Manta Transactions
    def MantaStart( self, channel, prefix, file_type, skip, use_timestamp ):

        # Create a file display
        file_display = manta_new(FileDisplay( str(prefix), str(file_type), 0, skip, use_timestamp ))

        # Obtain the current image display.
        current = self.engine.getImageDisplay( channel )

        # Create a multi-display.
        self.multi = manta_new(MultiDisplay())
        self.multi.add( current )
        self.multi.add( file_display )

        # Set the new image display for the channel.
        self.engine.setImageDisplay( channel, self.multi )
        
    def MantaStop( self, channel ):

        # Replace the image display with the previous.
        self.engine.setImageDisplay( channel, self.multi.get(0) )

        # Delete the temporary displays
        manta_delete( self.multi.get( 1 ) )
        manta_delete( self.multi )

        self.multi = None

    # How to overload this function??
    def Destroy(self):
        
        # Stop recording.
        if (self.stop_button.IsEnabled()):
            self.OnStopButton(())
            
        wx.Panel.Destroy()
            
class MantaCaptureFrame(wx.Frame):
    def __init__(self, parent, engine, channel=0 ):
        wx.Frame.__init__(self, parent=parent, title="Capture Frames")    

        panel = wx.Panel( self )
        
        # Create a Capture Panel.
        self.capture_panel = MantaCapturePanel( panel, engine, channel )

        self.start_button = wx.Button( panel, -1, "Start" )
        self.stop_button  = wx.Button( panel, -1, "Stop" )
        self.stop_button.Disable()
        self.capture_button = wx.Button( panel, -1, "Capture One Frame")

        vsizer = wx.BoxSizer(wx.VERTICAL)
        vsizer.Add( self.capture_panel, 0, wx.EXPAND )

        hsizer = wx.BoxSizer(wx.HORIZONTAL);        
        hsizer.Add( self.start_button, wx.ALIGN_CENTER )
        hsizer.Add( self.stop_button, wx.ALIGN_CENTER )
        # hsizer.Add( self.capture_button, wx.ALIGN_CENTER )

        vsizer.Add( hsizer, 0, wx.EXPAND )

        closeButton = wx.Button(panel, wx.ID_CLOSE)
        vsizer.Add(closeButton, 0, wx.ALIGN_CENTER|wx.ALL, 0)
        self.Bind(wx.EVT_BUTTON, self.OnCloseWindow, closeButton)

        panel.SetSizerAndFit( vsizer )

        self.SetClientSize(panel.GetSize())

        # Bind events.
        self.Bind(wx.EVT_BUTTON, self.OnStartButton,   self.start_button)
        self.Bind(wx.EVT_BUTTON, self.OnStopButton,    self.stop_button)
        self.Bind(wx.EVT_BUTTON, self.OnCaptureButton, self.capture_button)
        self.Bind(wx.EVT_CLOSE,  self.OnCloseWindow)
        panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        panel.SetFocus()

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Show(False)
        evt.Skip()

    def OnStartButton( self, event ):
      try:
        self.capture_panel.OnStartButton( event )
        
        self.start_button.Disable()
        self.stop_button.Enable()
        self.capture_button.Disable()
      except:
        print "Error encountered, make sure you have sufficient write priviledges"
        dlg = wx.MessageDialog(self, "Error: you probably don't have correct priviledges", "Error saving frames", wx.OK)
	dlg.ShowModal()
	dlg.Destroy()


    def OnStopButton( self, event ):
      try:
        self.capture_panel.OnStopButton( event )

        self.start_button.Enable()
        self.stop_button.Disable()
        self.capture_button.Enable()
      except:
        print "Error encountered, make sure you have sufficient write priviledges"
        dlg = wx.MessageDialog(self, "Error: you probably don't have correct priviledges", "Error saving frames", wx.OK)
        dlg.ShowModal()
        dlg.Destroy()

        

    def OnCaptureButton( self, event ):

        self.capture_panel.OnCaptureButton( event )
        

    def OnCloseWindow(self, event):
            
        self.Show( False )
    

def ResampleCapturedFrames( prefix, performance ):

    # Determine the prefix directory and file name.
    m = re.match("(.*)\/(.*)$", prefix);
    if (m):
        prefix_dir  = m.group(1)
        prefix_file = m.group(2)
    else:
        prefix_dir = os.getcwd()
        prefix_file = prefix

    print prefix_dir
    print prefix_file

    # Make a list of matching files.
    file_list = [];
    pattern = re.compile( "^" + prefix_file + "_\d+" );

    file_type = "png"

    # Examine the prefix directory
    files = os.listdir( prefix_dir )
    for name in files:
        # Check to see if each file matches the prefix.
        if (pattern.match( name )):
            # print "Found: " + name
            file_list.append( name )

    # Sort by alpha.
    file_list.sort(key=str.lower)

    # Check to see if the number of performance numbers matches the
    # the number of files.
    # if (len(file_list) < performance.size()):
    #     print "Length mismatch"

    # print "performance: " + str(performance.size())

    # Find the total time.
    total_time = 0.0
    for i in range(performance.size()):
        performance[i] = (1.0/performance[i])
        total_time += performance[i]

    # Determine total number of frames.
    frame_time = 1.0/30.0;
    total_frames = total_time / frame_time;

    # print "Total time: " + str(total_time)
    # print "Total frames: " + str(total_frames)

    frame_counter = 0

    # Copy frames as necessary.
    i = 0
    while (i < performance.size()):

        times_displayed = performance[i] / frame_time
        # print "performance:     " + str(performance[i])
        # print "Times displayed: " + str(times_displayed)

        # if (times_displayed < 1.0):

        # Skip frames until "frametime" ms has passed.
        # skip = int(1.0/times_displayed)
        # print "skip: " + str(skip)
        # i += skip
        # times_displayed = 1;
            
        for t in range(int(times_displayed)):
            # Output current frame
            output_file = prefix_file + "_resampled_" + "%010d" % frame_counter + "." + file_type
            # print file_list[i] + " -> " + output_file

            # Copy file.
            try:
                shutil.copyfile(file_list[i],output_file)
            except:
                print "Error: " + file_list[i] + " -> " + output_file
            frame_counter += 1

        # Delete the temporary file.
        os.unlink( file_list[i] )
        i += 1

    print "Output frames: " + str(frame_counter)

    return 1.0 / frame_time;
         



    

    
        
