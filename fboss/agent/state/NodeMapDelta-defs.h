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

#include <glog/logging.h>
#include "fboss/agent/state/NodeMapDelta.h"

namespace facebook::fboss {

template <typename MAP, typename VALUE, typename MAPPOINTERTRAITS>
std::shared_ptr<typename MAP::Node>
    NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::Iterator::nullNode_;

template <typename MAP, typename VALUE, typename MAPPOINTERTRAITS>
NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::Iterator::Iterator(
    const MapType* oldMap,
    typename MapType::Iterator oldIt,
    const MapType* newMap,
    typename MapType::Iterator newIt)
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

template <typename MAP, typename VALUE, typename MAPPOINTERTRAITS>
NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::Iterator::Iterator()
    : oldIt_(),
      newIt_(),
      oldMap_(nullptr),
      newMap_(nullptr),
      value_(nullNode_, nullNode_) {}

template <typename MAP, typename VALUE, typename MAPPOINTERTRAITS>
void NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::Iterator::updateValue() {
  if (oldIt_ == oldMap_->end()) {
    if (newIt_ == newMap_->end()) {
      value_.reset(nullNode_, nullNode_);
    } else {
      value_.reset(nullNode_, *newIt_);
    }
    return;
  }
  if (newIt_ == newMap_->end()) {
    value_.reset(*oldIt_, nullNode_);
    return;
  }
  auto oldKey = Traits::getKey(*oldIt_);
  auto newKey = Traits::getKey(*newIt_);
  if (oldKey < newKey) {
    value_.reset(*oldIt_, nullNode_);
  } else if (newKey < oldKey) {
    value_.reset(nullNode_, *newIt_);
  } else {
    value_.reset(*oldIt_, *newIt_);
  }
}

template <typename MAP, typename VALUE, typename MAPPOINTERTRAITS>
void NodeMapDelta<MAP, VALUE, MAPPOINTERTRAITS>::Iterator::advance() {
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
  auto oldKey = Traits::getKey(*oldIt_);
  auto newKey = Traits::getKey(*newIt_);
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

} // namespace facebook::fboss
