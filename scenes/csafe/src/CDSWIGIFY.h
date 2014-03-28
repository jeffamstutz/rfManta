//! Filename: CDSWIGIFY.h
/*!
 these functions are purely to fix weird issues with communicating between c++
 and python using swig.
 */

#include <Interface/MantaInterface.h>
#include <Core/Color/Color.h>
using namespace Manta;

int* SWIGIFYCreateIntArray(int size)
{
	return new int[size];
}

void SWIGIFYDestroyIntArray(int* array)
{
  delete[] array;
}

double SWIGIFYGetIntArrayValue(int* array, int i) { return array[i]; }

double* SWIGIFYCreateDouble(double val) { return new double(val);}
float* SWIGIFYCreateFloat(float val) { return new float(val); }

double SWIGIFYGetDouble(double* val) { return *val; }
float SWIGIFYGetFloat(float* val) { return *val; }
int SWIGIFYGetInt(int* val) { return *val; }
int SWIGIFYGetNumWorkers(MantaInterface* i) { return i->numWorkers(); }

float SWIGIFYGetColorValue(Color c, int i) { return c[i]; }
