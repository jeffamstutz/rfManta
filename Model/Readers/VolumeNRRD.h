/*
 *Author: Carson Brownlee (brownleeATcs.utah.edu)
 *Description: simple loader of Nrrd files into GridArray3
 *
*/

#ifndef VOLUMENRRD_H
#define VOLUMENRRD_H

#include <TeemConfig.h>
#include <Core/Containers/GridArray3.h>
#include <iostream>
#if HAVE_TEEM
#include <teem/nrrd.h>
#endif

namespace Manta
{
#if HAVE_TEEM
template<class T>
        GridArray3<T>* loadNRRDToGrid(std::string file, std::vector<std::pair<std::string, std::string> >* keyValuePairs = NULL )
{
    GridArray3<T>* grid = new GridArray3<T>();
    cout << "READING NRRD: " << file << "\n";
    Nrrd *new_nrrd = nrrdNew();
    ////////////////////////////////////////////////////////////////////////////
    // Attempt to open the nrrd
    if (nrrdLoad( new_nrrd, file.c_str(), 0 )) {
        char * reason = biffGetDone( NRRD );
        std::cout << "ERROR: Loading Nrrd Failed: " << reason << "\n";
        exit(__LINE__);
    }

    // Check to make sure the nrrd is the proper dimensions.
    if (new_nrrd->dim != 3) {
      std::cout << "\nERROR Nrrd must by a three dimension RGB.\n\n";
      exit(__LINE__);
    }
        
    // Check that the nrrd is the correct type.
    //  if (new_nrrd->type != nrrdTypeFloat) {
    //          std::cout << "ERROR Nrrd must be type nrrdTypeDouble" << std::endl;
    //          exit(__LINE__);
    //  }
        
    // Check that the nrrd could be a RGBA volume.
    //  if (new_nrrd->axis[0].size != 3) {
    //          std::cout << "ERROR Nrrd first axis must be RGB size 3." << std::endl;
    //          exit(__LINE__);
    //  }

    //Determine scaling for the volume.
    Vector size( new_nrrd->axis[0].size * new_nrrd->axis[0].spacing,
                     new_nrrd->axis[1].size * new_nrrd->axis[1].spacing,
                     new_nrrd->axis[2].size * new_nrrd->axis[2].spacing );
    if (keyValuePairs != NULL) {
      //keyValuePairs->clear();
      std::cout << "key value pairs: \n";
      int ki, nk;
      char *key, *val;
      nk = nrrdKeyValueSize(new_nrrd);
      for(ki=0; ki<nk; ki++) {
        nrrdKeyValueIndex(new_nrrd, &key, &val, ki);
        printf("%s = %s\n", key, val);
        keyValuePairs->push_back(std::pair<std::string, std::string>(key, val));
        free(key); free(val);
        key = NULL; val = NULL;
      }
    }
    

    size = Vector(new_nrrd->axis[0].size, new_nrrd->axis[1].size, new_nrrd->axis[2].size);
    std::cout << "size: " << size.x() << " " << size.y() << " " << size.z() << std::endl;
    // Rescale.
    float max_dim = max( size.x(), max( size.y(), size.z() ) );
    std::cout << "resizing\n";
    size /= max_dim;
        
    //_minBound = -size/2.0;
    //_maxBound = size/2.0;
    double min = FLT_MAX;
    double max = -FLT_MAX;
    std::cout << "resizing grid\n";      
    grid->resize(new_nrrd->axis[0].size, new_nrrd->axis[1].size, new_nrrd->axis[2].size);
    std::cout << "copying nrrd data\n";
    for(int i=0; i<grid->getNz(); i++)
        for(int j=0; j<grid->getNy(); j++)
            for(int k=0; k<grid->getNx(); k++)
    {
            //(*grid)(k,j,i) = 0.0;
            //(*grid)(k,j,i) = (*dataPtr)++;
            //void* vd = (void *) (&(idata[((i*ny + j)*nx + k) * formatSize]));
        if (new_nrrd->type == nrrdTypeFloat)
            (*grid)(k,j,i) = ((float*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
        else if (new_nrrd->type == nrrdTypeChar)
            (*grid)(k,j,i) = ((char*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
        else if (new_nrrd->type == nrrdTypeDouble)
            (*grid)(k,j,i) = ((double*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
        else if (new_nrrd->type == nrrdTypeUChar)
            (*grid)(k,j,i) = ((unsigned char*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
        else if (new_nrrd->type == nrrdTypeShort)
            (*grid)(k,j,i) = ((short*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
        else if (new_nrrd->type == nrrdTypeUShort)
            (*grid)(k,j,i) = ((unsigned short*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
        else if (new_nrrd->type == nrrdTypeInt)
            (*grid)(k,j,i) = ((int*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
        else if (new_nrrd->type == nrrdTypeUInt)
            (*grid)(k,j,i) = ((unsigned int*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
            //                          if (new_nrrd->type == nrrdTypeLong)
            //                                  (*grid)(k,j,i) = ((long*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
            //                          if (new_nrrd->type == nrrdTypeULong)
            //                                  (*grid)(k,j,i) = ((unsigned long*)new_nrrd->data)[(int)(((i*grid->getNy() + j)*grid->getNx() + k))];
        double val = (*grid)(k,j,i);
        if ( val < min)
          min = val;
        if (val > max)
          max = val;
    }
    std::cout << "min/max computed in volume: " << min << " " << max << std::endl;
    nrrdNuke(new_nrrd);
    std::cout << "done\n";
    return grid;
}
#else
template<class T>
        GridArray3<T>* loadNRRDToGrid(string file, std::vector<std::pair<std::string, std::string> >* keyValuePairs = NULL)
{
    std::cerr << "Error: " << __FILE__ << ": please link Manta with TEEM\n";
    return new GridArray3<T>();
}
#endif

}

#endif
