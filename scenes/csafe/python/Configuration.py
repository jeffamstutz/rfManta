
import sys, os, time, traceback, types, random, re
import wx
import SceneInfo
import Histogram
import TransferF
import wxManta

from csafe import *
from manta import *

from pycallback import *

def WriteConfiguration(scene, filename):

    # Make sure the 'filename' ends with ".cfg".  If not, add it.
    if ( not re.compile(".+\.cfg$").match( filename ) ) :
        filename = filename + ".cfg"

    print "Saving " + filename

    f = open(filename, 'w')
    ts = scene.frame.transferFunctions
    for i in range(len(ts)):
        t = ts[i]
        f.write('[Transfer Function]\n')
        f.write(t.label+'\n')
        f.write(str(t.id)+'\n')
        f.write(str(len(t.colors))+'\n')
        for j in range(len(t.colors)):
            c = t.colors[j]
            f.write(str(c[0])+' '+str(c[1])+' '+str(c[2])+' '+str(c[3])+' '+str(c[4])+'\n')
        f.write('\n\n')
    histoGroups = scene.frame.histoGroups

    f.write('[currentHistogram]\n')
    if (scene.currentHistogram != None):
      f.write( str( scene.frame.histoGroups.index( scene.currentHistogram ) ) )
    else :
      f.write("None")
    f.write('\n\n')

    f.write('[currentParticleHistogram]\n')
    if (scene.currentParticleHistogram != None):
    	f.write( str( scene.frame.histoGroups.index( scene.currentParticleHistogram ) ) )
    else :
	f.write("None")
    f.write('\n\n')

    f.write('[currentVolumeHistogram]\n')
    if (scene.currentVolumeHistogram != None):
    	f.write( str( scene.frame.histoGroups.index( scene.currentVolumeHistogram ) ) )
    else :
	f.write("None")
    f.write('\n\n')

    for i in range(len(histoGroups)):
        h = histoGroups[i]
        f.write('[Histogram]\n')
        f.write(h.title+'\n')
        f.write(str(h.varIndex)+'\n')
        f.write(str(h.group)+'\n')
        f.write(str(h.histogram.zoomDMin)+' '+str(h.histogram.zoomDMax)+'\n')
        f.write(str(h.histogram.cropDMin)+' '+str(h.histogram.cropDMax)+'\n')
        f.write(str(h.histogram.colorDMin)+' '+str(h.histogram.colorDMax)+'\n')
        f.write(str(h.histogram.absoluteDMin)+' '+str(h.histogram.absoluteDMax)+'\n')
        f.write(str(h.transferFID)+'\n')
        f.write('\n\n')

    f.write('[Eye]\n')
    data = scene.engine.getCamera(0).getBasicCameraData()        
    f.write( str(data.eye.x())    + " " + str(data.eye.y())    + " " + str(data.eye.z())  + "\n" )
    f.write( str(data.lookat.x()) + " " + str(data.lookat.y()) + " " + str(data.lookat.z()) + "\n" )
    f.write( str(data.up.x())     + " " + str(data.up.y())     + " " + str(data.up.z()) + "\n" )
    f.write( str(data.hfov)       + " " + str(data.vfov) + "\n" )
    f.write( '\n' )

    f.write('[Scene Properties]\n')
    f.write(str(scene.forceDataMin)+'\n')
    f.write(str(scene.forceDataMax)+'\n')
    f.write(str(scene.duration)+'\n')
    f.write(str(scene.ridx)+'\n')
    f.write(str(scene.radius)+'\n')
    f.write(str(int(scene.cidx))+'\n')
    f.write(str(scene.numThreads)+'\n')
    if (scene.labels == True):
        f.write("True\n")
    else:
        f.write("False\n")
    for j in range(3):
        f.write(str(scene.volumeMinBound[j])+" ")
    f.write("\n")
    for j in range(3):
        f.write(str(scene.volumeMaxBound[j])+" ")
    f.write("\n")
    f.write(str(len(scene.nrrdFiles2))+'\n')
    for i in range(len(scene.nrrdFiles2)):
        f.write(scene.nrrdFiles2[i]+'\n')
    print scene.nrrdFiles
    print len(scene.nrrdFiles)
    f.write(str(len(scene.nrrdFiles))+'\n')
    for i in range(len(scene.nrrdFiles)):
        f.write(scene.nrrdFiles[i]+'\n')
    f.close()

