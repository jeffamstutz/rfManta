#ifndef MANTA_CORE_UTIL_MEMORY_POOL_H_
#define MANTA_CORE_UTIL_MEMORY_POOL_H_

#include <list>
#include <vector>

namespace Manta {

  // A MemoryPool for a single StorageType.  TODO(boulos): Extend this
  // class to be a more generic MemoryPool that allows arbitrary
  // requests, uses a set of free lists instead of one, etc.

  template<class StorageType>
  class MemoryPool {
  private:
    // A Block is a chunk of memory along with its own capacity and
    // size (because lists don't have a constant time size operation).
    // TODO(boulos): add free list?
    struct Block {
      StorageType* data;
      size_t capacity;
      size_t size;
    };
  public:
    MemoryPool(size_t new_size = 8) : capacity(0), size(0) {
      reserve(new_size);
    }

    void clear() {
      // Consider coalescing all the blocks?
      for (size_t i = 0; i < blocks.size(); i++) {
        blocks[i].size = 0;
      }
    }

    void reserve(size_t new_size) {
      if (new_size < capacity)
        return;
      size_t new_block_size = new_size - capacity;

      Block new_block;
      new_block.size = 0;
      new_block.capacity = new_block_size;
      new_block.data = new StorageType[new_block_size];
      blocks.push_back(new_block);

      capacity = new_size;
    }

    StorageType* getItem() {
      // If we're out of space, ask for more
      if (size == capacity) {
        reserve(2 * size);
      }

      for (size_t i = 0; i < blocks.size(); i++) {
        // NOTE(boulos): The SGI docs imply that .empty() is constant
        // time while .size() is linear (which is reasonable)
        Block& block = blocks[i];
        if (block.size == block.capacity) {
          continue;
        }

        StorageType* result = &(block.data[block.size]);
        if (result == 0) {
          throw InternalError("Apparently we're holding a NULL pointer in our free list.");
        }
        block.size++;
        size++;
        return result;
      }

      // NOTE(boulos): This shouldn't happen, but not having it will
      // generate compiler warnings.
      throw InternalError("Unable to allocate a new item");
      return 0;
    }

    // TODO(boulos): Add the ability to return memory one-by-one.
  private:
    std::vector<Block> blocks;
    // Maintain an aggregate capacity and size so we don't need to
    // search when we need to allocate a new block.  TODO(boulos): Add
    // a first free block list to avoid searching through full blocks.
    size_t capacity;
    size_t size;
  };
};

#endif // MANTA_CORE_UTIL_MEMORY_POOL_H_
