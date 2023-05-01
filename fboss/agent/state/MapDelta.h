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

namespace {
template <typename T>
struct IsSharedPtr {
  static constexpr bool value = false;
};

template <typename T>
struct IsSharedPtr<std::shared_ptr<T>> {
  static constexpr bool value = true;
};
} // namespace

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
  using DeltaValue = DeltaValue<mapped_type, const mapped_type*>;
  using Extractor = Extractor<MAP>;
};

template <typename MAP, template <typename> typename Traits = MapDeltaTraits>
class MapDelta {
 public:
  using MapType = MAP;
  using VALUE = typename Traits<MAP>::DeltaValue;
  using Node = typename MAP::mapped_type;
  using NodeWrapper = typename VALUE::NodeWrapper;
  using NodeMapExtractor = typename Traits<MAP>::Extractor;
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

template <typename MAP>
struct ThriftMapNodeExtractor {
  using key_type = typename MAP::key_type;
  using mapped_type = typename MAP::mapped_type;
  using value_type = std::conditional_t<
      IsSharedPtr<mapped_type>::value,
      mapped_type,
      const mapped_type*>;

  static const key_type& getKey(typename MAP::const_iterator i) {
    return i->first;
  }
  static value_type getValue(typename MAP::const_iterator i) {
    if constexpr (IsSharedPtr<mapped_type>::value) {
      return i->second;
    } else {
      return &i->second;
    }
  }
};

template <typename MAP>
struct ThriftMapNodeDeltaTraits {
  using mapped_type = typename MAP::mapped_type;
  using Extractor = ThriftMapNodeExtractor<MAP>;
  using DeltaValue = DeltaValue<mapped_type, typename Extractor::value_type>;
};

template <typename MAP>
struct ThriftMapDelta : MapDelta<MAP, ThriftMapNodeDeltaTraits> {
  using Base = MapDelta<MAP, ThriftMapNodeDeltaTraits>;
  ThriftMapDelta(const MAP* oldMap, const MAP* newMap) : Base(oldMap, newMap) {}
};

} // namespace facebook::fboss