def ReadConfiguration(scene, filename):
    scene.readConfiguration = True
    try:
        f = open(filename, 'r')
    except IOError, (errno, strerror):
        print ""
        print "ERROR opening configuration file '%s': %s.  Goodbye." % (filename, strerror)
        print ""
        os._exit( -1 ) # We're done... force exit.
    current_line = 0
    try:

        ### Clear old values
        # for i in range(len(scene.frame.transferFunctions)):
        #    scene.frame.transferFunctions[i].Destroy()
        scene.frame.transferFunctions = []
        for i in range(len(scene.frame.histoGroups)):
            scene.frame.histoGroups[i].Destroy()
        scene.frame.histoGroups = []
        scene.nrrdFiles = []
        scene.nrrdFiles2 = []

        ### Read in new values
        lines = f.readlines()
        currentHistogramIndex = -1
        currentParticleHistogramIndex = -1
        currentVolumeHistogramIndex = -1
        for i in range(len(lines)):
            current_line = i
            ##### Transfer Function####
            if lines[i].find("[Transfer Function]") != -1:
                i+=1
                name = lines[i].strip();     i += 1
                id = int(lines[i].strip());  i += 1
                num = int(lines[i].strip()); i += 1
                colors = []
                slices = manta_new(vector_ColorSlice())
                for j in range(num):
                    line = lines[i].split(); i += 1
                    pos = float(line[0])
                    r = float(line[1])
                    g = float(line[2])
                    b = float(line[3])
                    a = float(line[4])
                    #colors.append( (pos, r, g, b, a) )
                    slices.push_back(ColorSlice(pos, RGBAColor(Color(RGBColor(r,g,b)), a)))

                scene.frame.transferFunctions.append(TransferF.TransferF(scene.frame, colors, id, name, manta_new(RGBAColorMap(slices))))

            #### Current Histogram ####
            elif lines[i].find("[currentHistogram]") != -1:
                i +=1
                line = lines[i].strip()
                if line != "None":
                  currentHistogramIndex = int( line)

            #### Current Histogram ####
            elif lines[i].find("[currentParticleHistogram]") != -1:
                i +=1
                line = lines[i].strip()
                if line != "None":
                  currentParticleHistogramIndex = int( line )

            #### Current Histogram ####
            elif lines[i].find("[currentVolumeHistogram]") != -1:
                i +=1
                line = lines[i].strip()
                if line != "None":
                  currentVolumeHistogramIndex = int( line )

            #### Histogram ####
            elif lines[i].find("[Histogram]") != -1:
                i +=1
                name = lines[i].strip();               i += 1
                index = int(float(lines[i].strip()));  i += 1
                group = int(float(lines[i].strip()));  i += 1
                zoomInto = lines[i].split();           i += 1
                zoomIntoMin = float(zoomInto[0])
                zoomIntoMax = float(zoomInto[1])
                cropDisplay = lines[i].split();        i += 1
                cropDisplayMin = float(cropDisplay[0])
                cropDisplayMax = float(cropDisplay[1])
                cropColor = lines[i].split();          i += 1
                cropColorMin = float(cropColor[0])
                cropColorMax = float(cropColor[1])
                absMinMax = lines[i].split(); 		     i += 1
                absMin = float(absMinMax[0])
                absMax = float(absMinMax[1])
                transferFID = int(lines[i].strip())
                histValues1 = []
                for i in range(scene.histogramBuckets):
                    histValues1.append(5.0)
                histoGroup = Histogram.HistogramGroup(scene.frame.panel, scene, index, name, transferFID)
                histoGroup.SetBackgroundColour(wx.Colour(90,90,90))
                histoGroup.group = group

                if( len( scene.frame.histoGroups ) == currentHistogramIndex ) :
                    scene.currentHistogram = histoGroup

                if( len( scene.frame.histoGroups ) == currentParticleHistogramIndex ) :
                    scene.currentParticleHistogram = histoGroup

                if( len( scene.frame.histoGroups ) == currentVolumeHistogramIndex ) :
                    scene.currentVolumeHistogram = histoGroup

                scene.frame.histoGroups.append(histoGroup)

                #TODO: read these in
                if (group == 1):
                    scene.test.setVolCMap(scene.frame.transferFunctions[transferFID].cmap)
                    scene.frame.transferFunctions[transferFID].cmap.scaleAlphas(.00125)
                # if (group == 0):
                #     scene.test.setClipMinMax(index, cropDisplayMin, cropDisplayMax)
                #     scene.test.setSphereCMinMax(index, cropColorMin, cropColorMax)
                # else:
                #     scene.test.setVolCMinMax(cropColorMin, cropColorMax)

                print "sending values: " + str(zoomIntoMin) + " " + str(zoomIntoMax) + " " + str(cropDisplayMin) + " " + str(cropDisplayMax) + " " + str(cropColorMin) + " " + str(cropColorMax)

                histoGroup.SendValues(zoomIntoMin, zoomIntoMax, cropDisplayMin, cropDisplayMax, cropColorMin, cropColorMax)
