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

#include "fboss/agent/Utils.h"
#include "fboss/agent/types.h"

#include <boost/cast.hpp>
#include <boost/container/flat_map.hpp>
#include <glog/logging.h>
#include <memory>
#include <type_traits>

#include <folly/dynamic.h>
#include <folly/json.h>

namespace facebook::fboss {

/*
 * NodeBase is the base class for all nodes in our SwitchState tree.
 *
 * NodeBase itself actually provides only a handful of methods common to all
 * nodes.  In particular, all entities in the state tree must provide a
 * published flag and a generation number.
 *
 * However, beyond this, most functionality is specific to the individual node
 * subclasses.  All nodes should generally implement clone() APIs as well,
 * but clone() needs to return the specific subclass type, and therefore cannot
 * be part of the NodeBase API.
 *
 * Most node implementations should derive from NodeBaseT to implement clone()
 * and to track the publish flag and generation number correctly.
 */
class NodeBase {
 public:
  virtual ~NodeBase() = default;

  /*
   * Returns if this state is published yet.
   *
   * Published states are visible by multiple threads, and are read-only.
   * The state may only be modified while it is still unpublished.  Once it has
   * been published, modifications are only possible by cloning a new
   * SwitchState object which replaces the old state.
   */
  bool isPublished() const {
    return published_;
  }

  /*
   * Indicate that this SwitchState object is about to be published as the
   * currently active state.
   */
  virtual void publish() {
    published_ = true;
  }

  /*
   * Get the generation number for this state object.
   *
   * The generation number is incremented with each state change.
   */
  uint32_t getGeneration() const {
    return generation_;
  }

  /**
   * Inherit the generation number from an old node
   *
   * NOTE: In most cases, you do not need to use this API.
   * A node's generation number is set automatically to either 0
   * (for a new node) or "generation number from the old node + 1" (for a
   * cloned node).
   * This API is added here for the use case where a complete new node is
   * created without using clone(). And then correct the generation number
   * of the new node based on the old node.
   */
  void inheritGeneration(const NodeBase& old) {
    CHECK(!published_);
    generation_ = old.getGeneration() + 1;
  }

  /*
   * Get the NodeID for this node.
   *
   * A NodeID represents a logical node in the state tree.  The NodeID stays
   * the same for the entire lifetime of a node, even as it is modified via the
   * copy-on-write process.  In other words, clone() returns a new Node with
   * the same NodeID.
   *
   * For example, say we create a new Vlan node to represent VLAN 5, and it
   * gets created with NodeID 12.  Whenever a modification is made to this
   * Vlan, a new Vlan made will be created via the copy-on-write process.
   * However, all nodes referring to VLAN 5 will continue to have NodeID 12.
   *
   * The main purpose for the NodeID field is so that HwSwitch implementations
   * can have a consistent way of referring to a node.  This allows the
   * HwSwitch to store maps from NodeID to their own internal state.  The exact
   * node object (and the pointer to it) may change as the SwitchState is
   * modified, but the NodeID will always remain the same.
   *
   * However, note that there is no convenient way to look up a node from its
   * NodeID (apart from doing a full walk of the SwitchState).
   */
  NodeID getNodeID() const {
    return nodeID_;
  }

 protected:
  NodeBase();
  NodeBase(NodeID id, uint32_t generation)
      : nodeID_(id), generation_(generation) {}

