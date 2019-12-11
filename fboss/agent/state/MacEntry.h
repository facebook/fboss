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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace facebook {
namespace fboss {

struct MacEntryFields {
  explicit MacEntryFields(folly::MacAddress mac, PortDescriptor portDescr)
      : mac_(mac), portDescr_(portDescr) {}

  MacEntryFields(
      folly::MacAddress mac,
      PortDescriptor portDescr,
      cfg::AclLookupClass classID)
      : mac_(mac), portDescr_(portDescr), classID_(classID) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static MacEntryFields fromFollyDynamic(const folly::dynamic& json);

  folly::MacAddress mac_;
  PortDescriptor portDescr_;
  std::optional<cfg::AclLookupClass> classID_{std::nullopt};
};

class MacEntry : public NodeBaseT<MacEntry, MacEntryFields> {
 public:
  explicit MacEntry(folly::MacAddress mac, PortDescriptor portDescr)
      : NodeBaseT(mac, portDescr) {}

  static std::shared_ptr<MacEntry> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = MacEntryFields::fromFollyDynamic(json);
    return std::make_shared<MacEntry>(fields);
  }

  static std::shared_ptr<MacEntry> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const MacEntry& macEntry) {
    return getFields()->mac_ == macEntry.getMac() &&
        getFields()->portDescr_ == macEntry.getPort() &&
        getFields()->classID_ == macEntry.getClassID();
  }

  folly::MacAddress getMac() const {
    return getFields()->mac_;
  }

  PortDescriptor getPort() const {
    return getFields()->portDescr_;
  }

  std::optional<cfg::AclLookupClass> getClassID() const {
    return getFields()->classID_;
  }

  folly::MacAddress getID() const {
    return getMac();
  }

  void setPortDescriptor(PortDescriptor portDescr) {
    writableFields()->portDescr_ = portDescr;
  }

  std::string str() const;

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace fboss
} // namespace facebook
