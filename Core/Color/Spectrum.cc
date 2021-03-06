
#include <Core/Color/Spectrum.h>
#include <Core/Exceptions/SerializationError.h>
#include <Core/Math/MinMax.h>

#include <sstream>
#include <string>
#include <iomanip>
#include <ctype.h>

using namespace Manta;
using namespace std;

typedef float MatchSample[4];

// Stiles & Burch (1955) 2-deg color matching functions
static MatchSample match[] = {
  {390,  1.83970e-003, -4.53930e-004,  1.21520e-002},
  {395,  4.61530e-003, -1.04640e-003,  3.11100e-002},
  {400,  9.62640e-003, -2.16890e-003,  6.23710e-002},
  {405,  1.89790e-002, -4.43040e-003,  1.31610e-001},
  {410,  3.08030e-002, -7.20480e-003,  2.27500e-001},
  {415,  4.24590e-002, -1.25790e-002,  3.58970e-001},
  {420,  5.16620e-002, -1.66510e-002,  5.23960e-001},
  {425,  5.28370e-002, -2.12400e-002,  6.85860e-001},
  {430,  4.42870e-002, -1.99360e-002,  7.96040e-001},
  {435,  3.22200e-002, -1.60970e-002,  8.94590e-001},
  {440,  1.47630e-002, -7.34570e-003,  9.63950e-001},
  {445, -2.33920e-003,  1.36900e-003,  9.98140e-001},
  {450, -2.91300e-002,  1.96100e-002,  9.18750e-001},
  {455, -6.06770e-002,  4.34640e-002,  8.24870e-001},
  {460, -9.62240e-002,  7.09540e-002,  7.85540e-001},
  {465, -1.37590e-001,  1.10220e-001,  6.67230e-001},
  {470, -1.74860e-001,  1.50880e-001,  6.10980e-001},
  {475, -2.12600e-001,  1.97940e-001,  4.88290e-001},
  {480, -2.37800e-001,  2.40420e-001,  3.61950e-001},
  {485, -2.56740e-001,  2.79930e-001,  2.66340e-001},
  {490, -2.77270e-001,  3.33530e-001,  1.95930e-001},
  {495, -2.91250e-001,  4.05210e-001,  1.47300e-001},
  {500, -2.95000e-001,  4.90600e-001,  1.07490e-001},
  {505, -2.97060e-001,  5.96730e-001,  7.67140e-002},
  {510, -2.67590e-001,  7.01840e-001,  5.02480e-002},
  {515, -2.17250e-001,  8.08520e-001,  2.87810e-002},
  {520, -1.47680e-001,  9.10760e-001,  1.33090e-002},
  {525, -3.51840e-002,  9.84820e-001,  2.11700e-003},
  {530,  1.06140e-001,  1.03390e+000, -4.15740e-003},
  {535,  2.59810e-001,  1.05380e+000, -8.30320e-003},
  {540,  4.19760e-001,  1.05120e+000, -1.21910e-002},
  {545,  5.92590e-001,  1.04980e+000, -1.40390e-002},
  {550,  7.90040e-001,  1.03680e+000, -1.46810e-002},
  {555,  1.00780e+000,  9.98260e-001, -1.49470e-002},
  {560,  1.22830e+000,  9.37830e-001, -1.46130e-002},
  {565,  1.47270e+000,  8.80390e-001, -1.37820e-002},
  {570,  1.74760e+000,  8.28350e-001, -1.26500e-002},
  {575,  2.02140e+000,  7.46860e-001, -1.13560e-002},
  {580,  2.27240e+000,  6.49300e-001, -9.93170e-003},
  {585,  2.48960e+000,  5.63170e-001, -8.41480e-003},
  {590,  2.67250e+000,  4.76750e-001, -7.02100e-003},
  {595,  2.80930e+000,  3.84840e-001, -5.74370e-003},
  {600,  2.87170e+000,  3.00690e-001, -4.27430e-003},
  {605,  2.85250e+000,  2.28530e-001, -2.91320e-003},
  {610,  2.76010e+000,  1.65750e-001, -2.26930e-003},
  {615,  2.59890e+000,  1.13730e-001, -1.99660e-003},
  {620,  2.37430e+000,  7.46820e-002, -1.50690e-003},
  {625,  2.10540e+000,  4.65040e-002, -9.38220e-004},
  {630,  1.81450e+000,  2.63330e-002, -5.53160e-004},
  {635,  1.52470e+000,  1.27240e-002, -3.16680e-004},
  {640,  1.25430e+000,  4.50330e-003, -1.43190e-004},
  {645,  1.00760e+000,  9.66110e-005, -4.08310e-006},
  {650,  7.86420e-001, -1.96450e-003,  1.10810e-004},
  {655,  5.96590e-001, -2.63270e-003,  1.91750e-004},
  {660,  4.43200e-001, -2.62620e-003,  2.26560e-004},
  {665,  3.24100e-001, -2.30270e-003,  2.15200e-004},
  {670,  2.34550e-001, -1.87000e-003,  1.63610e-004},
  {675,  1.68840e-001, -1.44240e-003,  9.71640e-005},
  {680,  1.20860e-001, -1.07550e-003,  5.10330e-005},
  {685,  8.58110e-002, -7.90040e-004,  3.52710e-005},
  {690,  6.02600e-002, -5.67650e-004,  3.12110e-005},
  {695,  4.14800e-002, -3.92740e-004,  2.45080e-005},
  {700,  2.81140e-002, -2.62310e-004,  1.65210e-005},
  {705,  1.91170e-002, -1.75120e-004,  1.11240e-005},
  {710,  1.33050e-002, -1.21400e-004,  8.69650e-006},
  {715,  9.40920e-003, -8.57600e-005,  7.43510e-006},
  {720,  6.51770e-003, -5.76770e-005,  6.10570e-006},
  {725,  4.53770e-003, -3.90030e-005,  5.02770e-006},
  {730,  3.17420e-003, -2.65110e-005,  4.12510e-006}
};

