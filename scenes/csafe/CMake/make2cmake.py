#!/usr/bin/python

import os,sys,string


if __name__ == "__main__":
    # Check the argument list size
    if (len(sys.argv) < 3):
        sys.stderr.write("USAGE: " + sys.argv[0] + " <input file> <output file>\n")
        sys.exit(1)

    infilename = sys.argv[1]
    outfilename = sys.argv[2]

    # U is the mode to open the file with universal newline support.
    infile = open(infilename, "rU")
    outfile = open(outfilename, "w")

    # Now read the lines
    lines = map( lambda x: string.strip(x, string.whitespace+"\\"),
                 infile.readlines() )
    infile.close()
    
    # Now write them
    outfile.write("SET( MANTA_SWIG_DEPEND\n")
    for l in lines[1:]:
        outfile.write(l + "\n")
    outfile.write(")\n")
    outfile.close()
    
