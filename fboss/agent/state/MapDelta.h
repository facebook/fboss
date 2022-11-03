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

template <typename MAP>
struct Extractor {
  using key_type = typename MAP::key_type;
  using mapped_type = typename MAP::mapped_type;

  static const key_type& getKey(typename MAP::const_iterator i) {
    return i->first;
  }
  static const mapped_type* getValue(typename MAP::const_iterator i) {
    return &i->second;
  }
};

template <typename MAP>
struct MapDeltaTraits {
  using mapped_type = typename MAP::mapped_type;
  using DeltaValueT = DeltaValue<mapped_type, const mapped_type*>;
  using ExtractorT = Extractor<MAP>;
};

template <typename MAP, template <typename> typename Traits = MapDeltaTraits>
class MapDelta {
 public:
  using MapType = MAP;
  using VALUE = typename Traits<MAP>::DeltaValueT;
  using Node = typename MAP::mapped_type;
  using NodeWrapper = typename VALUE::NodeWrapper;
  using NodeMapExtractor = typename Traits<MAP>::ExtractorT;
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

  auto getNew() const {
    return impl_.getNew();
  }

  auto getOld() const {
    return impl_.getOld();
  }

 private:
  Impl impl_;
};

} // namespace facebook::fboss
