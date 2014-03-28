#ifndef _MANTA_CORE_UTIL_UPDATE_GRAPH_H_
#define _MANTA_CORE_UTIL_UPDATE_GRAPH_H_


#include <map>
#include <set>
#include <vector>
#include <Core/Exceptions/InternalError.h>
#include <Core/Thread/AtomicCounter.h>
#include <Core/Thread/Mutex.h>
#include <iostream>
#include <string>

namespace Manta {

  template<class KeyType>
  class UpdateGraphNode {
  public:
    UpdateGraphNode(KeyType key, UpdateGraphNode* parent) :
      key(key),
      parent(parent),
      needs_update(false),
      counter("UpdateGraph Counter", 0),
      active_edges(0) {
    }

    void addChild(UpdateGraphNode* child) {
      children.push_back(child);
    }

    void print() {
      std::cout << "Node (" << this << ") has " << children.size() << " children, needs_update = " << needs_update << ", active_edges = " << active_edges << ", counter = " << counter;
    }

    KeyType key;
    UpdateGraphNode* parent;
    std::vector<UpdateGraphNode*> children;
    bool needs_update;
    AtomicCounter counter;
    int active_edges; // NOTE(boulos): We could also increment the
    // atomic counter and decrement it back to 0
    // to determine child completion
  };

  template<class KeyType>
  class UpdateGraph {
  public:
    typedef UpdateGraphNode<KeyType> Node;
    typedef typename std::map<KeyType, Node*>::iterator node_map_iterator;
    typedef typename std::set<Node*>::iterator leaf_iterator;

    UpdateGraph() :
      root_node(0),
      graph_mutex("Update Graph Mutex") {
    }

    Node* getNode(KeyType key) {
      node_map_iterator it = node_map.find(key);
      if (it == node_map.end()) {
        throw InternalError("Asked to mark an object not in the node map");
      }

      return it->second;
    }

    void lock() {
      graph_mutex.lock();
    }
    void unlock() {
      graph_mutex.unlock();
    }

    Node* finishUpdate(KeyType key) {
      Node* node = getNode(key);
      Node* parent = node->parent;
      // Reset the node for future frames
      node->active_edges = 0;
      node->needs_update = false;
      node->counter.set(0);

      if (parent) {
        int new_val = ++parent->counter;
        if (new_val == parent->active_edges) {
          return parent;
        }
      }
      return 0;
    }

    void markObject(KeyType key) {
      Node* node = getNode(key);
      leaf_nodes.insert(node);

      while (node && !node->needs_update) {
        node->needs_update = true;
        Node* parent = node->parent;
        if (parent) {
          parent->active_edges++;
        }
        leaf_nodes.erase(parent); // Set will remove it if it's there
        node = parent;
      }
    }

    // Insert a new Key into the UpdateGraph.  For the root of the
    // graph, pass parent as NULL and this will automatically work
    // correctly.  Note that there is no current way to get the root
    // of the tree, and there may not be one single root (in case many
    // people have no parent somehow)
    Node* insert(KeyType key, Node* parent) {
      Node* new_node = new Node(key, parent);
      if (parent) {
        parent->addChild(new_node);
      } else {
        root_node = new_node;
      }
      node_map[key] = new_node;
      return new_node;
    }

    void print(KeyType key) {
      Node* node = getNode(key);
      printNode(node, "");

      leaf_iterator leaf_it = leaf_nodes.begin();
      while (leaf_it != leaf_nodes.end()) {
        std::cout << "Leaf Node: " << *leaf_it << std::endl;
        leaf_it++;
      }
    }
    void printNode(Node* node, std::string indent) {
      std::cout << indent;
      node->print();
      std::cout << std::endl;
      indent += "  ";
      for (size_t i = 0; i < node->children.size(); i++) {
        printNode(node->children[i], indent);
      }
    }

    bool finished() {
      if (root_node)
        return root_node->needs_update == false;
      return true;
    }

    std::map<KeyType, Node*> node_map;
    std::set<Node*> leaf_nodes;
    Node* root_node;
    Mutex graph_mutex;
  };

} // end namespace Manta

#endif // _MANTA_CORE_UTIL_UPDATE_GRAPH_H_
