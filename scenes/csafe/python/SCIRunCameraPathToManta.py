#! /usr/bin/python

#
#  converts a scirun camera path file to a manta camera path file.
#
#  usage: script.py [scirun camerapath file] [output manta camera path filename]
#  by: Carson Brownlee (brownleeATcs.utah.edu)
#

import sys

if len(sys.argv) < 3:
    print "incorrect command line arguments.  \n correct usage: [inputfile] [outputfile]"
    sys.exit(1)

fin = open(sys.argv[1], 'r')
fout = open(sys.argv[2], 'w')

lines = fin.readlines()
num = int(float(lines[0]))
fout.write('delta_t(1.0000000)\n')
fout.write('delta_time(0.50000)\n')
for i in range(1,num):
    line = lines[i].split()
    e_x = float(line[0])
    e_y = float(line[1])
    e_z = float(line[2])

    l_x = float(line[3])
    l_y = float(line[4])
    l_z = float(line[5])

    up_x = float(line[6])
    up_y = float(line[7])
    up_z = float(line[8])

    fov = float(line[9])
    wtf = float(line[10])
    fout.write('control( -eye '+str(e_x)+' '+str(e_y)+' '+str(e_z)+' -lookat '+str(l_x) \
                   +' '+str(l_y)+' '+str(l_z)+' -up '+str(up_x)+' '+str(up_y)+' '+str(up_z)+' )\n')
             
    
