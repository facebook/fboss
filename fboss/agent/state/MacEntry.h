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

namespace facebook::fboss {

enum class MacEntryType : uint8_t { DYNAMIC_ENTRY, STATIC_ENTRY };

struct MacEntryFields {
  MacEntryFields(
      folly::MacAddress mac,
      PortDescriptor portDescr,
      std::optional<cfg::AclLookupClass> classID = std::nullopt,
      MacEntryType type = MacEntryType::DYNAMIC_ENTRY)
      : mac_(mac), portDescr_(portDescr), classID_(classID), type_(type) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static MacEntryFields fromFollyDynamic(const folly::dynamic& json);

  folly::MacAddress mac_;
  PortDescriptor portDescr_;
  std::optional<cfg::AclLookupClass> classID_{std::nullopt};
  MacEntryType type_{MacEntryType::DYNAMIC_ENTRY};
};

class MacEntry : public NodeBaseT<MacEntry, MacEntryFields> {
 public:
  MacEntry(
      folly::MacAddress mac,
      PortDescriptor portDescr,
      std::optional<cfg::AclLookupClass> classID = std::nullopt,
      MacEntryType type = MacEntryType::DYNAMIC_ENTRY)
      : NodeBaseT(mac, portDescr, classID, type) {}

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

  bool operator==(const MacEntry& macEntry) const {
    return getFields()->mac_ == macEntry.getMac() &&
        getFields()->portDescr_ == macEntry.getPort() &&
        getFields()->classID_ == macEntry.getClassID() &&
        getFields()->type_ == macEntry.getType();
  }

  bool operator!=(const MacEntry& other) const {
    return !(*this == other);
  }

  folly::MacAddress getMac() const {
    return getFields()->mac_;
  }

  void setMac(folly::MacAddress mac) {
    this->writableFields()->mac_ = mac;
  }

  PortDescriptor getPort() const {
    return getFields()->portDescr_;
  }

  void setPort(PortDescriptor portDescr) {
    this->writableFields()->portDescr_ = portDescr;
  }

  std::optional<cfg::AclLookupClass> getClassID() const {
    return getFields()->classID_;
  }

  void setClassID(std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    this->writableFields()->classID_ = classID;
  }

  folly::MacAddress getID() const {
    return getMac();
  }

  void setPortDescriptor(PortDescriptor portDescr) {
    writableFields()->portDescr_ = portDescr;
  }

  MacEntryType getType() const {
    return getFields()->type_;
  }

  void setType(MacEntryType type) {
    writableFields()->type_ = type;
  }

  std::string str() const;

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
