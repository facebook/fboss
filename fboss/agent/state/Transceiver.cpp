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

folly::dynamic TransceiverFields::toFollyDynamic() const {
  folly::dynamic tcvr = folly::dynamic::object;
  tcvr[kTransceiverID] = static_cast<uint32_t>(id);
  if (cableLength) {
    tcvr[kCableLength] = *cableLength;
  }
  if (mediaInterface) {
    auto interface = apache::thrift::util::enumName(*mediaInterface);
    if (interface == nullptr) {
      throw FbossError("Invalid MediaInterface for Transceiver:", id);
    }
    tcvr[kMediaInterface] = interface;
  }
  if (managementInterface) {
    auto interface = apache::thrift::util::enumName(*managementInterface);
    if (interface == nullptr) {
      throw FbossError("Invalid ManagementInterface for Transceiver:", id);
    }
    tcvr[kManagementInterface] = interface;
  }
  return tcvr;
}

TransceiverFields TransceiverFields::fromFollyDynamic(
    const folly::dynamic& tcvrJson) {
  TransceiverFields tcvr(TransceiverID(tcvrJson[kTransceiverID].asInt()));
  if (const auto& value = tcvrJson.find(kCableLength);
      value != tcvrJson.items().end()) {
    tcvr.cableLength = value->second.asDouble();
  }
  if (const auto& value = tcvrJson.find(kMediaInterface);
      value != tcvrJson.items().end()) {
    MediaInterfaceCode interface;
    apache::thrift::TEnumTraits<MediaInterfaceCode>::findValue(
        value->second.asString().c_str(), &interface);
    tcvr.mediaInterface = interface;
  }
  if (const auto& value = tcvrJson.find(kManagementInterface);
      value != tcvrJson.items().end()) {
    TransceiverManagementInterface interface;
    apache::thrift::TEnumTraits<TransceiverManagementInterface>::findValue(
        value->second.asString().c_str(), &interface);
    tcvr.managementInterface = interface;
  }
  return tcvr;
}

Transceiver::Transceiver(TransceiverID id) : NodeBaseT(id) {}

std::shared_ptr<Transceiver> Transceiver::createPresentTransceiver(
    const TransceiverInfo& tcvrInfo) {
  std::shared_ptr<Transceiver> newTransceiver;
  if (*tcvrInfo.present()) {
    newTransceiver =
        std::make_shared<Transceiver>(TransceiverID(*tcvrInfo.port()));
    if (tcvrInfo.cable() && tcvrInfo.cable()->length()) {
      newTransceiver->setCableLength(*tcvrInfo.cable()->length());
    }
    if (auto settings = tcvrInfo.settings();
        settings && settings->mediaInterface()) {
      const auto& interface = (*settings->mediaInterface())[0];
      newTransceiver->setMediaInterface(*interface.code());
    }
    if (auto interface = tcvrInfo.transceiverManagementInterface()) {
      newTransceiver->setManagementInterface(*interface);
    }
  }
  return newTransceiver;
}

bool Transceiver::operator==(const Transceiver& tcvr) const {
  return getID() == tcvr.getID() && getCableLength() == tcvr.getCableLength() &&
      getMediaInterface() == tcvr.getMediaInterface() &&
      getManagementInterface() == tcvr.getManagementInterface();
}

cfg::PlatformPortConfigOverrideFactor
Transceiver::toPlatformPortConfigOverrideFactor() const {
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

template class NodeBaseT<Transceiver, TransceiverFields>;
} // namespace facebook::fboss
