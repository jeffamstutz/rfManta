#ifndef Manta_Core_Color_Colormaps_LinearColormap_h
#define Manta_Core_Color_Colormaps_LinearColormap_h

#include <Core/Color/Color.h>
#include <Interface/Colormap.h>

namespace Manta{
  enum ColormapName {
    InvRainbowIso=0,
    InvRainbow,
    RainbowIso,
    Rainbow,
    InvGreyScale,
    InvBlackBody,
    BlackBody,
    GreyScale,
    Default
  };

  template<typename T>
  class LinearColormap : public Colormap<Color, T> {
  public:
    LinearColormap(float _min, float _max) : enableWrap(false){
      setRange(_min, _max);
    }

    void setRange(float newmin, float newmax){
      min = newmin;
      max = newmax;
      update();
    }

    void setMin(float newmin){
      setRange(newmin, max);
    }

    void setMax(float newmax){
      setRange(min, newmax);
    }

    void addColor(const Color& color){
      colors.push_back(color);
      update();
    }

    void setColors(const std::vector<Color>& colors_){
      colors = colors_;
      update();
    }

    float getMin() {
      return min;
    }

    float getMax() {
      return max;
    }

    // If set to false, colors are clamped outside of range.  True causes color
    // map to wrap so mapping repeats itself.
    void wrapColor(bool _enableWrap) {
      enableWrap = _enableWrap;
    }

    Color color(const T& val) const {
      // norm is a value in the range [0, N], where N is one less than the
      // number of colors.
      float norm = (val - min)*inv_range;
      if(norm > colors.size() - 1) {
        if (enableWrap)
          norm = fmod(norm, colors.size()-1);
        else
          norm = colors.size() - 1;
      }
      else if(norm < 0.0) {
        if (enableWrap)
          norm = fmod(-norm, colors.size()-1);
        else
          norm = 0.0;
      }

      unsigned idx = static_cast<int>(norm);
      float excess = norm - idx;

      // Correct color is <excess> amount between colors[idx] and colors[idx].

      // Special case for clamped-to-top of range value.
      if(idx == colors.size() - 2)
        return colors[idx];

      return colors[idx]*(1-excess) + colors[idx+1]*excess;
    }

