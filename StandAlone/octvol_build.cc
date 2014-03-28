// This program allows us to build octree volumes from a standalone program, without
// having to use the full Manta footprint.
//
// Author: Aaron Knoll
// Date: June 3, 2006

#include <Model/Primitives/OctreeVolume.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Util/Args.h>
#include <iostream>
#include <Core/Thread/Time.h>

using namespace Manta;
using namespace std;

static void printUsage()
{
    cerr << "octvol_build usage:" << endl;
    cerr << "  -buildfrom <filename> " << endl;
    cerr << "options:" << endl;
    cerr << "  -tstart <tstart>" << endl;
    cerr << "  -tend <tend>" << endl;
    cerr << "  -variance <variance>" << endl;
    cerr << "  -kernel_width <kernel_width>" << endl;
    cerr << "  -mres_levels <mres_levels>" << endl;
    cerr << "  -isomin <isomin>" << endl;
    cerr << "  -isomax <isomax>" << endl;
}

int main(int argc, char* argv[]) {

    // Copy args into a vector<string>
    vector<string> args;
    for(int i=1;i<argc;i++)
        args.push_back(argv[i]);

    if (argc == 0)
    {
        printUsage();
        return 0;
    }


    string buildfrom_filename = "";
    int tstart = 0;
    int tend = 0;
    double variance = 0.1;
    int kernel_width = 2;
    int mres_levels = 0;
    double isomin = 0;
    double isomax = 255;

    // Parse args.i
    for (size_t i=0;i<args.size();++i)
    {
        if (args[i] == "-buildfrom") {
            if (!getStringArg(i, args, buildfrom_filename))
                throw IllegalArgument("octisovol -buildfrom <filename>", i, args);
        }
        else if (args[i] == "-tstart") {
            if (!getIntArg(i, args, tstart))
                throw IllegalArgument("octisovol -tstart <tstart>", i, args);
        }
        else if (args[i] == "-tend") {
            if (!getIntArg(i, args, tend))
                throw IllegalArgument("octisovol -tend <tend>", i, args);
        }
        else if (args[i] == "-variance") {
            if (!getDoubleArg(i, args, variance))
                throw IllegalArgument("octisovol -variance <variance>", i, args);
        }
        else if (args[i] == "-kernel_width") {
            if (!getIntArg(i, args, kernel_width))
                throw IllegalArgument("octisovol -kernel_width <kernel_width>", i, args);
        }
        else if (args[i] == "-mres_levels") {
            if (!getIntArg(i, args, mres_levels))
                throw IllegalArgument("octisovol -mres_levels <mres_levels>", i, args);
        }
        else if (args[i] == "-isomin") {
            if (!getDoubleArg(i, args, isomin))
                throw IllegalArgument("octisovol -isomin <isomin>", i, args);
        }
        else if (args[i] == "-isomax") {
            if (!getDoubleArg(i, args, isomax))
                throw IllegalArgument("octisovol -isomax <isomax>", i, args);
        }
        else {
            printUsage();
            return 0;
        }
    }


    if (buildfrom_filename == "")
    {
        printUsage();
        return 0;
    }

    OctreeVolume ov((char*)buildfrom_filename.c_str(), tstart, tend, variance, kernel_width, (Manta::ST)isomin, (Manta::ST)isomax);

    return 0;
}