#                print "histo abs values: " + str(histoGroup.histogram.absoluteDMin)+ " " + str(histoGroup.histogram.absoluteDMax)
                histoGroup.histogram.absoluteDMin = absMin
                histoGroup.histogram.absoluteDMax = absMax
#                print "histo abs values after: " + str(histoGroup.histogram.absoluteDMin)+ " " + str(histoGroup.histogram.absoluteDMax)

            ##### Eye (Camera) ####
            elif lines[i].find("[Eye]") != -1:
                i += 1
                line = lines[i].split();                 i += 1
                x = float(line[0])
                y = float(line[1])
                z = float(line[2])
                eye = manta_new( Vector(x, y, z) )

                line = lines[i].split();                 i += 1
                x = float(line[0])
                y = float(line[1])
                z = float(line[2])
                lookat = manta_new( Vector(x, y, z) )

                line = lines[i].split();                 i += 1
                x = float(line[0])
                y = float(line[1])
                z = float(line[2])
                up     = manta_new( Vector(x, y, z) )

                line = lines[i].split()
                h = float(line[0]) # h-fov
                v = float(line[1]) # v-fov

                cbArgs = ( manta_new( BasicCameraData( eye, lookat, up, h, v ) ), )
                scene.engine.addTransaction( "Set basic camera data",
                                             manta_new( createMantaTransaction( scene.engine.getCamera(0).setBasicCameraData,
                                                                            cbArgs)))

        ##### Scene Properties ####
        # DataMin
        # DataMax
        # duration
        # ridx
        # radius
        # cidx
        # numThreads
        # minBound
        # maxBound
        # numVol files
        #  nrrd name(s)
        # numParticle files
        #  nrrd name(s)
            elif lines[i].find("[Scene Properties]") != -1:
                i+=1
                scene.forceDataMin = float(lines[i].strip());    i += 1
                scene.forceDataMax = float(lines[i].strip());    i += 1
                if (scene.forceDataMin != scene.forceDataMax):
                    scene.test.setVolColorMinMax(scene.forceDataMin, scene.forceDataMax)
                scene.duration = float(lines[i].strip());        i += 1
                scene.test.setDuration(scene.duration)
                scene.ridx = int(float(lines[i].strip()));       i += 1
                scene.test.setRidx(scene.ridx)
                scene.radius = float(lines[i].strip());          i += 1
                scene.test.setRadius(scene.radius)
                scene.cidx = int(float(lines[i].strip()));       i += 1
                scene.test.setCidx(scene.cidx)
                scene.numThreads = int(float(lines[i].strip())); i += 1
                scene.engine.changeNumWorkers(scene.numThreads)
                labels = lines[i].strip(); i+=1
                if labels == True:
                  scene.labels = True
                else:
                  scene.labels = False
                minBound = lines[i].split();                     i += 1
                maxBound = lines[i].split();                     i += 1
                for j in range(3):
                    scene.volumeMinBound[j] = float(minBound[j])
                    scene.volumeMaxBound[j] = float(maxBound[j])
                minVector = Vector(scene.volumeMinBound[0], scene.volumeMinBound[1], scene.volumeMinBound[2])
                maxVector = Vector(scene.volumeMaxBound[0], scene.volumeMaxBound[1], scene.volumeMaxBound[2])
                scene.test.setVolumeMinMaxBounds(minVector, maxVector)
                print "setting numWorkers to: " + str(scene.numThreads)
                numVol = int(float(lines[i].strip()));           i += 1

                for j in range(numVol):
                    nrrd = lines[i].strip();                     i += 1
                    if not os.path.exists( nrrd ) :
                        print ""
                        print "ERROR file '" + nrrd + "' does not exist.  Config file appears to be invalid.  Goodbye."
                        print ""
                        os._exit( -1 ) # We're done... force exit.
                    scene.nrrdFiles2.append( nrrd )

                numSphere = int(float(lines[i].strip()));        i += 1
                for j in range(numSphere):
                    nrrd = lines[i].strip();                     i += 1
                    if not os.path.exists( nrrd ) :
                        print ""
                        print "ERROR file '" + nrrd + "' does not exist.  Config file appears to be invalid.  Goodbye."
                        print ""
                        os._exit( -1 ) # We're done... force exit.
                    scene.nrrdFiles.append( nrrd )

        scene.frame.LayoutWindow()
        if scene.nrrdFiles > 1 or scene.nrrdFiles2 > 1:
            scene.frame.generateMenuItem.Enable(True)
        print "Done reading Configuration file"

    except:
        print "ERROR: unable to read Configuration file, failed at line: " + str(current_line)


