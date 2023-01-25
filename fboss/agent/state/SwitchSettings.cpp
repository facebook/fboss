/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/SwitchSettings.h"
#include <fboss/agent/gen-cpp2/switch_config_types.h>
#include "common/network/if/gen-cpp2/Address_types.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/state/NodeBase-defs.h"
#include "folly/dynamic.h"
#include "folly/json.h"

namespace {
constexpr auto kL2LearningMode = "l2LearningMode";
constexpr auto kQcmEnable = "qcmEnable";
constexpr auto kPtpTcEnable = "ptpTcEnable";
constexpr auto kL2AgeTimerSeconds = "l2AgeTimerSeconds";
constexpr auto kMaxRouteCounterIDs = "maxRouteCounterIDs";
constexpr auto kBlockNeighbors = "blockNeighbors";
constexpr auto kBlockNeighborVlanID = "blockNeighborVlanID";
constexpr auto kBlockNeighborIP = "blockNeighborIP";
constexpr auto kMacAddrsToBlock = "macAddrsToBlock";
constexpr auto kMacAddrToBlockVlanID = "macAddrToBlockVlanID";
constexpr auto kMacAddrToBlockAddr = "macAddrToBlockAddr";
constexpr auto kSwitchType = "switchType";
constexpr auto kSwitchId = "switchId";
constexpr auto kExactMatchTableConfigs = "exactMatchTableConfigs";
constexpr auto kExactMatchTableName = "name";
constexpr auto kExactMatchTableDstPrefixLength = "dstPrefixLength";

} // namespace

namespace facebook::fboss {

folly::dynamic SwitchSettingsFields::toFollyDynamicLegacy() const {
  folly::dynamic switchSettings = folly::dynamic::object;

  switchSettings[kL2LearningMode] = static_cast<int>(l2LearningMode);
  switchSettings[kQcmEnable] = static_cast<bool>(qcmEnable);
  switchSettings[kPtpTcEnable] = static_cast<bool>(ptpTcEnable);
  switchSettings[kL2AgeTimerSeconds] = static_cast<int>(l2AgeTimerSeconds);
  switchSettings[kMaxRouteCounterIDs] = static_cast<int>(maxRouteCounterIDs);

  switchSettings[kBlockNeighbors] = folly::dynamic::array;
  for (const auto& [vlanID, ipAddress] : blockNeighbors) {
    folly::dynamic jsonEntry = folly::dynamic::object;
    jsonEntry[kBlockNeighborVlanID] = folly::to<std::string>(vlanID);
    jsonEntry[kBlockNeighborIP] = folly::to<std::string>(ipAddress);
    switchSettings[kBlockNeighbors].push_back(jsonEntry);
  }

  switchSettings[kMacAddrsToBlock] = folly::dynamic::array;
  for (const auto& [vlanID, macAddr] : macAddrsToBlock) {
    folly::dynamic jsonEntry = folly::dynamic::object;
    jsonEntry[kMacAddrToBlockVlanID] = folly::to<std::string>(vlanID);
    jsonEntry[kMacAddrToBlockAddr] = folly::to<std::string>(macAddr);
    switchSettings[kMacAddrsToBlock].push_back(jsonEntry);
  }
  switchSettings[kSwitchType] = static_cast<int>(switchType);
  if (switchId) {
    switchSettings[kSwitchId] = *switchId;
  }

  switchSettings[kExactMatchTableConfigs] = folly::dynamic::array;
  for (const auto& exactMatchTableConfig : exactMatchTableConfigs) {
    folly::dynamic jsonEntry = folly::dynamic::object;
    jsonEntry[kExactMatchTableName] = *exactMatchTableConfig.name();
    if (exactMatchTableConfig.dstPrefixLength()) {
      jsonEntry[kExactMatchTableDstPrefixLength] =
          *exactMatchTableConfig.dstPrefixLength();
    }
    switchSettings[kExactMatchTableConfigs].push_back(jsonEntry);
  }

  return switchSettings;
}

SwitchSettingsFields SwitchSettingsFields::fromFollyDynamicLegacy(
    const folly::dynamic& json) {
  SwitchSettingsFields switchSettings = SwitchSettingsFields();

  if (json.find(kL2LearningMode) != json.items().end()) {
    switchSettings.l2LearningMode =
        cfg::L2LearningMode(json[kL2LearningMode].asInt());
  }
  if (json.find(kQcmEnable) != json.items().end()) {
    switchSettings.qcmEnable = json[kQcmEnable].asBool();
  }
  if (json.find(kPtpTcEnable) != json.items().end()) {
    switchSettings.ptpTcEnable = json[kPtpTcEnable].asBool();
  }
  if (json.find(kL2AgeTimerSeconds) != json.items().end()) {
    switchSettings.l2AgeTimerSeconds = json[kL2AgeTimerSeconds].asInt();
  }
  if (json.find(kMaxRouteCounterIDs) != json.items().end()) {
    switchSettings.maxRouteCounterIDs = json[kMaxRouteCounterIDs].asInt();
  }

  if (json.find(kBlockNeighbors) != json.items().end()) {
    for (const auto& entry : json[kBlockNeighbors]) {
      switchSettings.blockNeighbors.emplace_back(
          entry[kBlockNeighborVlanID].asInt(),
          entry[kBlockNeighborIP].asString());
    }
  }

  if (json.find(kMacAddrsToBlock) != json.items().end()) {
    for (const auto& entry : json[kMacAddrsToBlock]) {
      switchSettings.macAddrsToBlock.emplace_back(
          entry[kMacAddrToBlockVlanID].asInt(),
          entry[kMacAddrToBlockAddr].asString());
    }
  }
  if (json.find(kSwitchType) != json.items().end()) {
    switchSettings.switchType =
        static_cast<cfg::SwitchType>(json[kSwitchType].asInt());
  }
  if (json.find(kSwitchId) != json.items().end()) {
    switchSettings.switchId = json[kSwitchId].asInt();
  }
  if (json.find(kExactMatchTableConfigs) != json.items().end()) {
    for (const auto& entry : json[kExactMatchTableConfigs]) {
      cfg::ExactMatchTableConfig tableConfig;
      tableConfig.name() = entry[kExactMatchTableName].asString();
      if (entry.find(kExactMatchTableDstPrefixLength) != entry.items().end()) {
        tableConfig.dstPrefixLength() =
            entry[kExactMatchTableDstPrefixLength].asInt();
      }
      switchSettings.exactMatchTableConfigs.emplace_back(tableConfig);
    }
  }

  return switchSettings;
}

SwitchSettings* SwitchSettings::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newSwitchSettings = clone();
  auto* ptr = newSwitchSettings.get();
  (*state)->resetSwitchSettings(std::move(newSwitchSettings));
  return ptr;
}

