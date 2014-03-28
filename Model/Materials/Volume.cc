#include <Model/Materials/Volume.h>

#include <algorithm>

using namespace std;

namespace Manta
{
                
  bool lessThan(const ColorSlice& left, const ColorSlice& right)
  {
    return left.value <= right.value;
  }

  RGBAColorMap::RGBAColorMap(unsigned int type, int numSlices)
    :oldTInc(0.01), _colorScaled(false)
  {
    fillColor(type);
    squish_scale = 1;
    squish_min = 0;
    computeHash();
  }
        
  RGBAColorMap::~RGBAColorMap()
  {}

  void RGBAColorMap::fillColor(unsigned int type)
  {
    // Add colors base on color map type
    switch (type) {
    case Rainbow:
      {
        float p = 1.0f/11.0f;
        _slices.push_back(ColorSlice(p*0, RGBAColor(Color(RGB(0, 0, 1)), 1)));
        _slices.push_back(ColorSlice(p*1, RGBAColor(Color(RGB(0, 0.40000001, 1)), 1)));
        _slices.push_back(ColorSlice(p*2, RGBAColor(Color(RGB(0, 0.80000001, 1)), 1)));
        _slices.push_back(ColorSlice(p*3, RGBAColor(Color(RGB(0, 1, 0.80000001)), 1)));
        _slices.push_back(ColorSlice(p*4, RGBAColor(Color(RGB(0, 1, 0.40000001)), 1)));
        _slices.push_back(ColorSlice(p*5, RGBAColor(Color(RGB(0, 1, 0)), 1)));
        _slices.push_back(ColorSlice(p*6, RGBAColor(Color(RGB(0.40000001, 1, 0)), 1)));
        _slices.push_back(ColorSlice(p*7, RGBAColor(Color(RGB(0.80000001, 1, 0)), 1)));
        _slices.push_back(ColorSlice(p*8, RGBAColor(Color(RGB(1, 0.91764706, 0)), 1)));
        _slices.push_back(ColorSlice(p*9, RGBAColor(Color(RGB(1, 0.80000001, 0)), 1)));
        _slices.push_back(ColorSlice(p*10, RGBAColor(Color(RGB(1, 0.40000001, 0)), 1)));
        _slices.push_back(ColorSlice(p*11, RGBAColor(Color(RGB(1, 0, 0)), 1)));
        break;
      }
    case InvRainbow:
      {
        float p = 1.0f/11.0f;
        _slices.push_back(ColorSlice(p*0, RGBAColor(Color(RGB(1, 0, 0)), 1)));
        _slices.push_back(ColorSlice(p*1, RGBAColor(Color(RGB(1, 0.40000001, 0)), 1)));
        _slices.push_back(ColorSlice(p*2, RGBAColor(Color(RGB(1, 0.80000001, 0)), 1)));
        _slices.push_back(ColorSlice(p*3, RGBAColor(Color(RGB(1, 0.91764706, 0)), 1)));
        _slices.push_back(ColorSlice(p*4, RGBAColor(Color(RGB(0.80000001, 1, 0)), 1)));
        _slices.push_back(ColorSlice(p*5, RGBAColor(Color(RGB(0.40000001, 1, 0)), 1)));
        _slices.push_back(ColorSlice(p*6, RGBAColor(Color(RGB(0, 1, 0)), 1)));
        
        _slices.push_back(ColorSlice(p*7, RGBAColor(Color(RGB(0, 1, 0.40000001)), 1)));
        _slices.push_back(ColorSlice(p*8, RGBAColor(Color(RGB(0, 1, 0.80000001)), 1)));
        _slices.push_back(ColorSlice(p*9, RGBAColor(Color(RGB(0, 0.80000001, 1)), 1)));
        _slices.push_back(ColorSlice(p*10, RGBAColor(Color(RGB(0, 0.40000001, 1)), 1)));
        _slices.push_back(ColorSlice(p*11, RGBAColor(Color(RGB(0, 0, 1)), 1)));
        break;
      }
    case InvBlackBody:
      {
        float p = 1.0f/12.0f;
        _slices.push_back(ColorSlice(p*0, RGBAColor(Color(RGB(1, 1, 1)), 1)));
        _slices.push_back(ColorSlice(p*1, RGBAColor(Color(RGB(1, 1, 0.70588237)), 1)));
        _slices.push_back(ColorSlice(p*2, RGBAColor(Color(RGB(1, 0.96862745, 0.47058824)), 1)));
        _slices.push_back(ColorSlice(p*3, RGBAColor(Color(RGB(1, 0.89411765, 0.3137255)), 1)));
        _slices.push_back(ColorSlice(p*4, RGBAColor(Color(RGB(1, 0.80000001, 0.21568628)), 1)));
        _slices.push_back(ColorSlice(p*5, RGBAColor(Color(RGB(1, 0.63921571, 0.078431375)), 1)));
        _slices.push_back(ColorSlice(p*6, RGBAColor(Color(RGB(1, 0.47058824, 0)), 1)));
        _slices.push_back(ColorSlice(p*7, RGBAColor(Color(RGB(0.90196079, 0.27843139, 0)), 1)));
        _slices.push_back(ColorSlice(p*8, RGBAColor(Color(RGB(0.78431374, 0.16078432, 0)), 1)));
        _slices.push_back(ColorSlice(p*9, RGBAColor(Color(RGB(0.60000002, 0.070588239, 0)), 1)));
        _slices.push_back(ColorSlice(p*10, RGBAColor(Color(RGB(0.40000001, 0.0078431377, 0)), 1)));
        _slices.push_back(ColorSlice(p*11, RGBAColor(Color(RGB(0.20392157, 0, 0)), 1)));
        _slices.push_back(ColorSlice(p*12, RGBAColor(Color(RGB(0, 0, 0)), 1)));
        break;
      }

    case BlackBody:
      {
        float p = 1.0f/12.0f;
        _slices.push_back(ColorSlice(p*0, RGBAColor(Color(RGB(0, 0, 0)), 1)));
        _slices.push_back(ColorSlice(p*1, RGBAColor(Color(RGB(0.20392157, 0, 0)), 1)));
        _slices.push_back(ColorSlice(p*2, RGBAColor(Color(RGB(0.40000001, 0.0078431377, 0)), 1)));
        _slices.push_back(ColorSlice(p*3, RGBAColor(Color(RGB(0.60000002, 0.070588239, 0)), 1)));
        _slices.push_back(ColorSlice(p*4, RGBAColor(Color(RGB(0.78431374, 0.16078432, 0)), 1)));
        _slices.push_back(ColorSlice(p*5, RGBAColor(Color(RGB(0.90196079, 0.27843139, 0)), 1)));
        _slices.push_back(ColorSlice(p*6, RGBAColor(Color(RGB(1, 0.47058824, 0)), 1)));
        _slices.push_back(ColorSlice(p*7, RGBAColor(Color(RGB(1, 0.63921571, 0.078431375)), 1)));
        _slices.push_back(ColorSlice(p*8, RGBAColor(Color(RGB(1, 0.80000001, 0.21568628)), 1)));
        _slices.push_back(ColorSlice(p*9, RGBAColor(Color(RGB(1, 0.89411765, 0.3137255)), 1)));
        _slices.push_back(ColorSlice(p*10, RGBAColor(Color(RGB(1, 0.96862745, 0.47058824)), 1)));
        _slices.push_back(ColorSlice(p*11, RGBAColor(Color(RGB(1, 1, 0.70588237)), 1)));
        _slices.push_back(ColorSlice(p*12, RGBAColor(Color(RGB(1, 1, 1)), 1)));
        break;
      }
    case GreyScale:
      _slices.push_back(ColorSlice(0, RGBAColor(Color(RGB(0,0,0)), 1)));
      _slices.push_back(ColorSlice(1,RGBAColor(Color(RGB(1,1,1)), 1)));
      break;
    case InvGreyScale:
      _slices.push_back(ColorSlice(0, RGBAColor(Color(RGB(1,1,1)), 1)));
      _slices.push_back(ColorSlice(1,RGBAColor(Color(RGB(0,0,0)), 1)));
      break;
    case InvRainbowIso:
      {
        float p = 1.0f/11.0f;
        _slices.push_back(ColorSlice(p*0, RGBAColor(Color(RGB(0.528, 0.528, 1.0)), 1)));
        _slices.push_back(ColorSlice(p*1, RGBAColor(Color(RGB(0.304, 0.5824, 1.0)), 1)));
        _slices.push_back(ColorSlice(p*2, RGBAColor(Color(RGB(0.0, 0.6656, 0.832)), 1)));
        _slices.push_back(ColorSlice(p*3, RGBAColor(Color(RGB(0.0, 0.712, 0.5696)), 1)));
        _slices.push_back(ColorSlice(p*4, RGBAColor(Color(RGB(0.0, 0.744, 0.2976)), 1)));
        _slices.push_back(ColorSlice(p*5, RGBAColor(Color(RGB(0.0, 0.76, 0.0)), 1)));
        _slices.push_back(ColorSlice(p*6, RGBAColor(Color(RGB(0.304, 0.76, 0.0)), 1)));
        _slices.push_back(ColorSlice(p*7, RGBAColor(Color(RGB(0.5504, 0.688, 0.0)), 1)));
        _slices.push_back(ColorSlice(p*8, RGBAColor(Color(RGB(0.68, 0.624, 0.0)), 1)));
        _slices.push_back(ColorSlice(p*9, RGBAColor(Color(RGB(0.752, 0.6016, 0.0)), 1)));
        _slices.push_back(ColorSlice(p*10, RGBAColor(Color(RGB(1.0, 0.5008, 0.168)), 1)));
        _slices.push_back(ColorSlice(p*11, RGBAColor(Color(RGB(1.0, 0.424, 0.424)), 1)));
        break;
      }
    default:
      cerr<<"RGBAColorMap::fillColor(type="<<type
          <<"):  Invalid type, using gray scale\n";
      fillColor(GreyScale);
    }
  }