def ReadNRRDList(scene, filename):
    try:
        f = open(filename, 'r')

        ###clear old values
        scene.nrrdFiles = []
        scene.nrrdFiles2 = []

        ###read in new values
        lines = f.readlines()
        i = 0
        while i in range(len(lines)-1):
            sphere = ""
            vol = ""
            vol = lines[i].strip()
            i += 1
            while (vol == "" and i < len(lines)-1):
                vol = lines[i].strip()
                i += 1
            sphere = lines[i].strip()
            i += 1
            while(sphere == "" and i < len(lines) -1):
              sphere = lines[i].strip()
              i += 1
            if (sphere == "" or vol == ""):
                return
            if (sphere[len(sphere)-1] == "w") : #raw file look for nhdr
                sphere = sphere.strip(".raw")
                sphere = sphere + ".nhdr"

            print "volume file: " + str(vol) + " sphere file: " + str(sphere)
            scene.nrrdFiles.append(sphere)
            scene.nrrdFiles2.append(vol)
            if scene.nrrdFiles > 1 or scene.nrrdFiles2 > 1:
              scene.frame.generateMenuItem.Enable(True)
    except:
        print "ERROR: This appears to be an invalid NRRDList"

def WriteNRRDList(scene, filename):
    f = open(filename, 'w')
    if (len(scene.nrrdFiles) != len(scene.nrrdFiles2)):
        print "ERROR: nrrdlists need to have the same number of sphere and volume files"
    else:
        for i in range(len(scene.nrrdFiles)):
            f.write(str(scene.nrrdFiles2[i])+"\n")
            f.write(str(scene.nrrdFiles[i])+"\n")
