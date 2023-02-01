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
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook::fboss {

struct TransceiverSpecFields : public ThriftyFields<
                                   TransceiverSpecFields,
                                   state::TransceiverSpecFields> {
  explicit TransceiverSpecFields(TransceiverID id) {
    auto& data = writableData();
    *data.id() = id;
  }

  bool operator==(const TransceiverSpecFields& other) const {
    return data() == other.data();
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  state::TransceiverSpecFields toThrift() const override;
  static TransceiverSpecFields fromThrift(
      state::TransceiverSpecFields const& tcvrThrift);
  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);
  folly::dynamic toFollyDynamicLegacy() const;
  static TransceiverSpecFields fromFollyDynamicLegacy(
      const folly::dynamic& tcvrJson);
};

USE_THRIFT_COW(TransceiverSpec)
/*
 * TransceiverSpec stores state about one of the Present TransceiverSpec entries
 * on the switch. Mainly use it as a reference to program Port.
 */
class TransceiverSpec
    : public ThriftStructNode<TransceiverSpec, state::TransceiverSpecFields> {
 public:
  using LegacyFields = TransceiverSpecFields;
  using Base = ThriftStructNode<TransceiverSpec, state::TransceiverSpecFields>;
  explicit TransceiverSpec(TransceiverID id);
  static std::shared_ptr<TransceiverSpec> createPresentTransceiver(
      const TransceiverInfo& tcvrInfo);

  cfg::PlatformPortConfigOverrideFactor toPlatformPortConfigOverrideFactor()
      const;

  TransceiverID getID() const {
    return static_cast<TransceiverID>(get<switch_state_tags::id>()->cref());
  }

  std::optional<double> getCableLength() const {
    if (auto cableLength = get<switch_state_tags::cableLength>()) {
      return cableLength->cref();
    }
    return std::nullopt;
  }
  void setCableLength(double cableLength) {
    set<switch_state_tags::cableLength>(cableLength);
  }

  std::optional<MediaInterfaceCode> getMediaInterface() const {
    if (auto mediaInterface = get<switch_state_tags::mediaInterface>()) {
      return mediaInterface->cref();
    }
    return std::nullopt;
  }
  void setMediaInterface(MediaInterfaceCode mediaInterface) {
    set<switch_state_tags::mediaInterface>(mediaInterface);
  }

  std::optional<TransceiverManagementInterface> getManagementInterface() const {
    if (auto interface = get<switch_state_tags::managementInterface>()) {
      return interface->cref();
    }
    return std::nullopt;
  }
  void setManagementInterface(
      TransceiverManagementInterface managementInterface) {
    set<switch_state_tags::managementInterface>(managementInterface);
  }

  static std::shared_ptr<TransceiverSpec> fromFollyDynamic(
      const folly::dynamic& dyn) {
    auto fields = LegacyFields::fromFollyDynamic(dyn);
    auto obj = fields.toThrift();
    return std::make_shared<TransceiverSpec>(std::move(obj));
  }

  static std::shared_ptr<TransceiverSpec> fromFollyDynamicLegacy(
      const folly::dynamic& dyn) {
    auto fields = LegacyFields::fromFollyDynamicLegacy(dyn);
    auto obj = fields.toThrift();
    return std::make_shared<TransceiverSpec>(std::move(obj));
  }

  folly::dynamic toFollyDynamic() const override {
    auto fields = LegacyFields::fromThrift(this->toThrift());
    return fields.toFollyDynamic();
  }

  folly::dynamic toFollyDynamicLegacy() const {
    auto fields = LegacyFields::fromThrift(this->toThrift());
    return fields.toFollyDynamicLegacy();
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
