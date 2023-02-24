/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/Transceiver.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/NodeBase-defs.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {

namespace {
constexpr auto kTransceiverID = "id";
constexpr auto kCableLength = "cableLength";
constexpr auto kMediaInterface = "mediaInterface";
constexpr auto kManagementInterface = "managementInterface";
} // namespace

TransceiverSpec::TransceiverSpec(TransceiverID id) {
  set<switch_state_tags::id>(id);
}

std::shared_ptr<TransceiverSpec> TransceiverSpec::createPresentTransceiver(
    const TransceiverInfo& tcvrInfo) {
  std::shared_ptr<TransceiverSpec> newTransceiver;
  if (*tcvrInfo.present()) {
    newTransceiver =
        std::make_shared<TransceiverSpec>(TransceiverID(*tcvrInfo.port()));
    if (tcvrInfo.cable() && tcvrInfo.cable()->length()) {
      newTransceiver->setCableLength(*tcvrInfo.cable()->length());
    }
    if (auto settings = tcvrInfo.settings();
        settings && settings->mediaInterface()) {
      if (settings->mediaInterface()->size() == 0) {
        XLOG(WARNING) << "Missing media interface, skip setting it.";
      } else {
        const auto& interface = (*settings->mediaInterface())[0];
        newTransceiver->setMediaInterface(*interface.code());
      }
    }
    if (auto interface = tcvrInfo.transceiverManagementInterface()) {
      newTransceiver->setManagementInterface(*interface);
    }
  }
  return newTransceiver;
}

cfg::PlatformPortConfigOverrideFactor
TransceiverSpec::toPlatformPortConfigOverrideFactor() const {
  cfg::PlatformPortConfigOverrideFactor factor;
  if (auto cableLength = getCableLength()) {
    factor.cableLengths() = {*cableLength};
  }
  if (auto mediaInterface = getMediaInterface()) {
    factor.mediaInterfaceCode() = *mediaInterface;
  }
  if (auto managerInterface = getManagementInterface()) {
    factor.transceiverManagementInterface() = *managerInterface;
  }
  return factor;
}

template class ThriftStructNode<TransceiverSpec, state::TransceiverSpecFields>;
} // namespace facebook::fboss
