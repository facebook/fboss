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

#include <fboss/agent/state/NodeMapDelta.h>

namespace facebook::fboss {

template <
    typename MAP,
    typename VALUE =
        DeltaValue<typename MAP::mapped_type, const typename MAP::mapped_type*>>
class MapDelta {
 public:
  using MapType = MAP;
  using Node = typename MAP::mapped_type;
  using NodeWrapper = typename VALUE::NodeWrapper;
  struct NodeMapExtractor {
    static const auto& getKey(typename MAP::const_iterator i) {
      return i->first;
    }
    static auto getValue(typename MAP::const_iterator i) {
      return &i->second;
    }
  };
  using Iterator = DeltaValueIterator<MAP, VALUE, NodeMapExtractor>;

  MapDelta(const MAP* oldMap, const MAP* newMap) : old_(oldMap), new_(newMap) {}

  /*
   * Return an iterator pointing to the first change.
   */
  Iterator begin() const {
    return Iterator(old_, old_->begin(), new_, new_->begin());
  }

  /*
   * Return an iterator pointing just past the last change.
   */
  Iterator end() const {
    return Iterator(old_, old_->end(), new_, new_->end());
  }

 private:
  const MAP *old_, *new_;
};

} // namespace facebook::fboss
