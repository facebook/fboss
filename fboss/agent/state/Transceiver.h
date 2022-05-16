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

struct TransceiverFields
    : public BetterThriftyFields<TransceiverFields, state::TransceiverFields> {
  explicit TransceiverFields(TransceiverID id) {
    *data.id() = id;
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  state::TransceiverFields toThrift() const;
  static TransceiverFields fromThrift(
      state::TransceiverFields const& tcvrThrift);
  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);
  folly::dynamic toFollyDynamicLegacy() const;
  static TransceiverFields fromFollyDynamicLegacy(
      const folly::dynamic& tcvrJson);
};

/*
 * Transceiver stores state about one of the Present Transceiver entries on the
 * switch. Mainly use it as a reference to program Port.
 */
class Transceiver : public ThriftyBaseT<
                        state::TransceiverFields,
                        Transceiver,
                        TransceiverFields> {
 public:
  explicit Transceiver(TransceiverID id);
  static std::shared_ptr<Transceiver> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = TransceiverFields::fromFollyDynamicLegacy(json);
    return std::make_shared<Transceiver>(fields);
  }

  static std::shared_ptr<Transceiver> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  static std::shared_ptr<Transceiver> createPresentTransceiver(
      const TransceiverInfo& tcvrInfo);

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamicLegacy();
  }

  cfg::PlatformPortConfigOverrideFactor toPlatformPortConfigOverrideFactor()
      const;

  TransceiverID getID() const {
    return TransceiverID(*getFields()->data.id());
  }

  std::optional<double> getCableLength() const {
    return getFields()->data.cableLength().to_optional();
  }
  void setCableLength(double cableLength) {
    writableFields()->data.cableLength() = cableLength;
  }

  std::optional<MediaInterfaceCode> getMediaInterface() const {
    return getFields()->data.mediaInterface().to_optional();
  }
  void setMediaInterface(MediaInterfaceCode mediaInterface) {
    writableFields()->data.mediaInterface() = mediaInterface;
  }

  std::optional<TransceiverManagementInterface> getManagementInterface() const {
    return getFields()->data.managementInterface().to_optional();
  }
  void setManagementInterface(
      TransceiverManagementInterface managementInterface) {
    writableFields()->data.managementInterface() = managementInterface;
  }

  bool operator==(const Transceiver& tcvr) const;
  bool operator!=(const Transceiver& tcvr) const {
    return !(*this == tcvr);
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::TransceiverFields, Transceiver, TransceiverFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
