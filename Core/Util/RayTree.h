
#ifndef Manta_Core_RayTree_h
#define Manta_Core_RayTree_h

#include <cstdio>
#include <vector>

namespace Manta {
  class RayInfo {
  public:
    RayInfo(){}

    enum RayType {
      PrimaryRay = 0,
      ShadowRay,
      ReflectionRay,
      RefractionRay,
      DiffuseRay,
      UnknownRay
    };

    // 4 byte entries
    float origin[3];    // ray origin
    float direction[3]; // ray direction
    float time; // motion blur time [0,1)
    float HitParameter; // ray t_value at hitpoint
    int object_id; // object you hit, -1 for background
    int material_id; // material of object, -1 for background
    float s,t; // surface parameterization
    // normal? ONB?
    int depth; // bounce depth for reconstructing trees
    RayType type;

    // 8 byte entries
    long long int ray_id; // unique ray identifier
    long long int parent_id; // -1 for root

    static const int Num4Byte = 14;
    static const int Num8Byte =  2;

    bool writeToFile(FILE* output) const;
    bool readFromFile(FILE* input, bool swapit);

    void setOrigin(float x, float y, float z)
    {
      origin[0] = x;
      origin[1] = y;
      origin[2] = z;
    }

    void setDirection(float x, float y, float z)
    {
      direction[0] = x;
      direction[1] = y;
      direction[2] = z;
    }

    void setTime(float t)
    {
      time = t;
    }

    void setHit(float t)
    {
      HitParameter = t;
    }

    void setObjectID(int i)
    {
      object_id = i;
    }

    void setMaterialID(int i)
    {
      material_id = i;
    }

    void setSurfaceParams(float u, float v)
    {
      s = u;
      t = v;
    }

    void setDepth(int d)
    {
      depth = d;
    }

    void setType(RayType t)
    {
      type = t;
    }

    void setRayID(long long int id)
    {
      ray_id = id;
    }

    void setParentID(long long int id)
    {
      parent_id = id;
    }
  };

  class RayTree {
  public:
    RayTree() {}
    RayTree(const RayInfo& r) : node(r) {}
    ~RayTree();

    void addChild(RayTree* t);
    bool writeToFile(FILE* output) const;
    bool readFromFile(FILE* input, bool swapit);

    RayInfo node;
    std::vector<RayTree*> children;
  };

  // a forest is a collection of Trees ;)
  class RayForest {
  public:
    RayForest() {}
    ~RayForest();

    void addChild(RayTree* t);
    bool writeToFile(FILE* output=0) const;
    bool readFromFile(FILE* input=0);

    std::vector<RayTree*> trees;
  };
}

#endif // Manta_Core_RayTree_h