state::SwitchSettingsFields SwitchSettingsFields::toThrift() const {
  state::SwitchSettingsFields thriftFields{};
  thriftFields.l2LearningMode() = l2LearningMode;
  thriftFields.qcmEnable() = qcmEnable;
  thriftFields.ptpTcEnable() = ptpTcEnable;
  thriftFields.l2AgeTimerSeconds() = l2AgeTimerSeconds;
  thriftFields.maxRouteCounterIDs() = maxRouteCounterIDs;
  for (auto neighbor : blockNeighbors) {
    auto [vlan, ip] = neighbor;
    state::BlockedNeighbor blockedNeighbor;
    blockedNeighbor.blockNeighborVlanID() = vlan;
    blockedNeighbor.blockNeighborIP() = facebook::network::toBinaryAddress(ip);
    thriftFields.blockNeighbors()->push_back(blockedNeighbor);
  }
  for (auto macAddr : macAddrsToBlock) {
    auto [vlan, mac] = macAddr;
    state::BlockedMacAddress blockedMac;
    blockedMac.macAddrToBlockVlanID() = vlan;
    blockedMac.macAddrToBlockAddr() = mac.toString();
    thriftFields.macAddrsToBlock()->push_back(blockedMac);
  }
  thriftFields.switchType() = switchType;
  if (switchId) {
    thriftFields.switchId() = *switchId;
  }
  thriftFields.exactMatchTableConfigs() = exactMatchTableConfigs;
  if (systemPortRange) {
    thriftFields.systemPortRange() = *systemPortRange;
  }
  return thriftFields;
}

SwitchSettingsFields SwitchSettingsFields::fromThrift(
    state::SwitchSettingsFields const& fields) {
  SwitchSettingsFields settings{};
  settings.l2LearningMode = *fields.l2LearningMode();
  settings.qcmEnable = *fields.qcmEnable();
  settings.ptpTcEnable = *fields.ptpTcEnable();
  settings.l2AgeTimerSeconds = *fields.l2AgeTimerSeconds();
  settings.maxRouteCounterIDs = *fields.maxRouteCounterIDs();
  for (auto blockedNeighbor : *fields.blockNeighbors()) {
    auto ip =
        facebook::network::toIPAddress(*blockedNeighbor.blockNeighborIP());
    auto vlan = static_cast<VlanID>(*blockedNeighbor.blockNeighborVlanID());
    settings.blockNeighbors.push_back(std::make_pair(vlan, ip));
  }
  for (auto macAddr : *fields.macAddrsToBlock()) {
    auto mac = folly::MacAddress(*macAddr.macAddrToBlockAddr());
    auto vlan = static_cast<VlanID>(*macAddr.macAddrToBlockVlanID());
    settings.macAddrsToBlock.push_back(std::make_pair(vlan, mac));
  }
  settings.switchType = *fields.switchType();
  if (fields.switchId()) {
    settings.switchId = *fields.switchId();
  }
  settings.exactMatchTableConfigs = *fields.exactMatchTableConfigs();
  if (fields.systemPortRange()) {
    settings.systemPortRange = *fields.systemPortRange();
  }

  return settings;
}

