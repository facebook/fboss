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

#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook::fboss {

struct TransceiverFields {
  explicit TransceiverFields(TransceiverID id) : id(id) {}

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  folly::dynamic toFollyDynamic() const;
  static TransceiverFields fromFollyDynamic(const folly::dynamic& tcvrJson);

  const TransceiverID id{0};
  // Right now, we just need to store factors that we actually need for
  // getting the port config to program ports
  std::optional<double> cableLength;
  std::optional<MediaInterfaceCode> mediaInterface;
  std::optional<TransceiverManagementInterface> managementInterface;
};

/*
 * Transceiver stores state about one of the Present Transceiver entries on the
 * switch. Mainly use it as a reference to program Port.
 */
class Transceiver : public NodeBaseT<Transceiver, TransceiverFields> {
 public:
  explicit Transceiver(TransceiverID id);
  static std::shared_ptr<Transceiver> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = TransceiverFields::fromFollyDynamic(json);
    return std::make_shared<Transceiver>(fields);
  }

  static std::shared_ptr<Transceiver> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  static std::shared_ptr<Transceiver> createPresentTransceiver(
      const TransceiverInfo& tcvrInfo);

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  TransceiverID getID() const {
    return getFields()->id;
  }

  std::optional<double> getCableLength() const {
    return getFields()->cableLength;
  }
  void setCableLength(double cableLength) {
    writableFields()->cableLength = cableLength;
  }

  std::optional<MediaInterfaceCode> getMediaInterface() const {
    return getFields()->mediaInterface;
  }
  void setMediaInterface(MediaInterfaceCode mediaInterface) {
    writableFields()->mediaInterface = mediaInterface;
  }

  std::optional<TransceiverManagementInterface> getManagementInterface() const {
    return getFields()->managementInterface;
  }
  void setManagementInterface(
      TransceiverManagementInterface managementInterface) {
    writableFields()->managementInterface = managementInterface;
  }

  bool operator==(const Transceiver& tcvr) const;
  bool operator!=(const Transceiver& tcvr) const {
    return !(*this == tcvr);
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
