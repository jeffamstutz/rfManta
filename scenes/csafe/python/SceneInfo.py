import wx

class Scene(wx.Object):
  def __init__(self):
    self.volVar = 8  #index used to denote volume
    self.nrrdFiles = []
    self.test = None
    self.ridx = -1
    self.radius = 0.001
    self.cidx = -1
    self.nrrdFiles2 = []
    self.numFrames = 0
    self.engine = None
    self.duration = 10.0
    self.numThreads = 1
    self.sceneName = "Untitled"
    self.sceneWD = "/"
    self.histogramBMPLoaded = False
    self.bgColor = wx.Colour(90,90,90)
    self.startFrame = self.endFrame = 0
    self.clipFrames = False  #true if start/end are set
    self.lockFrames = False
    self.loop = True
    self.repeatLastFrame = 0.0
    self.mantaFrame = None
    self.useAO = False
    self.aoCutoff = 0.01
    self.aoNum = 10
    self.showSpheres = True
    self.showVolume = True
    self.biggify = False # make text bigger
    self.minX = -1.0
    self.minY = -1.0
    self.minZ = -1.0
    self.maxX = 1.0
    self.maxY = 1.0
    self.maxZ = 1.0
    self.histogramBuckets = 300
    self.volumeMinBound = [-0.001, -0.101, -0.001]
    self.volumeMaxBound = [0.101, 0.201, 0.101]
    self.forceDataMin = float(0) #manually set volume data range
    self.forceDataMax = float(0)
    self.stepSize = float(0.0015)
    self.currentHistogram = None
    self.autoBuildHistograms = True
    self.readConfiguration = False
    self.measurementsFrame = None
    self.selectedBGColor = wx.Color(20,70,120)
    self.labels = True
    self.loaded = False
    self.currentParticleHistogram = None
    self.currentVolumeHistogram = None
