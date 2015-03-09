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

#include <functional>
#include <memory>
#include <type_traits>
#include <cstddef>

#include <folly/ApplyTuple.h>

namespace facebook { namespace fboss {

template<typename NODE>
class DeltaValue;

/*
 * NodeMapDelta contains code for examining the differences between two NodeMap
 * objects.
 *
 * The main function of this class is the Iterator that it provides.  This
 * allows caller to walk over the changed, added, and removed nodes.
 */
template<typename MAP, typename VALUE = DeltaValue<typename MAP::Node>>
class NodeMapDelta {
 public:
  typedef MAP MapType;
  typedef typename MAP::Node Node;
  class Iterator;

  NodeMapDelta(const MapType* oldMap, const MapType* newMap)
    : old_(oldMap),
      new_(newMap) {}

  const MapType* getOld() const {
    return old_;
  }
  const MapType* getNew() const {
    return new_;
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
   * Note that we assume NodeMapDelta is always used by StateDelta.  StateDelta
   * holds a shared_ptr to the old and new SwitchState objects, so that we can
   * use raw pointers here, rather than shared_ptrs.
   */
  const MapType* old_;
  const MapType* new_;
};

template<typename NODE>
class DeltaValue {
 public:
  typedef NODE Node;
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
template<typename MAP, typename VALUE>
class NodeMapDelta<MAP, VALUE>::Iterator {
 public:
  typedef MAP MapType;
  typedef typename MAP::Node Node;

  // Iterator properties
  typedef std::forward_iterator_tag iterator_category;
  typedef VALUE value_type;
  typedef ptrdiff_t difference_type;
  typedef VALUE* pointer;
  typedef VALUE& reference;

  Iterator(const MapType* oldMap,
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
  typedef typename MapType::Iterator InnerIter;
  typedef typename MapType::Traits Traits;

  void advance();
  void updateValue();

  InnerIter oldIt_;
  InnerIter newIt_;
  const MapType* oldMap_;
  const MapType* newMap_;
  VALUE value_;

  static std::shared_ptr<Node> nullNode_;
};

template<typename MAP, typename VALUE>
typename NodeMapDelta<MAP, VALUE>::Iterator
NodeMapDelta<MAP, VALUE>::begin() const {
  if (old_ == new_) {
    return end();
  }
  // To support deltas where the old node is null (to represent newly created
  // nodes), point the old side of the iterator at the new node, but start it
  // at the end of the map.
  if (!old_) {
    return Iterator(new_, new_->end(), new_, new_->begin());
  }
  // And vice-versa for the new node being null (to represent removed nodes).
  if (!new_) {
    return Iterator(old_, old_->begin(), old_, old_->end());
  }
  return Iterator(old_, old_->begin(), new_, new_->begin());
}

template<typename MAP, typename VALUE>
typename NodeMapDelta<MAP, VALUE>::Iterator
NodeMapDelta<MAP, VALUE>::end() const {
  if (!old_) {
    return Iterator(new_, new_->end(), new_, new_->end());
  }
  if (!new_) {
    return Iterator(old_, old_->end(), old_, old_->end());
  }
  return Iterator(old_, old_->end(), new_, new_->end());
}

}} // facebook::fboss
