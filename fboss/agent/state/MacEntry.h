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
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace facebook::fboss {

enum class MacEntryType : uint8_t { DYNAMIC_ENTRY, STATIC_ENTRY };

struct MacEntryFields
    : public ThriftyFields<MacEntryFields, state::MacEntryFields> {
  MacEntryFields(
      folly::MacAddress mac,
      PortDescriptor portDescr,
      std::optional<cfg::AclLookupClass> classID = std::nullopt,
      MacEntryType type = MacEntryType::DYNAMIC_ENTRY)
      : mac_(mac), portDescr_(portDescr), classID_(classID), type_(type) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  state::MacEntryFields toThrift() const override;
  static MacEntryFields fromThrift(state::MacEntryFields const& ma);

  folly::dynamic toFollyDynamicLegacy() const;
  static MacEntryFields fromFollyDynamicLegacy(const folly::dynamic& json);

  folly::MacAddress mac_;
  PortDescriptor portDescr_;
  std::optional<cfg::AclLookupClass> classID_{std::nullopt};
  MacEntryType type_{MacEntryType::DYNAMIC_ENTRY};
};

USE_THRIFT_COW(MacEntry);

class MacEntry : public ThriftStructNode<MacEntry, state::MacEntryFields> {
 public:
  using LegacyFields = MacEntryFields;
  using Base = ThriftStructNode<MacEntry, state::MacEntryFields>;
  MacEntry(
      folly::MacAddress mac,
      PortDescriptor portDescr,
      std::optional<cfg::AclLookupClass> classID = std::nullopt,
      MacEntryType type = MacEntryType::DYNAMIC_ENTRY) {
    setMac(mac);
    setPort(portDescr);
    setClassID(classID);
    setType(type);
  }

  static std::shared_ptr<MacEntry> fromFollyDynamicLegacy(
      const folly::dynamic& json) {
    auto macEntry = std::make_shared<MacEntry>();
    const auto& fields = MacEntryFields::fromFollyDynamicLegacy(json);
    macEntry->fromThrift(fields.toThrift());
    return macEntry;
  }

  folly::dynamic toFollyDynamicLegacy() const {
    auto fields = MacEntryFields::fromThrift(toThrift());
    return fields.toFollyDynamicLegacy();
  }

  folly::dynamic toFollyDynamic() const override {
    auto fields = MacEntryFields::fromThrift(toThrift());
    return fields.toFollyDynamic();
  }

  static std::shared_ptr<MacEntry> fromFollyDynamic(
      const folly::dynamic& json) {
    auto macEntry = std::make_shared<MacEntry>();
    auto fields = MacEntryFields::fromFollyDynamic(json);
    macEntry->fromThrift(fields.toThrift());
    return macEntry;
  }

  bool operator!=(const MacEntry& other) const {
    return !(*this == other);
  }

  folly::MacAddress getMac() const {
    return folly::MacAddress(get<switch_state_tags::mac>()->cref());
  }

  void setMac(folly::MacAddress mac) {
    set<switch_state_tags::mac>(mac.toString());
  }

  PortDescriptor getPort() const {
    return PortDescriptor::fromThrift(
        get<switch_state_tags::portId>()->toThrift());
  }

  void setPort(PortDescriptor portDescr) {
    set<switch_state_tags::portId>(portDescr.toThrift());
  }

  std::optional<cfg::AclLookupClass> getClassID() const {
    if (auto classID = get<switch_state_tags::classID>()) {
      return classID->cref();
    }
    return std::nullopt;
  }

  void setClassID(std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    if (classID) {
      set<switch_state_tags::classID>(classID.value());
    } else {
      ref<switch_state_tags::classID>().reset();
    }
  }

  std::string getID() const {
    return getMac().toString();
  }

  void setPortDescriptor(PortDescriptor portDescr) {
    setPort(portDescr);
  }

  MacEntryType getType() const {
    return static_cast<MacEntryType>(get<switch_state_tags::type>()->cref());
  }

  void setType(MacEntryType type) {
    set<switch_state_tags::type>(static_cast<state::MacEntryType>(type));
  }

  std::string str() const;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