  RGBAColorMap::RGBAColorMap(vector<Color>& colors, float* positions, float* opacities, int numSlices)
  {
    _colorScaled = false;
    _numSlices = numSlices;
    _min =0;
    _max = 1;
    _invRange = 0;
    oldTInc = 0.01f;
    for(size_t i = 0; i < colors.size(); i++)
    {
        if (opacities)
          _slices.push_back(ColorSlice(positions[i], RGBAColor(colors[i], opacities[i]) ));
        else
          _slices.push_back(ColorSlice(positions[i], RGBAColor(colors[i], 1.0f) ));
    }
    squish_min = 0;
    squish_scale = 1;
    std::sort(_slices.begin(), _slices.end(), lessThan);
    computeHash();
  }

  RGBAColorMap::RGBAColorMap(vector<ColorSlice>& colors, int numSlices)
  {
    _colorScaled = false;
    _numSlices = numSlices;
    _min =0;
    _max = 1;
    _invRange = 0;
    oldTInc = 0.01f;
    squish_min = 0;
    squish_scale = 1;
    _slices = colors;
    std::sort(_slices.begin(), _slices.end(), lessThan);
    computeHash();
  }

  RGBAColor RGBAColorMap::GetColor(float v)  // get the interpolated color at value v (0-1)
  {
    if (_slices.size() == 0)
      return RGBAColor(Color(RGB(0,0,0)), 1.0f);
    int start = 0;
    int end = _slices.size()-1;
        
    if (v < _slices[0].value*squish_scale + squish_min)
      return _slices[0].color;
    if (v > _slices[end].value*squish_scale + squish_min)
      return _slices[end].color;
    //binary search over values
    int middle;
    while(end > start+1)
      {
        middle = (start+end)/2;
        if (v < _slices[middle].value*squish_scale + squish_min)
          end = middle;
        else if (v > _slices[middle].value*squish_scale + squish_min)
          start = middle;
        else
          return _slices[middle].color;
      }
    float interp = (v - (_slices[start].value*squish_scale + squish_min)) / ((_slices[end].value - _slices[start].value)*squish_scale);
    return RGBAColor( Color(_slices[start].color.color*(1.0f-interp) + _slices[end].color.color*interp), 
                      _slices[start].color.a*(1.0f-interp) + _slices[end].color.a*interp);
        
  }

