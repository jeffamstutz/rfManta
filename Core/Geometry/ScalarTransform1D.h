
#ifndef Manta_Core_ScalarTransform1D_h
#define Manta_Core_ScalarTransform1D_h

#include <MantaTypes.h>
#include <Core/Containers/Array1.h>
#include <Core/Math/MiscMath.h>

namespace Manta
{
  // LType is the lookup type - it should be a scalar
  // RType is the result type - can be any type
  template<class LType, class RType>
  class ScalarTransform1D
  {
  public:
    ScalarTransform1D(Array1<RType>* results=0);
    ScalarTransform1D(const Array1<RType> &r);
    ~ScalarTransform1D(void);

    inline int getLookupIndex(const LType value) const {
      return static_cast<int>((value - min)*inv_range*(results->size() - 1));
    }

    inline RType lookup(const LType value) const {
      return (*results)[getLookupIndex(value)];
    }

    inline int getBoundLookupIndex(const LType value) const {
      int idx=getLookupIndex(value);
      return Clamp(idx, 0, results->size() - 1);
    }

    inline RType lookupBound(const LType value) const {
      return (*results)[getBoundLookupIndex(value)];
    }

    inline int getLookupIndex(const LType value, const LType min_,
                              const LType max_) const {
      int idx=static_cast<int>((value - min_)/(max_ - min_)*results->size() - 1);
      return Clamp(idx, 0, results->size() - 1);
    }

    inline RType lookup(const LType value, const LType min_,
                        const LType max_) const {
      return (*results)[get_lookup_index(value,min_,max_)];
    }

    inline RType lookupWithIndex(const int idx) const {
      return (*results)[Clamp(idx, 0, results->size() - 1)];
    }

    inline RType& operator[](const int idx) const {
      return (*results)[idx];
    }

    inline RType interpolate(const LType value) const {
      double step=(value - min)*inv_range*(results->size() - 1);
      int x_low=Clamp((int)step, 0, results->size() - 2);
      double x_weight_low=(x_low + 1) - step;
      return (*results)[x_low]*x_weight_low +
        (*results)[x_low + 1]*(1 - x_weight_low);
    }
  
    Array1<RType> *getResultsPtr(void) {
      return results;
    }

    Array1<RType> &getResultsRef(void) {
      return *results;
    }

    // This function could cause problems in parallel, so watch out!
    void setResultsPtr(Array1<RType> *results_) {
      if (nuke)
        delete results;

      nuke=false;
      results=results_;
    }

    // If you want the results pointer to not be deleted (in the case
    // where it was created by ScalarTransform1D you need to call this
    // function.  This will prevent the class from deleting the memory.
    void dontNuke(void) {
      nuke=false;
    }
  
    void getMinMax(LType& min_, LType& max_) const;
    void scale(LType min_, LType max_);
    bool isScaled() const { return is_scaled; } 

    inline int size() const { return results->size(); }

  private:
    Array1<RType>* results;
    LType min, max;
    Real inv_range;
    bool is_scaled;
    bool nuke;
  };

  template<class LType, class RType>
  ScalarTransform1D<LType,RType>::ScalarTransform1D(Array1<RType> *results) :
    results(results), min(0), max(1), inv_range(1), is_scaled(false),
    nuke(false)
  {
    // Do nothing
  }

  template<class LType, class RType>
  ScalarTransform1D<LType,RType>::ScalarTransform1D(const Array1<RType> &r):
    min(0), max(1), inv_range(1), is_scaled(false), nuke(true)
  {
    results=new Array1<RType>(r);
  }

  template<class LType, class RType>
  ScalarTransform1D<LType,RType>::~ScalarTransform1D()
  {
    if (nuke)
      if (results != 0)
        delete results;
  }

  template<class LType, class RType>
  void ScalarTransform1D<LType,RType>::getMinMax(LType &min_, LType &max_) const
  {
    min_=min;
    max_=max;
  }
  
  template<class LType, class RType>
  void ScalarTransform1D<LType,RType>::scale(LType min_, LType max_)
  {
    min=min_;
    max=max_;

    // Make sure that min and max are in the right order
    if (min > max) {
      LType t=min;
      min=max;
      max=t;
    }

    // Set inv_range to 0 if min and max equal each other
    if (min == max)
      inv_range=0;
    else
      inv_range=1/static_cast<Real>(max - min);

    is_scaled=true;
  }
}

#endif // Manta_Core_ScalarTransform1D_h
