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
  using Impl = MapDeltaImpl<MAP, VALUE, Iterator>;

  MapDelta(const MAP* oldMap, const MAP* newMap)
      : impl_(std::move(oldMap), std::move(newMap)) {}

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