std::string Spectrum::toString() const
{
  return writePersistentString();
}

float sample_scale[] = {1./206.327774, 1./94.4312286, 1./55.3023796};

RGBColor Spectrum::getRGB() const
{
  // Integrate against the color matching curves
  int nmatch = sizeof(match)/sizeof(MatchSample);
  
  int imatch = 0;
  while(imatch+1 < nmatch && match[imatch+1][0] < samples[0])
    imatch++;

  int nsamples = samples.size();
  int isample = 0;
  while(isample+2 < nsamples && samples[isample+2] < match[imatch][0])
    isample += 2;

  float sum[3];
  for(int i=0;i<3;i++)
    sum[i] = 0;

  float w0 = Min(match[imatch][0], samples[isample]);
  imatch++; isample += 2;
  for(;;){
    float w1 = Min(match[imatch][0], samples[isample]);

    float ma = (samples[isample+1] - samples[isample-1]) / (samples[isample] - samples[isample-2]);
    float ba = samples[isample-1] - ma * samples[isample-2];
    for(int j=0;j<3;j++){
      float mb = (match[imatch][j+1] - match[imatch-1][j+1]) / (match[imatch][0] - match[imatch-1][0]);
      float bb = match[imatch-1][j+1] - mb * match[imatch-1][0];
      float integral = ma*mb           * (w1*w1*w1 - w0*w0*w0) * (1./3) + 
                       (ba*mb + ma*bb) * (w1*w1 - w0*w0) * (1./2.) + 
                       ba*bb           * (w1 - w0);
      sum[j] += integral;
    }

    if(match[imatch][0] == w1){
      imatch++;
      if(imatch >= nmatch)
        break;
    }
    if(samples[isample] == w1){
      isample += 2;
      if(isample >= nsamples)
        break;
    }
    w0 = w1;
  }
  for(int j=0;j<3;j++)
    sum[j] *= sample_scale[j];
  return RGBColor(sum[0], sum[1], sum[2]);
}

std::string Spectrum::writePersistentString() const
{
  ostringstream str;
  str << "spectrum:";
  for(vector<float>::const_iterator iter = samples.begin(); iter != samples.end(); iter++)
    str << ' ' << *iter;
  return str.str();
}

bool Spectrum::readPersistentString(const std::string& str)
{
  if(str.substr(0, 9) == "spectrum:"){
    // A spectrum
    std::istringstream in(str.substr(9));
    float f;
    while(in >> f)
      samples.push_back(f);
    if(samples.size() % 2 != 0)
      throw SerializationError("Illegal spectrum specification");
    if(samples.size() < 4)
      throw SerializationError("Illegal spectrum specification: too few samples");
    float oldlambda = 0;
    for(unsigned int i = 0; i < samples.size(); i+=2) {
      float lambda = samples[i];
      if(lambda < oldlambda)
        throw SerializationError("Spectrum wavelengths must increase monotonically");
      oldlambda = lambda;
    }
    return true;
  } else {
    return false;
  }
}