folly::dynamic SwitchSettingsFields::migrateToThrifty(
    folly::dynamic const& dynLegacy) {
  folly::dynamic newDynamic = dynLegacy;

  folly::dynamic blockedNeighborsDynamic = folly::dynamic::array;
  for (auto neighborDynLegacy : dynLegacy["blockNeighbors"]) {
    auto addr = facebook::network::toBinaryAddress(
        folly::IPAddress(neighborDynLegacy["blockNeighborIP"].asString()));

    std::string jsonStr;
    apache::thrift::SimpleJSONSerializer::serialize(addr, &jsonStr);
    folly::dynamic neighborDyn = folly::dynamic::object;
    neighborDyn["blockNeighborIP"] = folly::parseJson(jsonStr);
    neighborDyn["blockNeighborVlanID"] =
        static_cast<int16_t>(neighborDynLegacy["blockNeighborVlanID"].asInt());

    blockedNeighborsDynamic.push_back(neighborDyn);
  }
  newDynamic["blockNeighbors"] = blockedNeighborsDynamic;

  folly::dynamic macAddrsToBlockDynamic = folly::dynamic::array;
  if (dynLegacy.find("macAddrsToBlock") != dynLegacy.items().end()) {
    for (auto macDynLegacy : dynLegacy["macAddrsToBlock"]) {
      folly::dynamic macDyn = folly::dynamic::object;

      macDyn["macAddrToBlockVlanID"] =
          static_cast<int16_t>(macDynLegacy["macAddrToBlockVlanID"].asInt());
      macDyn["macAddrToBlockAddr"] =
          macDynLegacy["macAddrToBlockAddr"].asString();
      macAddrsToBlockDynamic.push_back(macDyn);
    }
  }

  newDynamic["macAddrsToBlock"] = macAddrsToBlockDynamic;
  return newDynamic;
}

void SwitchSettingsFields::migrateFromThrifty(folly::dynamic& dyn) {
  folly::dynamic& blockedNeighborsDynamic = dyn["blockNeighbors"];
  folly::dynamic& blockedMacAddrsDynamic = dyn["macAddrsToBlock"];

  folly::dynamic blockedNeighborsLegacy = folly::dynamic::array;
  for (auto& neighborDyn : blockedNeighborsDynamic) {
    auto vlan =
        static_cast<uint16_t>(neighborDyn["blockNeighborVlanID"].asInt());
    auto jsonStr = folly::toJson(neighborDyn["blockNeighborIP"]);
    auto inBuf =
        folly::IOBuf::wrapBufferAsValue(jsonStr.data(), jsonStr.size());
    auto addr = facebook::network::toIPAddress(
        apache::thrift::SimpleJSONSerializer::deserialize<
            facebook::network::thrift::BinaryAddress>(
            folly::io::Cursor{&inBuf}));

    folly::dynamic blockedNeighborLegacy = folly::dynamic::object;
    neighborDyn["blockNeighborIP"] = addr.str();
    neighborDyn["blockNeighborVlanID"] = vlan;
  }

  for (auto& macDyn : blockedMacAddrsDynamic) {
    auto vlan = static_cast<uint16_t>(macDyn["macAddrToBlockVlanID"].asInt());
    macDyn["macAddrToBlockVlanID"] = vlan;
  }

  // Temporarily not dumping this field in folly dynamic. See
  // https://fb.workplace.com/groups/1015730552263827/permalink/1452005081969703/
  // for more details.
  // TODO(zecheng): remove this after the config is available in prod.
  auto exactMatchSP = folly::StringPiece("exactMatchTableConfigs");
  if (dyn.find(exactMatchSP) != dyn.items().end()) {
    dyn.erase(exactMatchSP);
  }
}

template class ThriftStructNode<SwitchSettings, state::SwitchSettingsFields>;

} // namespace facebook::fboss
