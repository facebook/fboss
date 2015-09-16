/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <boost/container/flat_map.hpp>

#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/NodeMapIterator.h"

namespace facebook { namespace fboss {

/*
 * NodeMapFields defines the fields contained inside a NodeMapT instantiation
 */
template <typename TraitsT>
struct NodeMapFields {
  typedef typename TraitsT::KeyType KeyType;
  typedef typename TraitsT::Node Node;
  typedef typename TraitsT::ExtraFields ExtraFields;
  typedef boost::container::flat_map<KeyType, std::shared_ptr<Node>>
    NodeContainer;

  NodeMapFields() {}
  NodeMapFields(const NodeMapFields& other, NodeContainer nodes)
    : nodes(std::move(nodes)),
      extra(other.extra) {}

  template<typename Fn>
  void forEachChild(Fn fn) {
    for (const auto& nodePtr : nodes) {
      fn(nodePtr.second.get());
    }
    extra.forEachChild(fn);
  }

  NodeContainer nodes;
  typename TraitsT::ExtraFields extra;
};

struct NodeMapNoExtraFields {
  template<typename Fn> void forEachChild(Fn fn) {}

  folly::dynamic toFollyDynamic() const {
    return folly::dynamic::object;
  }

  static NodeMapNoExtraFields
  fromFollyDynamic(const folly::dynamic& json) {
    return NodeMapNoExtraFields();
  }
};

template<typename KeyT, typename NodeT, typename ExtraT = NodeMapNoExtraFields>
struct NodeMapTraits {
  typedef KeyT KeyType;
  typedef NodeT Node;
  typedef ExtraT ExtraFields;

  static KeyType getKey(const std::shared_ptr<Node>& node) {
    return node->getID();
  }
};

/*
 * A helper class for implementing state nodes that store a set of Node
 * children.
 *
 * The TraitsT class specifies the Node type, and how to get the map key from a
 * Node object.  The default Traits implementation calls the getID() method on
 * the Node.
 */
template <typename MapTypeT, typename TraitsT>
class NodeMapT : public NodeBaseT<MapTypeT,
                                  NodeMapFields<TraitsT>> {
 public:
  typedef TraitsT Traits;
  typedef typename TraitsT::KeyType KeyType;
  typedef typename TraitsT::Node Node;
  typedef typename TraitsT::ExtraFields ExtraFields;
  typedef MapTypeT MapType;
  typedef NodeMapFields<TraitsT> Fields;
  typedef typename Fields::NodeContainer NodeContainer;
  typedef NodeMapIterator<Node, NodeContainer> Iterator;
  typedef ReverseNodeMapIterator<Node, NodeContainer> ReverseIterator;

  const std::shared_ptr<Node>& getNode(KeyType key) const;
  std::shared_ptr<Node> getNodeIf(KeyType key) const;

  size_t size() const {
    return this->getFields()->nodes.size();
  }

  const NodeContainer& getAllNodes() const {
    return this->getFields()->nodes;
  }
  NodeContainer& writableNodes() {
    return this->writableFields()->nodes;
  }

  const ExtraFields& getExtraFields() const {
    return this->getFields()->extra;
  }
  ExtraFields& writableExtraFields() {
    return this->writableFields()->extra;
  }

  Iterator begin() const {
    return(Iterator(getAllNodes().begin()));
  }
  Iterator end() const {
    return(Iterator(getAllNodes().end()));
  }
  ReverseIterator rbegin() const {
    return(ReverseIterator(getAllNodes().rbegin()));
  }
  ReverseIterator rend() const {
    return(ReverseIterator(getAllNodes().rend()));
  }


  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */
  void addNode(const std::shared_ptr<Node>& node);
  void updateNode(const std::shared_ptr<Node>& node);
  void removeNode(const std::shared_ptr<Node>& node);
  std::shared_ptr<Node> removeNode(const KeyType& key);
  std::shared_ptr<Node> removeNodeIf(const KeyType& key);

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const override;

  /*
   * Serialize to json string
   */
  static std::shared_ptr<MapTypeT> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  /*
   * Deserialize to folly::dynamic
   */
  static std::shared_ptr<MapTypeT>
    fromFollyDynamic(const folly::dynamic& json);

  static constexpr char kExtraFields[] = "extraFields";
  static constexpr char kEntries[] = "entries";

 private:
  // Inherit the constructor required for clone()
  using NodeBaseT<MapTypeT, NodeMapFields<TraitsT>>::NodeBaseT;
  friend class CloneAllocator;
};

}} // namespace facebook::fboss
