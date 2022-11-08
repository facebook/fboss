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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct SflowCollectorMapThriftTraits
    : public ThriftyNodeMapTraits<std::string, state::SflowCollectorFields> {
  static const KeyType parseKey(const folly::dynamic& key) {
    return key.asString();
  }

  // unfortunately the primary key "id" of this node was never explicitly
  // written in the legacy serialization and instead just built from the other
  // members. Need to override how we fetch this key
  template <typename NodeKeyT>
  static inline const std::string getKeyFromLegacyNode(
      const folly::dynamic& dyn,
      const std::string& /* keyName */) {
    // folly::to<std::string>(
    // address.getFullyQualified(), ':', address.getPort());
    return folly::to<std::string>(
        dyn["ip"].asString(), ":", dyn["port"].asString());
  }
};

using SflowCollectorMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using SflowCollectorMapThriftType =
    std::map<std::string, state::SflowCollectorFields>;

class SflowCollectorMap;
using SflowCollectorMapTraits = ThriftMapNodeTraits<
    SflowCollectorMap,
    SflowCollectorMapTypeClass,
    SflowCollectorMapThriftType,
    SflowCollector>;
/*
 * A container for the set of collectors.
 */
class SflowCollectorMap
    : public ThriftMapNode<SflowCollectorMap, SflowCollectorMapTraits> {
 public:
  using Base = ThriftMapNode<SflowCollectorMap, SflowCollectorMapTraits>;
  SflowCollectorMap() = default;
  ~SflowCollectorMap() override = default;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