  RGBAColor RGBAColorMap::GetColorAbs(float v)
  {
    return GetColor((v - _min)/_invRange);
  }

  void RGBAColorMap::SetColors(vector<ColorSlice>& colors)
  {
    _slices = colors;
    std::sort(_slices.begin(), _slices.end(), lessThan);
    if (_colorScaled)
      {
        oldTInc = 0.01;
        scaleAlphas(tInc);
      }
  }

  void RGBAColorMap::SetMinMax(float min, float max)
  {
    _min = min; 
    _max = max;
    _invRange = 1.0/(max - min);
  }



  void RGBAColorMap::computeHash()
  {

    unsigned long long new_course_hash = 0;
    int nindex = 63; // 64 - 1

    for (int a_index = 0; a_index < int(_slices.size()-1); a_index++) {
      // This code looks for segments where either the start or the end
      // is non zeoro.  When this happens indices are produces which
      // round down on the start and round up on the end.  This can
      // cause some overlap at adjacent non zero segments, but this is
      // OK as we are only turning bits on.
      float val = _slices[a_index].color.a;
      float next_val = _slices[a_index+1].color.a;
      if (val != 0 || next_val != 0) {
        int start, end;
        start = (int)((_slices[a_index].value * squish_scale + squish_min)*nindex);
        end = (int)ceilf((_slices[a_index+1].value * squish_scale +squish_min)*nindex);
        for (int i = start; i <= end; i++)
          // Turn on the bits.
          new_course_hash |= 1ULL << i;
      }
    }
    course_hash = new_course_hash;
    //size_t num_bits = sizeof(course_hash)*8;
    //for(size_t i = 0; i < num_bits; i++)
        //if (course_hash & ( 1ULL << i ))
       //cout << 1;
      //else
       // cout << 0;

  }

