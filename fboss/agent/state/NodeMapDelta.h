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

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>

#include <folly/functional/ApplyTuple.h>

namespace facebook::fboss {

template <typename NODE>
class DeltaValue;

template <typename MAP>
class MapPointerTraits {
 public:
  using RawConstPointerType = const MAP*;
  // Embed const in MapPointerType for raw pointers.
  // Since we want to use both raw and unique_ptr,
  // we don't want to add const to the Map pointer type
  // in function declarations, where we will want to move
  // from unique_ptr (moving from const unique_ptr is not
  // permitted since its a modifying operation).
  using MapPointerType = const MAP*;
  static RawConstPointerType getRawPointer(MapPointerType map) {
    return map;
  }
};

template <typename MAP>
class MapUniquePointerTraits {
 public:
  using RawConstPointerType = const MAP*;
  // Non const MapPointer type, const unique_ptr is
  // severly restrictive when received as a function
  // argument, since it can't be moved from.
  using MapPointerType = std::unique_ptr<MAP>;
  static RawConstPointerType getRawPointer(const MapPointerType& map) {
    return map.get();
  }
};
/*
 * NodeMapDelta contains code for examining the differences between two NodeMap
 * objects.
 *
 * The main function of this class is the Iterator that it provides.  This
 * allows caller to walk over the changed, added, and removed nodes.
 */
template <
    typename MAP,
    typename VALUE = DeltaValue<typename MAP::Node>,
    typename MAPPOINTERTRAITS = MapPointerTraits<MAP>>
class NodeMapDelta {
 public:
  using MapPointerType = typename MAPPOINTERTRAITS::MapPointerType;
  using RawConstPointerType = typename MAPPOINTERTRAITS::RawConstPointerType;
  using Node = typename MAP::Node;
  class Iterator;

  NodeMapDelta(MapPointerType&& oldMap, MapPointerType&& newMap)
      : old_(std::move(oldMap)), new_(std::move(newMap)) {}

  RawConstPointerType getOld() const {
    return MAPPOINTERTRAITS::getRawPointer(old_);
  }
  RawConstPointerType getNew() const {
    return MAPPOINTERTRAITS::getRawPointer(new_);
  }

  /*
   * Return an iterator pointing to the first change.
   */
  Iterator begin() const;

  /*
   * Return an iterator pointing just past the last change.
   */
  Iterator end() const;

 private:
  /*
   * NodeMapDelta is used by StateDelta.  StateDelta holds a shared_ptr to
   * the old and new SwitchState objects, which in turn holds
   * shared_ptrs to NodeMaps, hence in the common case use raw pointers here
   * and don't bother with ownership. However there are cases where collections
   * are not easily mapped to a NodeMap structure, but we still want to box
   * them into a NodeMap while computing delta. In this case NodeMap is created
   * on the fly and we pass ownership to NodeMapDelta object via a unique_ptr.
   * See MapPointerTraits and MapUniquePointerTraits classes for details.
   */
  MapPointerType old_;
  MapPointerType new_;
};

template <typename NODE>
class DeltaValue {
 public:
  using Node = NODE;
  DeltaValue(const std::shared_ptr<Node>& o, const std::shared_ptr<Node>& n)
      : old_(o), new_(n) {}

  void reset(const std::shared_ptr<Node>& o, const std::shared_ptr<Node>& n) {
    old_ = o;
    new_ = n;
  }

  const std::shared_ptr<Node>& getOld() const {
    return old_;
  }
  const std::shared_ptr<Node>& getNew() const {
    return new_;
  }

 private:
  // TODO: We should probably change this to store raw pointers.
  // Storing shared_ptrs means a lot of unnecessary reference count increments
  // and decrements as we iterate through the changes.
  //
  // It should be sufficient for users to receive raw pointers rather than
  // shared_ptrs.  Callers should always maintain a shared_ptr to the top-level
  // SwitchState object, so they should never need the shared_ptr reference
  // count on individual nodes.
  std::shared_ptr<Node> old_;
  std::shared_ptr<Node> new_;
};

/*
 * An iterator for walking over the Nodes that changed between the two
 * NodeMaps.
 */
template <typename MAP, typename VALUE, typename MAPPOINTERTRAITS>
class NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::Iterator {
 public:
  using MapType = MAP;
  using Node = typename MAP::Node;

  // Iterator properties
  using iterator_category = std::forward_iterator_tag;
  using value_type = VALUE;
  using difference_type = ptrdiff_t;
  using pointer = VALUE*;
  using reference = VALUE&;

  Iterator(
      const MapType* oldMap,
      typename MapType::Iterator oldIt,
      const MapType* newMap,
      typename MapType::Iterator newIt);
  Iterator();

  const value_type& operator*() const {
    return value_;
  }
  const value_type* operator->() const {
    return &value_;
  }

  Iterator& operator++() {
    advance();
    return *this;
  }
  Iterator operator++(int) {
    Iterator tmp(*this);
    advance();
    return tmp;
  }

  bool operator==(const Iterator& other) const {
    return oldIt_ == other.oldIt_ && newIt_ == other.newIt_;
  }
  bool operator!=(const Iterator& other) const {
    return !operator==(other);
  }

 private:
  using InnerIter = typename MapType::Iterator;
  using Traits = typename MapType::Traits;

  void advance();
  void updateValue();

  InnerIter oldIt_{nullptr};
  InnerIter newIt_{nullptr};
  const MapType* oldMap_{nullptr};
  const MapType* newMap_{nullptr};
  VALUE value_;

  static std::shared_ptr<Node> nullNode_;
};

template <typename MAP, typename VALUE, typename MAPPOINTERTRAITS>
typename NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::Iterator
NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::begin() const {
  if (old_ == new_) {
    return end();
  }
  // To support deltas where the old node is null (to represent newly created
  // nodes), point the old side of the iterator at the new node, but start it
  // at the end of the map.
  if (!old_) {
    return Iterator(getNew(), new_->end(), getNew(), new_->begin());
  }
  // And vice-versa for the new node being null (to represent removed nodes).
  if (!new_) {
    return Iterator(getOld(), old_->begin(), getOld(), old_->end());
  }
  return Iterator(getOld(), old_->begin(), getNew(), new_->begin());
}

template <typename MAP, typename VALUE, typename MAPPOINTERTRAITS>
typename NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::Iterator
NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::end() const {
  if (!old_) {
    return Iterator(getNew(), new_->end(), getNew(), new_->end());
  }
  if (!new_) {
    return Iterator(getOld(), old_->end(), getOld(), old_->end());
  }
  return Iterator(getOld(), old_->end(), getNew(), new_->end());
}

} // namespace facebook::fboss
