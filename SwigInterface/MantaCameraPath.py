

import wx
import sys
import time

import threading
import math
import wx.lib.colourselect as csel
import os

from manta import *
from pycallback import *

from MantaCapture import *
# from MantaPlot import *

import FloatSpin as FS

    

###############################################################################
# Manta Image Sequence Capture Frame.
# This class ports functionality of FMantaCapture dialog to wxManta.

class MantaCameraPathPanel(wx.Panel):
    def __init__(self, parent, engine, channel=0 ):

        wx.Panel.__init__(self, parent )

        self.engine  = engine
        self.channel = channel
        self.parent  = parent
        self.record_delta_t = 0.0
	self.record_delta_time = 0.0

        # Load/Create Path.
        self.load_path_button = wx.Button( self, -1, "Load Path" )
        self.new_path_button  = wx.Button( self, -1, "New Path" )
        self.save_path_button  = wx.Button( self, -1, "Save Path" )
        self.record_interval  = wx.SpinCtrl( self, -1 )
        self.record_interval.SetRange( 1, 60000 )
        self.record_interval.SetValue( 500 )

        self.record_timer = wx.Timer( self )
        self.new_path_counter = 0
        
        # List of available paths.
        self.path_list = wx.ListCtrl( self, -1, size=(400,-1),style=wx.LC_REPORT|wx.LC_EDIT_LABELS )
        self.path_list.InsertColumn( 0, "Path Name" )
        self.path_list.InsertColumn( 1, "Key Frames" )
        self.path_list.InsertColumn( 2, "Delta time" )
        self.path_list.InsertColumn( 3, "Delta t" )

        self.path_list.SetColumnWidth(0, 100)
        self.path_list.SetColumnWidth(1, 100)
        self.path_list.SetColumnWidth(2, 100)
        self.path_list.SetColumnWidth(3, 100)

        self.path_loop_check = wx.CheckBox( self, -1, "Play in a loop." )

        self.automator = []; # Empty list.

        # Capture frames from a path.
        self.capture_frames_check = wx.CheckBox( self, -1, "Capture frames from path" )
        self.capture_panel = MantaCapturePanel( self, engine, channel );
        self.capture_panel.Disable()
        self.capture_separate_check = wx.CheckBox( self, -1, "Record in multiple passes." )
        self.capture_pass = 0

        # Run a benchmark using a path.
        self.benchmark_check = wx.CheckBox( self, -1, "Benchmark path" )
        self.benchmark_list = wx.ListCtrl( self, -1, size=(400,-1),style=wx.LC_REPORT|wx.LC_EDIT_LABELS )
        self.benchmark_list.InsertColumn( 0, "Path Name" )
        self.benchmark_list.InsertColumn( 1, "Total Samples" )        
        self.benchmark_list.InsertColumn( 2, "Average FPS" )

        self.benchmark_list.SetColumnWidth(0, 100)
        self.benchmark_list.SetColumnWidth(1, 100)
        self.benchmark_list.SetColumnWidth(2, 100)
        
        self.benchmarks = []; # Empty list.

        # Just run a freaking path!
        self.start_button = wx.Button( self, -1, "Start" )
        self.pause_button = wx.Button( self, -1, "Pause" )
        self.stop_button  = wx.Button( self, -1, "Stop"  )
        
        # Arrange gui components.
        vsizer = wx.BoxSizer(wx.VERTICAL);

        # Load, New Buttons.
        hsizer = wx.BoxSizer(wx.HORIZONTAL);
        hsizer.Add( self.load_path_button, wx.ALIGN_CENTER )
        hsizer.Add( self.new_path_button, wx.ALIGN_CENTER )
        vsizer.Add( hsizer, wx.ALIGN_CENTER )        

        hsizer = wx.BoxSizer(wx.HORIZONTAL);        
        hsizer.Add( wx.StaticText( self, -1, "Record Interval " ), wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER)
        hsizer.Add( self.record_interval, 0, wx.ALIGN_CENTER )
        hsizer.Add( wx.StaticText( self, -1, "ms" ), wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER)        
        hsizer.Add( self.save_path_button, wx.ALIGN_CENTER )
        vsizer.Add( hsizer, wx.ALIGN_CENTER )

        # Path List.
        vsizer.Add( self.path_list, 0, wx.EXPAND )
        vsizer.Add( self.path_loop_check, 0, wx.ALIGN_LEFT )

        self.path_list.Bind( wx.EVT_RIGHT_UP, self.OnPathRightClick )

        # Capture.
        hsizer = wx.BoxSizer(wx.HORIZONTAL);
        hsizer.Add( self.capture_frames_check, wx.ALIGN_CENTER )
        hsizer.Add( self.capture_separate_check, wx.ALIGN_CENTER )
        vsizer.Add( hsizer, wx.ALIGN_CENTER )
        vsizer.Add( self.capture_panel, wx.EXPAND )

        # Benchmark.
        hsizer = wx.BoxSizer(wx.HORIZONTAL);
        hsizer.Add( self.benchmark_check, wx.ALIGN_CENTER )
        vsizer.Add( hsizer, wx.ALIGN_CENTER )
        vsizer.Add( self.benchmark_list, 0, wx.EXPAND )

        # self.benchmark_list.Bind( wx.EVT_RIGHT_UP, self.OnListRightClick )

        # Controls.
        hsizer = wx.BoxSizer(wx.HORIZONTAL);
        hsizer.Add( self.start_button, wx.ALIGN_CENTER )
        hsizer.Add( self.pause_button, wx.ALIGN_CENTER )
        hsizer.Add( self.stop_button, wx.ALIGN_CENTER )
        vsizer.Add( hsizer, wx.ALIGN_CENTER )

        self.SetSizerAndFit(vsizer)

        # Bind events.
        self.Bind(wx.EVT_BUTTON, self.OnLoadPathButton,          self.load_path_button)
        self.Bind(wx.EVT_BUTTON, self.OnNewPathButton,           self.new_path_button)
        self.Bind(wx.EVT_BUTTON, self.OnSavePathButton,          self.save_path_button)        
        self.Bind(wx.EVT_BUTTON, self.OnStartButton,             self.start_button)
        self.Bind(wx.EVT_BUTTON, self.OnStopButton,              self.stop_button)

        self.Bind(wx.EVT_CHECKBOX, self.OnCaptureFramesCheck, self.capture_frames_check)
        self.Bind(wx.EVT_TIMER, self.OnRecordTimer )

    ###########################################################################
    # Gui interactions.
    def OnLoadPathButton( self, event ):

        # Get the filename.
        dialog = wx.FileDialog( self,
                                message="Choose path file",
                                defaultDir=os.getcwd(),
                                defaultFile="",
                                wildcard="Text files (*.txt)|*.txt|All Files (*.*)|*.*",
                                style=wx.OPEN|wx.CHANGE_DIR )
        
        # Determine the file to open.
        if (dialog.ShowModal() == wx.ID_OK):

            # Attempt to add a path per file selected.
            files = dialog.GetPaths()
            index = -1

            for name in files:
                # Parse the file.
                try:
                    automator = manta_new(CameraPathAutomator( self.engine, 0, 0, str(name) ))
                    self.AddAutomator( automator, name )
                except:
                    print "Failed to load: " + name
                
            # Check to see if any were successful.
            if (index >= 0):
                # Enable Buttons.
                self.start_button.Enable()
                self.path_list.SetItemState( index, wx.LIST_STATE_SELECTED, wx.LIST_STATE_SELECTED )


    def AddAutomator( self, automator, name ):

        # Specifiy that the automator should wait for additional commands after running.
        automator.set_automator_mode( AutomatorUI.AUTOMATOR_KEEPALIVE )

        cbArgs = ( automator, )
        automator.set_terminate_callback( manta_new(createMantaTransaction(self.MantaCameraPathComplete, cbArgs )))
        
        # Add the automator the list.
        index = self.path_list.InsertStringItem( sys.maxint, name )
        self.path_list.SetStringItem( index, 1, str(automator.get_total_points()) );
        self.path_list.SetStringItem( index, 2, str(automator.get_delta_time()) );
        self.path_list.SetStringItem( index, 3, str(automator.get_delta_t()) );
        
        # Store the automator
        self.automator.append(automator)

    def OnNewPathButton( self, event ):

        if (self.record_timer.IsRunning()):

            # Stop recording.
            self.record_timer.Stop()

            # Toggle buttons.
            self.new_path_button.SetLabel( "New Path" )

            
            # Create a new automator.
            automator = CameraPathAutomator( self.engine, 0, 0,
                                             self.new_path_data,
                                             self.record_delta_t,
                                             self.record_delta_time )
            self.AddAutomator( automator, "NewPath" + str(self.new_path_counter) + ".txt" )
            self.new_path_counter += 1

            # Remove temporary storage.
            self.new_path_data.Clear()

        else:
            # Determine the interval
            interval = self.record_interval.GetValue()

            # Create a camera data list.
            self.new_path_data = CameraPathDataVector()
            
            # Add a timer to record camera position information.
            self.record_timer.Start( interval )
            self.record_prev_frame = self.engine.getFrameState().frameSerialNumber
            self.record_prev_time  = time.time()
            
            # Toggle buttons.
            self.new_path_button.SetLabel( "Done" )

    def OnRecordTimer( self, event ):

        # Record camera position.
        camera_data = self.engine.getCamera( self.channel ).getBasicCameraData()

        # Compute elapse time and frames.
        current_frame = self.engine.getFrameState().frameSerialNumber
        current_time  = time.time()
        
        elapse_frames = current_frame - self.record_prev_frame;
        elapse_time   = current_time  - self.record_prev_time;

        if (elapse_frames > 0):

            # Compute delta t and time.
            self.record_delta_t    = 1.0 / elapse_frames;
            self.record_delta_time = elapse_time / elapse_frames;
            
            # Add the position to the current path.
            self.new_path_data.PushBack( camera_data )

        self.record_prev_frame = current_frame;
        self.record_prev_time  = current_time;

    def OnSavePathButton( self, event ):
        
        # Determine the currently selected automator.
        index = self.path_list.GetNextItem( -1, wx.LIST_NEXT_ALL, wx.LIST_STATE_SELECTED )
        if (index >= 0):
            
            automator = self.automator[index]
            filename = self.path_list.GetItemText( index )

            dialog = wx.FileDialog( self,
                                    message = "Save path as...",
                                    defaultDir = os.getcwd(),
                                    defaultFile = filename,
                                    wildcard="Text files (*.txt)|*.txt|All Files (*.*)|*.*",
                                    style=wx.SAVE|wx.CHANGE_DIR )

            if (dialog.ShowModal() == wx.ID_OK):

                # Get the pathname.
                filename = dialog.GetPath()

                # Write to a file.
                automator.write_path( str(filename) )
        
    def OnStartButton( self, event ):

        # Determine the currently selected automator.
        index = self.path_list.GetNextItem( -1, wx.LIST_NEXT_ALL, wx.LIST_STATE_SELECTED )
        if (index >= 0):

            automator = self.automator[index]

            # Set the loop behavior.
            if (self.path_loop_check.GetValue()):
                automator.set_loop_behavior( CameraPathAutomator.PATH_LOOP )
            else:
                automator.set_loop_behavior( CameraPathAutomator.PATH_STOP )

            # Initialize Benchmark
            if (self.benchmark_check.IsChecked()):

                # Synchronzie the automator thread with Manta every frame.
                automator.set_sync_frames( 1 )
                automator.set_sync_quiet( True )
                
            else:
                automator.set_sync_frames( 0 )

            # Start camera path if necessary.
            if (self.capture_frames_check.IsChecked()):

                # Check if multiple passes should be used.
                if (self.capture_separate_check.IsChecked()):

                    # Controlled capture mode.
                    self.benchmark_check.SetValue( False )
                    self.capture_pass += 1

                    if (self.capture_pass == 1):

                        # Pass 1 collect performance data.
                        automator.set_sync_frames( 1 )
                        automator.set_sync_quiet( True )
                        
                        self.parent.statusbar.SetStatusText( "Pass 1 collect performance data." )
                        
                    if(self.capture_pass == 2):

                        # Pass 2 capture frame images.
                        automator.set_sync_frames( 1 )
                        automator.set_sync_quiet( True )

                        self.parent.statusbar.SetStatusText( "Pass 2 capture frame images." )
                        self.capture_panel.OnStartButton(())
                    
                else:
                    # Normal capture mode.
                    self.parent.statusbar.SetStatusText( "Capturing frame images." )                    
                    self.capture_panel.OnStartButton(())

            # Toggle buttons.
            self.load_path_button.Disable()
            self.new_path_button.Disable()
            self.start_button.Disable()
            self.stop_button.Enable()

            # Start the asynchronous thread.
            automator.restart()
        else:
            self.parent.statusbar.SetStatusText( "Select a Path" )
            

    def OnStopButton( self, event ):

        # Determine the currently selected automator.
        index = self.path_list.GetNextItem( -1, wx.LIST_NEXT_ALL, wx.LIST_STATE_SELECTED )
        if (index >= 0):

            # Access the automator.
            automator = self.automator[index]

            # Start camera path if necessary.
            if (self.capture_frames_check.IsChecked()):
                self.capture_panel.OnStopButton(())

            # Cause the automator to abort and call its termination callback.
            automator.set_loop_behavior ( CameraPathAutomator.PATH_ABORT )
            automator.set_automator_mode( AutomatorUI.AUTOMATOR_EXIT )

    def OnCaptureFramesCheck( self, event ):

        if (self.capture_frames_check.IsChecked()):
            self.capture_panel.Enable()
        else:
            self.capture_panel.Disable()

    def OnPathRightClick( self, event ):

        # Popup menu options.
        self.POPUP_GLYPH     = wx.NewId()
        self.Bind(wx.EVT_MENU, self.OnPopupGlyphCheck,    id=self.POPUP_GLYPH )
 
        menu = wx.Menu()
        menu.Append( self.POPUP_GLYPH, "Toggle Glyphs" )
        
        self.path_list.PopupMenu( menu )
        menu.Destroy()

    def OnPopupGlyphCheck( self, event ):

        # Get the selected automator.
        index = self.path_list.GetFirstSelected()
        automator = self.automator[index]

        # Create a Group containing glyphs for each control point.
        group = manta_new(Group())
        total = automator.get_total_points()
        for i in range(0,total):

            # Place a Cone glyph at the control point.
            c = automator.GetControlPoint( i )
            
            material = manta_new(Flat(Color(RGBColor((float(i)/float(total)),0.1,0.1))))            
            glyph = manta_new(Sphere( material, c.eye, 15.0 ))
            group.add(manta_new( glyph ))

        # Build an acceleration structure for the group.
        self.glyph_bvh = manta_new( DynBVH() )
        self.glyph_bvh.rebuild( group )

        # Add the group to manta in a transaction.
        cbArgs = ( self.glyph_bvh, )
        self.engine.addTransaction("Manta Add Glyph",
                                   manta_new(createMantaTransaction(self.MantaAddGlyph, cbArgs)))
        

    def MantaAddGlyph(self, glyph_bvh ):

        scene = self.engine.getScene()

        new_world = manta_new( Group() )
        new_world.add( glyph_bvh )
        new_world.add( scene.getObject() )

        scene.setObject( new_world )

    def OnListRightClick( self, event ):

        # Popup menu options.
        self.POPUP_RENAME    = wx.NewId()
        self.POPUP_HISTOGRAM = wx.NewId()
        self.POPUP_PLOT      = wx.NewId()

        self.Bind(wx.EVT_MENU, self.OnPopupRename,    id=self.POPUP_RENAME )
        self.Bind(wx.EVT_MENU, self.OnPopupHistogram, id=self.POPUP_HISTOGRAM )
        self.Bind(wx.EVT_MENU, self.OnPopupPlot,      id=self.POPUP_PLOT )

        menu = wx.Menu()
        # menu.Append( self.POPUP_RENAME, "Rename" )        
        menu.Append( self.POPUP_HISTOGRAM, "Histogram" )
        menu.Append( self.POPUP_PLOT,      "Plot" )
        
        self.benchmark_list.PopupMenu( menu )
        menu.Destroy()
        
    def OnPopupRename( self, event ):

        # Lookup selected item.
        index = self.benchmark_list.GetFirstSelected()        

        print "Unimplemented."

    def OnPopupPlot( self, event ):

        # Look up the performance vector.
        index = self.benchmark_list.GetFirstSelected()        
        plotable = self.benchmarks[index];

        # Look up the path name.
        plotname = self.path_list.GetItemText( index )

        # Fps Histogram
        plot_frame = PlotFrame( self, plotable, plotname );
        plot_frame.Show()

    def OnPopupHistogram( self, event ):

        # Look up the performance vector.
        index = self.benchmark_list.GetFirstSelected()        
        plotable = self.benchmarks[index];

        # Look up the path name.
        plotname = self.path_list.GetItemText( index )

        # Fps Histogram
        plot_frame = HistogramFrame( self, plotable, plotname );
        plot_frame.Show()        


    ###########################################################################
    # Manta Transactions
    def MantaCameraPathComplete( self, automator ):

        # Stop capturing frames.
        if (self.capture_frames_check.IsChecked()):

            # Check if multiple passes should be used.
            if (self.capture_separate_check.IsChecked()):
                
                # Controlled capture mode.
                if (self.capture_pass == 1):
                    
                    # Pass 1 collect performance data.
                    self.parent.statusbar.SetStatusText( "First Pass Complete" )  
                    self.capture_performance = automator.get_performance()

                    wx.CallAfter( self.OnStartButton, () )

                if (self.capture_pass == 2):
                    
                    # Pass 2 captured frame images.
                    self.capture_pass = 0
                    self.capture_panel.OnStopButton(())
                    
                    # Reprocess images.
                    self.parent.statusbar.SetStatusText( "Resampling frames." )

                    wx.CallAfter( ResampleCapturedFrames, self.capture_panel.GetPrefix(),
                                                          self.capture_performance )
                    
                    # Cleanup.
                    self.capture_performance = ()
            else:
                # Normal capture mode.
                self.capture_panel.OnStopButton(())

        # Add results to benchmark table.
        if (self.benchmark_check.IsChecked()):

            # Copy the performance data.
            performance = automator.get_performance()
            self.benchmarks.append( performance )

            # Lookup the path name.
            index = self.automator.index( automator )
            filename = self.path_list.GetItemText( index )

            # Add a row to the benchmark table.
            index = self.benchmark_list.InsertStringItem( sys.maxint, filename )
            self.benchmark_list.SetStringItem( index, 1, str(performance.size()) );
            self.benchmark_list.SetStringItem( index, 2, str(automator.get_average_fps()) );

            self.parent.statusbar.SetStatusText( "Right click to plot performance." )            

        # Renable buttons.
        self.load_path_button.Enable()
        self.new_path_button.Enable()
        self.start_button.Enable()
        self.stop_button.Disable()
            