 private:
  NodeID nodeID_{0};
  uint32_t generation_{0};
  bool published_{false};
};

/*
 * NodeBaseT is a helper class for implementing nodes in the SwitchState tree.
 *
 * To use NodeBaseT, nodes must define a structure containing all of their copy
 * on write fields.  This structure is supplied as the second template
 * parameter.  The first template parameter is the subclass itself implementing
 * the node.
 *
 * NodeBaseT implements clone(), enforces the published state, and updates the
 * generation number correctly.
 *
 * NodeBaseT implements clone() by relying on the field structure's copy
 * constructor.  If the field structure contains other constructors that also
 * accept a "const FieldsT&" first argument with additional arguments, clone()
 * methods will also be provided with the same arguments.
 *
 * NodeBaseT enforces the published state by ensuring that any modifications to
 * the fields must occur via the writableFields() method.  writableFields() is
 * the only API for getting a writable pointer of the fields, and it ensures
 * that it can only be called on unpublished nodes.a
 *
 * Fields structures must provided a forEachChild() template method, which
 * calls the specified function on child node stored in the fields.  This is
 * used to implement publish().
 *
 * For an example of how to use NodeBaseT, see Vlan.h or Port.h.
 */
template <typename NodeT, typename FieldsT>
class NodeBaseT : public NodeBase {
 public:
  using Node = NodeT;
  using Fields = FieldsT;

  ~NodeBaseT() override = default;

  /*
   * Create a new version of the node, cloned from this one.
   *
   * The new node is unpublished, and it's generation number has been
   * incremented.  All of its fields will be the same as this nodes' fields.
   * It will point to the same children nodes as this node.  (The children
   * nodes themselves are not cloned.)
   *
   * This can be used to create a new node that can be modified before it is
   * published and committed.
   *
   * The new node is returned as a shared_ptr for efficient allocation with
   * make_shared (since published node objects must eventually be stored in a
   * shared_ptr).  However, the caller is the sole owner of the new object when
   * it is returned.
   */
  std::shared_ptr<Node> clone() const;

  /*
   * Note that most NodeBase subclasses also provide a modify() function.
   *
   * This function returns a modifiable version of the node.  If the node is
   * unpublished modify() just returns the current object.  However, if the
   * object is published (and therefore unmodifiable), a newly cloned version
   * will be returned.  The modify() method always takes a pointer to the
   * SwitchState, and updates the state to contain the new node.  The
   * SwitchState itself will also be cloned if necessary.
   */

  template <typename... Args>
  std::enable_if_t<
      std::is_constructible<Fields, const Fields&, Args...>::value,
      std::shared_ptr<Node>>
  clone(Args&&... args) const {
    return std::allocate_shared<NodeT>(
        CloneAllocator(), self(), std::forward<Args>(args)...);
  }

  void publish() override;

  const Fields* getFields() const {
    return &fields_;
  }
  Fields* writableFields() {
    CHECK(!isPublished());
    return &fields_;
  }

  /*
   * Serialize to folly::dynamic
   * Generate folly::dynamic toFollyDynamic if
   * Fields::toFollyDynamic exists.
   */
  virtual folly::dynamic toFollyDynamic() const = 0;

  /*
   * Serialize to JSON
   * Generate folly::dynamic toFollyDynamic if
   * Fields::toFollyDynamic exists.
   */
  folly::fbstring toJson() {
    return folly::toJson(toFollyDynamic());
  }

  template <typename... Args>
  explicit NodeBaseT(Args&&... args) : fields_(std::forward<Args>(args)...) {}

  // Create node from fields
  explicit NodeBaseT(const FieldsT& fields) : fields_(fields) {}

  // Constructors used by clone()
  template <typename... Args>
  NodeBaseT(const Node* orig, Args&&... args)
      : NodeBase(orig->getNodeID(), orig->getGeneration() + 1),
        fields_(orig->fields_, std::forward<Args>(args)...) {}

 protected:
  class CloneAllocator : public std::allocator<NodeT> {
   public:
    template <typename... Args>
    void construct(void* p, Args&&... args) {
      new (p) NodeT(std::forward<Args>(args)...);
    }
  };

 private:
  // Forbidden copy constructor and assignment operator
  NodeBaseT(NodeBaseT const&) = delete;
  NodeBaseT& operator=(NodeBaseT const&) = delete;

  Node* self() {
    return boost::polymorphic_downcast<Node*>(this);
  }
  const Node* self() const {
    return boost::polymorphic_downcast<const Node*>(this);
  }

  Fields fields_;
};

} // namespace facebook::fboss
