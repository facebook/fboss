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

#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/Traits.h"

#include <folly/dynamic.h>

namespace facebook::fboss {
constexpr auto kKey = "adapterkey";
constexpr auto kAdapterKeys = "adapterKeys";

template <typename SaiObjectTraits>
std::enable_if_t<AdapterKeyIsObjectId<SaiObjectTraits>::value, folly::dynamic>
toFollyDynamic(const typename SaiObjectTraits::AdapterKey& adapterKey) {
  folly::dynamic key = folly::dynamic::object;
  key[kKey] = static_cast<long>(adapterKey);
  return key;
}

template <typename SaiObjectTraits>
std::enable_if_t<
    AdapterKeyIsObjectId<SaiObjectTraits>::value,
    typename SaiObjectTraits::AdapterKey>
fromFollyDynamic(const folly::dynamic& obj) {
  return typename SaiObjectTraits::AdapterKey(obj[kKey].asInt());
}

template <typename SaiObjectTraits>
std::
    enable_if_t<AdapterKeyIsEntryStruct<SaiObjectTraits>::value, folly::dynamic>
    toFollyDynamic(const typename SaiObjectTraits::AdapterKey& adapterKey) {
  return adapterKey.toFollyDynamic();
}

template <typename SaiObjectTraits>
std::enable_if_t<
    AdapterKeyIsEntryStruct<SaiObjectTraits>::value,
    typename SaiObjectTraits::AdapterKey>
fromFollyDynamic(const folly::dynamic& obj) {
  return SaiObjectTraits::AdapterKey::fromFollyDynamic(obj);
}

} // namespace facebook::fboss