  void RGBAColorMap::squish(float globalMin, float globalMax, float squishMin, float squishMax)
  {
    squishMin = max(globalMin, squishMin);
    squishMax = min(globalMax, squishMax);
    squish_scale = (squishMax - squishMin)/(globalMax - globalMin);
    squish_min = (squishMin - globalMin)/(globalMax-globalMin);
    computeHash();
  }


  //scale alpha values by t value
  // Preconditions:
  //   assuming that the data in alpha_transform and alpha_stripes is already
  //     allocated.
  //   alpha_list.size() >= 2
  //   alpha_list[0].x == 0
  //   alpha_list[alpha_list.size()-1].x == 1

  void RGBAColorMap::scaleAlphas(float t) {
    _colorScaled = true;
    // the ratio of values as explained in rescale_alphas
    float d2_div_d1 = t/oldTInc;

    for (int a_index = 0; a_index < int(_slices.size()); a_index++) {
      float val = _slices[a_index].color.a;
      float one_minus_val = 1.0f - val;
      if (one_minus_val >= 1e-6f)
        _slices[a_index].color.a = 1 - powf(one_minus_val, d2_div_d1);
      else
        _slices[a_index].color.a = 1;
    }
    oldTInc = tInc;
    tInc = t;
  }

} //namespace manta

