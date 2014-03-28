import wx
import wxManta

from manta import *
from pycallback import *

###############################################################################
###############################################################################
# LIGHT FRAME          LIGHT FRAME          LIGHT FRAME          LIGHT FRAME   
###############################################################################
###############################################################################        

class KDTreeDynFrame(wx.Frame):
    def __init__(self, parent, engine, kdtree):
        wx.Frame.__init__(self, parent=parent, title="KDTreeDyn Explorer")

        self.engine = engine
        self.kdtree = kdtree
        self.node_stack = []

        # Intialize material array.
        self.material = [  manta_new( Transparent( Color(RGBColor( 1.0, 0.0, 0.0 )), 0.5 ) ),
                           manta_new( Transparent( Color(RGBColor( 0.0, 1.0, 0.0 )), 0.5 ) ),
                           manta_new( Transparent( Color(RGBColor( 0.0, 0.0, 1.0 )), 0.5 ) ) ]

        

        panel = wx.Panel(self,-1)
        sizer = wx.BoxSizer(wx.VERTICAL)

        # Create buttons.
        reset = wx.Button(panel, -1, "Reset Root")
        sizer.Add(reset,0,wx.EXPAND)
        self.Bind(wx.EVT_BUTTON, self.OnResetRoot, reset)
        
        self.parent = wx.Button(panel, -1, "Parent")
        sizer.Add(self.parent,0,wx.EXPAND)
        self.Bind(wx.EVT_BUTTON, self.OnSetParent, self.parent)
        
        self.left = wx.Button(panel, -1, "Left")
        sizer.Add(self.left,0,wx.EXPAND)
        self.Bind(wx.EVT_BUTTON, self.OnSetLeft, self.left)        
        
        self.right = wx.Button(panel, -1, "Right")
        sizer.Add(self.right,0,wx.EXPAND)
        self.Bind(wx.EVT_BUTTON, self.OnSetRight, self.right)        

        # Create a status bar.
        self.status = self.CreateStatusBar()
        
        panel.SetSizerAndFit(sizer)
        self.SetClientSize((256,128))

        # Add a plane group.
        self.plane_group = manta_new( Group() )

        # Place the plane group in the scene.
        self.plane_group.add( self.engine.getScene().getObject() )
        self.engine.getScene().setObject( self.plane_group )

        # Update plane.
        self.engine.addTransaction( "update plane",
                                    manta_new( createMantaTransaction( self.UpdateFrame,
                                                                        () )))        
        
    def UpdateFrame(self):
        
        # Get the current root node.
        node = self.kdtree.getNode( self.kdtree.getRootOffset() )

        # Update text.
        axis = ("x", "y", "z")
        if (node.type == KDTreeDyn.LeafType):
            self.status.SetStatusText( "Leaf Depth: " + str(len(self.node_stack)) +
                                       " Size: " + str(node.leaf().listLen) )
        else:
            self.status.SetStatusText( "Size: " + str(node.size) + " Axis: " + axis[node.type] +
                                       " Split: " + str(node.internal().split) )

        self.MantaUpdatePlane()

    ###########################################################################
    ###########################################################################
    # Transaction callback.
    ###########################################################################
    ###########################################################################

    def printv( self, v ):
        print str(v.x()) + " " + str(v.y()) + " " + str(v.z())

    def MantaUpdatePlane(self):

        # Get the current root node.
        node = self.kdtree.getNode( self.kdtree.getRootOffset() )

        # Check to see if the node is a leaf.
        if (node.type != KDTreeDyn.LeafType):
        
            # Move the cutting plane.
            anchor = Vector(self.kdtree.bbox.getMin())
            anchor[node.type] = node.internal().split
            
            amax = Vector(self.kdtree.bbox.getMax())
            amax[node.type] = node.internal().split
            
            diag = amax - anchor
            
            v1i = (node.type+1)%3
            v2i = (node.type+2)%3
            
            v1 = Vector(diag)
            v2 = Vector(diag)

            v1[v1i] = 0
            v2[v2i] = 0

            # Create the parallelogram
            parallelogram = manta_new(Parallelogram( self.material[node.type],
                                                     anchor, v1, v2 ))
            
            # Preprocess
            context = PreprocessContext( self.engine, self.engine.getScene().getLights() )
            parallelogram.preprocess( context )
            
            # Check to see if the plane group already has a parallelogram.
            if (self.plane_group.getSize() == 2):
                old = self.plane_group.get(1)
                manta_delete( old )
                self.plane_group.set(1,parallelogram)
            else:
                self.plane_group.add(parallelogram)
                
    ###########################################################################
    ###########################################################################
    ## Event Callbacks
    ###########################################################################
    ###########################################################################

    def OnShow(self, event):
        self.Show()

    def OnResetRoot(self, event):

        # Check that we're not already at the root.
        if (len(self.node_stack) > 0):

            # Reset the node stack.
            offset = self.node_stack[0]
            self.node_stack = []
            
            
            self.engine.addTransaction( "reset root",
                                        manta_new( createMantaTransaction( self.kdtree.setRootOffset,
                                                                       (offset, ) )))
            self.engine.addTransaction( "Update",
                                        manta_new( createMantaTransaction( self.UpdateFrame, () )))
    def OnSetParent(self, event):

        if (len(self.node_stack) > 0):
            offset = self.node_stack.pop()

            self.engine.addTransaction( "set parent",
                                        manta_new( createMantaTransaction( self.kdtree.setRootOffset,
                                                                       (offset, ) )))
            self.engine.addTransaction( "Update",
                                        manta_new( createMantaTransaction( self.UpdateFrame, () )))
            
    def OnSetLeft(self, event):

        # Get the current node.
        node = self.kdtree.getNode( self.kdtree.getRootOffset() )

        if (node.type != KDTreeDyn.LeafType):

            # Push on stack.
            self.node_stack.append( self.kdtree.getRootOffset() )
            
            # Get the left node.
            offset = node.internal().leftOffset;

            self.engine.addTransaction( "set left",
                                        manta_new( createMantaTransaction( self.kdtree.setRootOffset,
                                                                       (offset, ) )))
            self.engine.addTransaction( "Update",
                                        manta_new( createMantaTransaction( self.UpdateFrame, () )))            

    def OnSetRight(self, event):
        
        # Get the current node.
        node = self.kdtree.getNode( self.kdtree.getRootOffset() )

        if (node.type != KDTreeDyn.LeafType):

            # Push on stack.
            self.node_stack.append( self.kdtree.getRootOffset() )
            
            # Get the left node.
            offset = node.internal().leftOffset+1;

            self.engine.addTransaction( "set left",
                                        manta_new( createMantaTransaction( self.kdtree.setRootOffset,
                                                                       (offset, ) )))
            self.engine.addTransaction( "Update",
                                        manta_new( createMantaTransaction( self.UpdateFrame, () )))




            
