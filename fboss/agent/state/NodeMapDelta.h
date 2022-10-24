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
#include <glog/logging.h>

namespace facebook::fboss {

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

template <typename NODE, typename NODE_WRAPPER = std::shared_ptr<NODE>>
class DeltaValue {
 public:
  using Node = NODE;
  using NodeWrapper = NODE_WRAPPER;
  DeltaValue(const NodeWrapper& o, const NodeWrapper& n) : old_(o), new_(n) {}

  void reset(const NodeWrapper& o, const NodeWrapper& n) {
    old_ = o;
    new_ = n;
  }

  const NodeWrapper& getOld() const {
    return old_;
  }
  const NodeWrapper& getNew() const {
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
  NodeWrapper old_;
  NodeWrapper new_;
};

template <typename MAP, typename VALUE, typename MAP_EXTRACTOR>
class DeltaValueIterator {
 private:
  using InnerIter = typename MAP::const_iterator;
  auto getKey(InnerIter it) {
    return MAP_EXTRACTOR::getKey(it);
  }
  auto getValue(InnerIter it) {
    return MAP_EXTRACTOR::getValue(it);
  }

 public:
  using MapType = MAP;
  using Node = typename MAP::mapped_type;

  // Iterator properties
  using iterator_category = std::forward_iterator_tag;
  using value_type = VALUE;
  using difference_type = ptrdiff_t;
  using pointer = VALUE*;
  using reference = VALUE&;

  DeltaValueIterator(
      const MapType* oldMap,
      typename MapType::const_iterator oldIt,
      const MapType* newMap,
      typename MapType::const_iterator newIt)
      : oldIt_(oldIt),
        newIt_(newIt),
        oldMap_(oldMap),
        newMap_(newMap),
        value_(nullNode_, nullNode_) {
    // Advance to the first difference
    while (oldIt_ != oldMap_->end() && newIt_ != newMap_->end() &&
           *oldIt_ == *newIt_) {
      ++oldIt_;
      ++newIt_;
    }
    updateValue();
  }

  const value_type& operator*() const {
    return value_;
  }
  const value_type* operator->() const {
    return &value_;
  }

  DeltaValueIterator& operator++() {
    advance();
    return *this;
  }
  DeltaValueIterator operator++(int) {
    DeltaValueIterator tmp(*this);
    advance();
    return tmp;
  }

  bool operator==(const DeltaValueIterator& other) const {
    return oldIt_ == other.oldIt_ && newIt_ == other.newIt_;
  }
  bool operator!=(const DeltaValueIterator& other) const {
    return !operator==(other);
  }

 private:
  void advance() {
    // If we have already hit the end of one side, advance the other.
    // We are immediately done after this.
    if (oldIt_ == oldMap_->end()) {
      // advance() shouldn't be called if we are already at the end
      CHECK(newIt_ != newMap_->end());
      ++newIt_;
      updateValue();
      return;
    } else if (newIt_ == newMap_->end()) {
      ++oldIt_;
      updateValue();
      return;
    }

    // We aren't at the end of either side.
    // Check to see which one (or both) needs to be advanced.
    const auto& oldKey = getKey(oldIt_);
    const auto& newKey = getKey(newIt_);
    if (oldKey < newKey) {
      ++oldIt_;
    } else if (oldKey > newKey) {
      ++newIt_;
    } else {
      ++oldIt_;
      ++newIt_;
    }

    // Advance past any unchanged nodes.
    while (oldIt_ != oldMap_->end() && newIt_ != newMap_->end() &&
           *oldIt_ == *newIt_) {
      ++oldIt_;
      ++newIt_;
    }
    updateValue();
  }
  void updateValue() {
    if (oldIt_ == oldMap_->end()) {
      if (newIt_ == newMap_->end()) {
        value_.reset(nullNode_, nullNode_);
      } else {
        value_.reset(nullNode_, getValue(newIt_));
      }
      return;
    }
    if (newIt_ == newMap_->end()) {
      value_.reset(getValue(oldIt_), nullNode_);
      return;
    }
    const auto& oldKey = getKey(oldIt_);
    const auto& newKey = getKey(newIt_);
    if (oldKey < newKey) {
      value_.reset(getValue(oldIt_), nullNode_);
    } else if (newKey < oldKey) {
      value_.reset(nullNode_, getValue(newIt_));
    } else {
      value_.reset(getValue(oldIt_), getValue(newIt_));
    }
  }

 public:
  InnerIter oldIt_;
  InnerIter newIt_;
  const MapType* oldMap_;
  const MapType* newMap_;
  VALUE value_;
  static const typename VALUE::NodeWrapper nullNode_;
};

template <typename MAP, typename VALUE, typename MAP_EXTRACTOR>
const typename VALUE::NodeWrapper
    DeltaValueIterator<MAP, VALUE, MAP_EXTRACTOR>::nullNode_ = nullptr;
template <
    typename MAP,
    typename VALUE,
    typename ITERATOR,
    typename MAPPOINTERTRAITS = MapPointerTraits<MAP>>
class MapDeltaImpl {
 public:
  using MapPointerType = typename MAPPOINTERTRAITS::MapPointerType;
  using RawConstPointerType = typename MAPPOINTERTRAITS::RawConstPointerType;
  using MapType = MAP;
  using Node = typename MAP::mapped_type;
  using NodeWrapper = typename VALUE::NodeWrapper;
  using Iterator = ITERATOR;

  MapDeltaImpl(MapPointerType&& oldMap, MapPointerType&& newMap)
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
  Iterator begin() const {
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

  /*
   * Return an iterator pointing just past the last change.
   */
  Iterator end() const {
    if (!old_) {
      return Iterator(getNew(), new_->end(), getNew(), new_->end());
    }
    if (!new_) {
      return Iterator(getOld(), old_->end(), getOld(), old_->end());
    }
    return Iterator(getOld(), old_->end(), getNew(), new_->end());
  }

 private:
  MapPointerType old_;
  MapPointerType new_;
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
    typename VALUE =
        DeltaValue<typename MAP::Node, std::shared_ptr<typename MAP::Node>>,
    typename MAPPOINTERTRAITS = MapPointerTraits<MAP>>
class NodeMapDelta {
 public:
  using MapPointerType = typename MAPPOINTERTRAITS::MapPointerType;
  using RawConstPointerType = typename MAPPOINTERTRAITS::RawConstPointerType;
  using MapType = MAP;
  using Node = typename MAP::Node;
  using NodeWrapper = std::shared_ptr<Node>;

  struct NodeMapExtractor {
    using KeyType = typename MAP::KeyType;
    static KeyType getKey(typename MAP::const_iterator i) {
      return MAP::Traits::getKey(*i);
    }
    static const std::shared_ptr<Node> getValue(
        typename MAP::const_iterator i) {
      return *i;
    }
  };
  using Iterator = DeltaValueIterator<MAP, VALUE, NodeMapExtractor>;
  using Impl = MapDeltaImpl<MAP, VALUE, Iterator, MAPPOINTERTRAITS>;

  NodeMapDelta(MapPointerType&& oldMap, MapPointerType&& newMap)
      : impl_(std::move(oldMap), std::move(newMap)) {}

  RawConstPointerType getOld() const {
    return impl_.getOld();
  }
  RawConstPointerType getNew() const {
    return impl_.getNew();
  }
  Iterator begin() const {
    return impl_.begin();
  }
  Iterator end() const {
    return impl_.end();
  }

 private:
  Impl impl_;
};

} // namespace facebook::fboss
