
#include <Core/Util/RayTree.h>

#include <cstdio>
#include <cstdlib>

#define MAGIC 1234
#define byteswap(x) swapbytes(reinterpret_cast<unsigned char*>(&x), sizeof(x))

using namespace Manta;

// TODO:  Prefer not to use unsigned int for num_{trees,children}, but size_t
//        causes issues between 32- and 64-bit systems

inline void swapbytes(unsigned char* b, int n)
{
  register unsigned int i=0;
  register unsigned int j=n - 1;
  while (i<j) {
    std::swap(b[i], b[j]);
    ++i;
    --j;
  }
}

RayForest::~RayForest()
{
  // printf("RayForest::~RayForest - deleting trees\n");
  for (size_t i = 0; i < trees.size(); i++ )
    delete trees[i];
}

void RayForest::addChild(RayTree* t)
{
  trees.push_back(t);
}

bool RayForest::writeToFile(FILE* output) const
{
  // printf("RayForest::writeToFile() -- %d Children\n", int(trees.size()));
  // write out the number of trees then
  // call writeToFile recursively
  unsigned int magic = MAGIC;
  size_t result = fwrite((const void*)&magic, sizeof(unsigned int), (size_t)1, output);
  if (result != 1)
    return false;

  unsigned int num_trees = trees.size();
  result = fwrite((const void*)&num_trees, sizeof(unsigned int), (size_t)1, output);
  if (result != 1)
    return false;

  for (size_t i = 0; i < num_trees; i++ )
    {
      // printf("RayForest::writeToFile() -- Writing Tree #%d\n", int(i));
      if (!trees[i]->writeToFile(output))
        return false;
    }
  return true;
}

bool RayForest::readFromFile(FILE* input)
{
  bool swapit = false;
  unsigned int magic;
  size_t result = fread((void*)&magic, sizeof(int), (size_t)1, input);
  if (result != 1)
    return false;
  else if (magic != MAGIC) {
    swapit = true;
    // printf("RayForest::readFromFile() -- swapping bytes\n");
    byteswap(magic);
    if (magic != MAGIC) {
      // printf("RayForest::readFromFile() -- swapping bytes didn't work!\n");
      return false;
    }
  }

  // printf("RayForest::readFromFile() -- magic = %d, swapit = %s\n", magic, (swapit?"true":"false"));

  unsigned int num_trees = 0;
  result = fread((void*)&num_trees, sizeof(unsigned int), (size_t)1, input);
  if (result != 1)
    return false;
  else if (swapit)
    byteswap(num_trees);

  trees.resize(num_trees);
  for (size_t i = 0; i < num_trees; i++ )
    {
      trees[i] = new RayTree;
      if (!trees[i]->readFromFile(input, swapit))
        return false;
    }
  return true;
}

RayTree::~RayTree()
{
  // printf("RayTree::~RayTree - deleting children\n");
  for (size_t i = 0; i < children.size(); i++ )
    delete children[i];
}

bool RayTree::writeToFile(FILE* output) const
{
  // printf("RayTree::writeToFile() -- %d Children\n", int(children.size()));
  node.writeToFile(output);
  // write out the number of children then
  // call writeToFile recursively
  unsigned int num_children = children.size();
  size_t result = fwrite((const void*)&num_children, sizeof(unsigned int), (size_t)1, output);
  if (result != 1)
    {
      fprintf(stderr, "Failed to write number of children\n");
      return false;
    }

  for (size_t i = 0; i < num_children; i++)
    {
      // printf("RayTree::writeToFile() -- Writing Child %d\n", int(i));
      if (!children[i]->writeToFile(output))
        return false;
    }

  return true;
}

bool RayTree::readFromFile(FILE* input, bool swapit)
{
  node.readFromFile(input, swapit);

  unsigned int num_children = 0;
  size_t result = fread((void*)&num_children, sizeof(unsigned int), (size_t)1, input);
  if (result != 1) {
    fprintf(stderr, "Failed to read number of children\n");
    return false;
  } else if (swapit) {
    byteswap(result);
  }

  children.resize(num_children);
  for (size_t i = 0; i < num_children; i++ )
    {
      children[i] = new RayTree;
      if (!children[i]->readFromFile(input, swapit))
        return false;
    }
  return true;
}

void RayTree::addChild(RayTree* t)
{
  children.push_back(t);
}

bool RayInfo::writeToFile(FILE* output) const
{
  // printf("RayInfo::writeToFile() -- Writing 4 Byte Values\n");
  size_t result = fwrite((const void*)origin, (size_t)4, (size_t)RayInfo::Num4Byte, output);
  if (result != Num4Byte)
    return false;

  // printf("RayInfo::writeToFile() -- Writing %d Byte Values\n", int(sizeof(long long int)));
  result = fwrite((const void*)&ray_id, (size_t)8, (size_t)RayInfo::Num8Byte, output);
  if (result != Num8Byte)
    return false;

  return true;
}

bool RayInfo::readFromFile(FILE* input, bool swapit)
{
  size_t result = fread((void*)origin, (size_t)4, (size_t)RayInfo::Num4Byte, input);
  if (result != Num4Byte)
    return false;
  else if (swapit) {
    int* buffer = reinterpret_cast<int*>(origin);
    for (int i = 0; i < RayInfo::Num4Byte; ++i)
      byteswap(buffer[i]);
  }

  result = fread((void*)&ray_id, sizeof(long long int), (size_t)RayInfo::Num8Byte, input);
  if (result != Num8Byte)
    return false;
  else if (swapit) {
    long long int* buffer = reinterpret_cast<long long int*>(&ray_id);
    for (int i = 0; i < RayInfo::Num8Byte; ++i)
      byteswap(buffer[i]);
  }

  return true;
}