class MantaCameraPathFrame(wx.Frame):
    def __init__(self, parent, engine, channel=0 ):
        wx.Frame.__init__(self, parent=parent, title="Camera Paths")    
        panel = wx.lib.scrolledpanel.ScrolledPanel(self, -1, style=wx.TAB_TRAVERSAL)
        # Create a Capture Panel.
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.panel = MantaCameraPathPanel( panel, engine, channel )
        self.statusbar = self.CreateStatusBar()

        sizer.Add(self.panel, 0, wx.ALIGN_CENTER|wx.ALL, 0)
        closeButton = wx.Button(panel, wx.ID_CLOSE)
        sizer.Add(closeButton, 0, wx.ALIGN_CENTER|wx.ALL, 0)
        self.Bind(wx.EVT_BUTTON, self.OnCloseWindow, closeButton)

        panel.SetSizer(sizer)
        panel.Layout()
        self.Layout()
        self.SetClientSize(self.panel.GetSize()+(0,50))

        self.Bind(wx.EVT_CLOSE,  self.OnCloseWindow)
        panel.SetFocus()
        panel.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)

    def OnKeyDown(self, evt):
        keycode = evt.GetKeyCode()
        if (keycode == wx.WXK_ESCAPE):
           self.Show(False)
        evt.Skip()



    def OnCloseWindow(self, event):
            
        self.Show( False )
    