  public:
    static LinearColormap *createColormap(ColormapName kind, float min, float max){
      LinearColormap *cmap = new LinearColormap(min, max);

      switch(kind){
      case InvRainbowIso:
        cmap->addColor(Color(RGB(0.528, 0.528, 1.0)));
        cmap->addColor(Color(RGB(0.304, 0.5824, 1.0)));
        cmap->addColor(Color(RGB(0.0, 0.6656, 0.832)));
        cmap->addColor(Color(RGB(0.0, 0.712, 0.5696)));
        cmap->addColor(Color(RGB(0.0, 0.744, 0.2976)));
        cmap->addColor(Color(RGB(0.0, 0.76, 0.0)));
        cmap->addColor(Color(RGB(0.304, 0.76, 0.0)));
        cmap->addColor(Color(RGB(0.5504, 0.688, 0.0)));
        cmap->addColor(Color(RGB(0.68, 0.624, 0.0)));
        cmap->addColor(Color(RGB(0.752, 0.6016, 0.0)));
        cmap->addColor(Color(RGB(1.0, 0.5008, 0.168)));
        cmap->addColor(Color(RGB(1.0, 0.424, 0.424)));
        break;

      case InvRainbow:
        cmap->addColor(Color(RGB(0, 0, 1)));
        cmap->addColor(Color(RGB(0, 0.40000001, 1)));
        cmap->addColor(Color(RGB(0, 0.80000001, 1)));
        cmap->addColor(Color(RGB(0, 1, 0.80000001)));
        cmap->addColor(Color(RGB(0, 1, 0.40000001)));
        cmap->addColor(Color(RGB(0, 1, 0)));
        cmap->addColor(Color(RGB(0.40000001, 1, 0)));
        cmap->addColor(Color(RGB(0.80000001, 1, 0)));
        cmap->addColor(Color(RGB(1, 0.91764706, 0)));
        cmap->addColor(Color(RGB(1, 0.80000001, 0)));
        cmap->addColor(Color(RGB(1, 0.40000001, 0)));
        cmap->addColor(Color(RGB(1, 0, 0)));
        break;

      case RainbowIso:
        cmap->addColor(Color(RGB(1.0, 0.424, 0.424)));
        cmap->addColor(Color(RGB(1.0, 0.5008, 0.168)));
        cmap->addColor(Color(RGB(0.752, 0.6016, 0.0)));
        cmap->addColor(Color(RGB(0.68, 0.624, 0.0)));
        cmap->addColor(Color(RGB(0.5504, 0.688, 0.0)));
        cmap->addColor(Color(RGB(0.304, 0.76, 0.0)));
        cmap->addColor(Color(RGB(0.0, 0.76, 0.0)));
        cmap->addColor(Color(RGB(0.0, 0.744, 0.2976)));
        cmap->addColor(Color(RGB(0.0, 0.712, 0.5696)));
        cmap->addColor(Color(RGB(0.0, 0.6656, 0.832)));
        cmap->addColor(Color(RGB(0.304, 0.5824, 1.0)));
        cmap->addColor(Color(RGB(0.528, 0.528, 1.0)));
        break;

      case Rainbow:
        cmap->addColor(Color(RGB(1, 0, 0)));
        cmap->addColor(Color(RGB(1, 0.40000001, 0)));
        cmap->addColor(Color(RGB(1, 0.80000001, 0)));
        cmap->addColor(Color(RGB(1, 0.91764706, 0)));
        cmap->addColor(Color(RGB(0.80000001, 1, 0)));
        cmap->addColor(Color(RGB(0.40000001, 1, 0)));
        cmap->addColor(Color(RGB(0, 1, 0)));
        cmap->addColor(Color(RGB(0, 1, 0.40000001)));
        cmap->addColor(Color(RGB(0, 1, 0.80000001)));
        cmap->addColor(Color(RGB(0, 0.80000001, 1)));
        cmap->addColor(Color(RGB(0, 0.40000001, 1)));
        cmap->addColor(Color(RGB(0, 0, 1)));
        break;

      case InvGreyScale:
        cmap->addColor(Color(RGB(1,1,1)));
        cmap->addColor(Color(RGB(0,0,0)));
        break;

      case InvBlackBody:
        cmap->addColor(Color(RGB(1, 1, 1)));
        cmap->addColor(Color(RGB(1, 1, 0.70588237)));
        cmap->addColor(Color(RGB(1, 0.96862745, 0.47058824)));
        cmap->addColor(Color(RGB(1, 0.89411765, 0.3137255)));
        cmap->addColor(Color(RGB(1, 0.80000001, 0.21568628)));
        cmap->addColor(Color(RGB(1, 0.63921571, 0.078431375)));
        cmap->addColor(Color(RGB(1, 0.47058824, 0)));
        cmap->addColor(Color(RGB(0.90196079, 0.27843139, 0)));
        cmap->addColor(Color(RGB(0.78431374, 0.16078432, 0)));
        cmap->addColor(Color(RGB(0.60000002, 0.070588239, 0)));
        cmap->addColor(Color(RGB(0.40000001, 0.0078431377, 0)));
        cmap->addColor(Color(RGB(0.20392157, 0, 0)));
        cmap->addColor(Color(RGB(0, 0, 0)));
        break;

      case BlackBody:
        cmap->addColor(Color(RGB(0, 0, 0)));
        cmap->addColor(Color(RGB(0.20392157, 0, 0)));
        cmap->addColor(Color(RGB(0.40000001, 0.0078431377, 0)));
        cmap->addColor(Color(RGB(0.60000002, 0.070588239, 0)));
        cmap->addColor(Color(RGB(0.78431374, 0.16078432, 0)));
        cmap->addColor(Color(RGB(0.90196079, 0.27843139, 0)));
        cmap->addColor(Color(RGB(1, 0.47058824, 0)));
        cmap->addColor(Color(RGB(1, 0.63921571, 0.078431375)));
        cmap->addColor(Color(RGB(1, 0.80000001, 0.21568628)));
        cmap->addColor(Color(RGB(1, 0.89411765, 0.3137255)));
        cmap->addColor(Color(RGB(1, 0.96862745, 0.47058824)));
        cmap->addColor(Color(RGB(1, 1, 0.70588237)));
        cmap->addColor(Color(RGB(1, 1, 1)));
        break;

      case GreyScale:
        cmap->addColor(Color(RGB(0,0,0)));
        cmap->addColor(Color(RGB(1,1,1)));
        break;

      case Default:
        cmap->addColor(Color(RGB(1,0,0)));
        cmap->addColor(Color(RGB(0,0,1)));
        break;
      }

      return cmap;
    }

  private:
    void update(){
      inv_range = (colors.size()-1) / (max - min);
    }

    float min, max, inv_range;
    std::vector<Color> colors;
    bool enableWrap;
  };
}

#endif
