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

/*
 * NodeMapIterator is a very small wrapper around flat_map::const_iterator.
 *
 * The main difference is that dereferencing it returns only the Node,
 * and not a pair of (_Id, _Node)
 */
template <typename _Node, typename _Storage>
class NodeMapIterator
    : public std::iterator<std::forward_iterator_tag, std::shared_ptr<_Node>> {
 private:
  typedef _Storage NodeContainer;

 public:
  explicit NodeMapIterator(typename NodeContainer::const_iterator it)
      : it_(it) {}
  NodeMapIterator() : it_() {}

  const std::shared_ptr<_Node>& operator*() const {
    return it_->second;
  }
  const std::shared_ptr<_Node>* operator->() const {
    return &(it_->second);
  }

  NodeMapIterator& operator++() {
    ++it_;
    return *this;
  }
  NodeMapIterator operator++(int) {
    NodeMapIterator tmp(*this);
    ++it_;
    return tmp;
  }

  bool operator==(const NodeMapIterator& other) const {
    return it_ == other.it_;
  }
  bool operator!=(const NodeMapIterator& other) const {
    return it_ != other.it_;
  }

 private:
  typename NodeContainer::const_iterator it_;
};

/*
 * ReverseNodeMapIterator is a very small wrapper around
 * flat_map::const_reverse_iterator.
 *
 * The main difference is that dereferencing it returns only the Node,
 * and not a pair of (_Id, _Node)
 */
template <typename _Node, typename _Storage>
class ReverseNodeMapIterator
    : public std::iterator<std::forward_iterator_tag, std::shared_ptr<_Node>> {
 private:
  typedef _Storage NodeContainer;

 public:
  explicit ReverseNodeMapIterator(
      typename NodeContainer::const_reverse_iterator it)
      : it_(it) {}
  ReverseNodeMapIterator() : it_() {}

  const std::shared_ptr<_Node>& operator*() const {
    return it_->second;
  }
  const std::shared_ptr<_Node>* operator->() const {
    return &(it_->second);
  }

  ReverseNodeMapIterator& operator++() {
    ++it_;
    return *this;
  }
  ReverseNodeMapIterator operator++(int) {
    ReverseNodeMapIterator tmp(*this);
    ++it_;
    return tmp;
  }

  bool operator==(const ReverseNodeMapIterator& other) const {
    return it_ == other.it_;
  }
  bool operator!=(const ReverseNodeMapIterator& other) const {
    return it_ != other.it_;
  }

 private:
  typename NodeContainer::const_reverse_iterator it_;
};
