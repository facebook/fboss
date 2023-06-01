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
struct ExtractorT {
  using key_type = typename MAP::key_type;
  using mapped_type = typename MAP::mapped_type;

  template <typename T>
  static constexpr bool is_mapped_type_shared_ptr_v =
      IsSharedPtr<typename T::mapped_type>::value;

  using value_type = std::conditional_t<
      is_mapped_type_shared_ptr_v<MAP>,
      mapped_type,
      std::add_pointer_t<std::add_const_t<mapped_type>>>;

  static const key_type& getKey(typename MAP::const_iterator i) {
    return i->first;
  }

  template <
      typename T = MAP,
      typename = std::enable_if_t<!is_mapped_type_shared_ptr_v<T>>>
  static const mapped_type* getValue(typename MAP::const_iterator i) {
    return &i->second;
  }

  template <
      typename T = MAP,
      typename = std::enable_if_t<is_mapped_type_shared_ptr_v<T>>>
  static const mapped_type getValue(typename MAP::const_iterator i) {
    return i->second;
  }
};

template <typename MAP>
struct MapDeltaTraits {
  using mapped_type = typename MAP::mapped_type;
  using Extractor = ExtractorT<MAP>;
  using value_type = typename Extractor::value_type;
  using Delta = DeltaValue<mapped_type, value_type>;
  using NodeWrapper = typename Delta::NodeWrapper;
  using DeltaValueIterator = DeltaValueIteratorT<MAP, Delta, Extractor>;
  using MapPointerTraits = MapPointerTraitsT<MAP>;
};

template <typename MAP, template <typename> typename Traits = MapDeltaTraits>
class MapDelta {
 public:
  using MapType = MAP;
  using VALUE = typename Traits<MAP>::Delta;
  using Node = typename Traits<MAP>::mapped_type;
  using NodeWrapper = typename Traits<MAP>::NodeWrapper;
  using NodeMapExtractor = typename Traits<MAP>::Extractor;
  using Iterator = typename Traits<MAP>::DeltaValueIterator;
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

template <typename MAP, template <typename> typename Traits = MapDeltaTraits>
struct ThriftMapDelta : MapDelta<MAP, Traits> {
  using Base = MapDelta<MAP, MapDeltaTraits>;
  ThriftMapDelta(const MAP* oldMap, const MAP* newMap) : Base(oldMap, newMap) {}
};

template <typename OuterMap>
struct MultiSwitchMapDeltaTraits
    : NestedMapDeltaTraits<
          OuterMap, /* outer map */
          typename OuterMap::InnerMap, /* inner map */
          ThriftMapDelta, /* map delta for outer map */
          ThriftMapDelta, /* map delta for inner map */
          MapDeltaTraits, /* map delta traits for outer map */
          MapDeltaTraits /* map delta traits for inner map */
          > {};

template <typename OuterMap>
struct MultiSwitchMapDelta
    : NestedMapDelta<MultiSwitchMapDeltaTraits<OuterMap>> {
  using Base = NestedMapDelta<MultiSwitchMapDeltaTraits<OuterMap>>;
  using Base::Base;
};

} // namespace facebook::fboss
