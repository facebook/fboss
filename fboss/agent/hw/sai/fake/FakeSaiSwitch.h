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

#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <folly/MacAddress.h>

#include <map>

extern "C" {
#include <sai.h>
#include "fboss/agent/hw/sai/api/fake/saifakeextensions.h"
}

namespace facebook::fboss {

class FakeSwitch {
 public:
  void setSrcMac(const sai_mac_t& mac) {
    folly::ByteRange r(std::begin(mac), std::end(mac));
    srcMac_ = folly::MacAddress::fromBinary(r);
  }
  folly::MacAddress srcMac() const {
    return srcMac_;
  }
  bool isInitialized() const {
    return inited_;
  }
  void setInitStatus(bool inited) {
    inited_ = inited;
  }
  void setShellStatus(bool enabled) {
    shellEnabled_ = enabled;
  }
  void setEcmpHashV4Id(const sai_object_id_t id) {
    ecmpHashV4_ = id;
  }
  void setEcmpHashV6Id(const sai_object_id_t id) {
    ecmpHashV6_ = id;
  }
  void setLagHashV4Id(const sai_object_id_t id) {
    lagHashV4_ = id;
  }
  void setLagHashV6Id(const sai_object_id_t id) {
    lagHashV6_ = id;
  }
  void setEcmpSeed(sai_uint32_t seed) {
    ecmpSeed_ = seed;
  }
  void setLagSeed(sai_uint32_t seed) {
    lagSeed_ = seed;
  }
  void setEcmpAlgorithm(sai_int32_t algorithm) {
    ecmpAlgorithm_ = algorithm;
  }
  void setLagAlgorithm(sai_int32_t algorithm) {
    lagAlgorithm_ = algorithm;
  }
  void setHwInfo(std::vector<int8_t> hwInfo) {
    hwInfo_ = std::move(hwInfo);
  }
  void setRestartWarm(bool warm) {
    restartWarm_ = warm;
  }
  void setFirmwarePathName(std::vector<int8_t> fwPath) {
    firmwarePathName_ = std::move(fwPath);
  }
  void setFirmwareLoadMethod(int32_t fwLoadMethod) {
    firmwareLoadMethod_ = fwLoadMethod;
  }
  void setFirmwareLoadType(int32_t firmwareLoadType) {
    firmwareLoadType_ = firmwareLoadType;
  }
  void setHardwareAccessBus(int32_t hardwareAccessBus) {
    hardwareAccessBus_ = hardwareAccessBus;
  }
  void setPlatformContext(sai_uint64_t platformContext) {
    platformContext_ = platformContext;
  }
  void setProfileId(sai_uint32_t profileId) {
    profileId_ = profileId;
  }
  void setSwitchId(sai_uint32_t switchId) {
    switchId_ = switchId;
  }
  void setMaxSystemCores(sai_uint32_t maxSystemCores) {
    maxSystemCores_ = maxSystemCores;
  }
  void setSysPortConfigList(
      std::vector<sai_system_port_config_t> sysPortConfigList) {
    sysPortConfigList_ = std::move(sysPortConfigList);
  }
  void setSwitchType(sai_uint32_t switchType) {
    switchType_ = switchType;
  }
  void setReadFn(sai_pointer_t readFn) {
    readFn_ = readFn;
  }
  void setWriteFn(sai_pointer_t writeFn) {
    writeFn_ = writeFn;
  }
  bool isShellEnabled() const {
    return shellEnabled_;
  }
  sai_object_id_t ecmpHashV4Id() const {
    return ecmpHashV4_;
  }
  sai_object_id_t ecmpHashV6Id() const {
    return ecmpHashV6_;
  }
  sai_object_id_t lagHashV4Id() const {
    return lagHashV4_;
  }
  sai_object_id_t lagHashV6Id() const {
    return lagHashV6_;
  }
  sai_uint32_t ecmpSeed() const {
    return ecmpSeed_;
  }
  sai_uint32_t lagSeed() const {
    return lagSeed_;
  }
  sai_int32_t ecmpAlgorithm() const {
    return ecmpAlgorithm_;
  }
  sai_int32_t lagAlgorithm() const {
    return lagAlgorithm_;
  }
  const std::vector<int8_t>& hwInfo() const {
    return hwInfo_;
  }
  int8_t* hwInfoData() {
    return hwInfo_.data();
  }
  bool restartWarm() const {
    return restartWarm_;
  }
  const std::vector<int8_t>& firmwarePath() const {
    return firmwarePathName_;
  }
  int8_t* firmwarePathData() {
    return firmwarePathName_.data();
  }
  int32_t firmwareLoadMethod() {
    return firmwareLoadMethod_;
  }
  int32_t firmwareLoadType() {
    return firmwareLoadType_;
  }
  int32_t hardwareAccessBus() {
    return hardwareAccessBus_;
  }
  sai_uint64_t platformContext() {
    return platformContext_;
  }
  sai_uint32_t profileId() {
    return profileId_;
  }
  int32_t switchId() {
    return switchId_;
  }
  int32_t maxSystemCores() {
    return maxSystemCores_;
  }
  const std::vector<sai_system_port_config_t>& sysPortConfigList() const {
    return sysPortConfigList_;
  }
  sai_system_port_config_t* sysPortConfigListData() {
    return sysPortConfigList_.data();
  }
  int32_t switchType() {
    return switchType_;
  }
  sai_pointer_t readFn() {
    return readFn_;
  }
  sai_pointer_t writeFn() {
    return writeFn_;
  }
  void setDscpToTc(sai_object_id_t oid) {
    qosMapDscpToTc_ = oid;
  }
  void setTcToQueue(sai_object_id_t oid) {
    qosMapTcToQueue_ = oid;
  }
  void setExpToTc(sai_object_id_t oid) {
    qosMapExpToTc_ = oid;
  }
  void setTcToExp(sai_object_id_t oid) {
    qosMapTcToExp_ = oid;
  }
  sai_object_id_t dscpToTc() const {
    return qosMapDscpToTc_;
  }
  sai_object_id_t tcToQueue() const {
    return qosMapTcToQueue_;
  }
  sai_object_id_t expToTc() const {
    return qosMapExpToTc_;
  }
  sai_object_id_t tcToExp() const {
    return qosMapTcToExp_;
  }
  sai_uint32_t getMacAgingTime() const {
    return macAgingTime_;
  }
  void setMacAgingTime(sai_uint32_t time) {
    macAgingTime_ = time;
  }
  void setUseEcnThresholds(bool enabled) {
    UseEcnThresholds_ = enabled;
  }
  bool getUseEcnThresholds() {
    return UseEcnThresholds_;
  }
  sai_object_id_t getIngressAcl() {
    return ingressAcl_;
  }
  void setIngressAcl(sai_object_id_t oid) {
    ingressAcl_ = oid;
  }
  sai_uint32_t getCounterRefreshInterval() {
    return counterRefreshInterval_;
  }
  void setCounterRefreshInterval(sai_uint32_t interval) {
    counterRefreshInterval_ = interval;
  }

  void setDefaultVlanId(sai_object_id_t id) {
    defaultVlanId_ = id;
  }

  sai_object_id_t getDefaultVlanId() const {
    return defaultVlanId_;
  }

  sai_uint32_t getMaxEcmpMemberCount() const {
    return maxEcmpMemberCount_;
  }

  sai_uint32_t getEcmpMemberCount() const {
    return ecmpMemberCount_;
  }

  void setEcmpMemberCount(sai_uint32_t count) {
    ecmpMemberCount_ = count;
  }

  void setCreditWatchdogEnable(bool enable) {
    creditWatchDogEnable_ = enable;
  }

  bool getCreditWatchdogEnable() const {
    return creditWatchDogEnable_;
  }

  void setPfcDlrPacketAction(sai_packet_action_t pktAction) {
    pfcDlrPacketAction_ = pktAction;
  }

  sai_packet_action_t getPfcDlrPacketAction() const {
    return pfcDlrPacketAction_;
  }

  sai_object_id_t id;

  sai_status_t setLed(const sai_attribute_t* attr);
  sai_status_t getLed(sai_attribute_t* attr) const;

 private:
  folly::MacAddress srcMac_;
  bool inited_{false};
  bool shellEnabled_{false};
  sai_uint32_t ecmpSeed_{0};
  sai_uint32_t lagSeed_{0};
  sai_int32_t ecmpAlgorithm_{SAI_HASH_ALGORITHM_CRC};
  sai_int32_t lagAlgorithm_{SAI_HASH_ALGORITHM_CRC};
  sai_object_id_t ecmpHashV4_{SAI_NULL_OBJECT_ID};
  sai_object_id_t ecmpHashV6_{SAI_NULL_OBJECT_ID};
  sai_object_id_t lagHashV4_{SAI_NULL_OBJECT_ID};
  sai_object_id_t lagHashV6_{SAI_NULL_OBJECT_ID};
  sai_object_id_t qosMapDscpToTc_{SAI_NULL_OBJECT_ID};
  sai_object_id_t qosMapTcToQueue_{SAI_NULL_OBJECT_ID};
  sai_object_id_t qosMapExpToTc_{SAI_NULL_OBJECT_ID};
  sai_object_id_t qosMapTcToExp_{SAI_NULL_OBJECT_ID};
  std::vector<int8_t> hwInfo_;
  bool restartWarm_{false};
  sai_uint32_t macAgingTime_{0};
  bool UseEcnThresholds_{false};
  sai_object_id_t ingressAcl_{SAI_NULL_OBJECT_ID};
  struct FakeSwitchLedState {
    bool reset{};
    std::array<uint8_t, 256> program{};
    std::map<sai_uint32_t, sai_uint32_t> data{};
  };
  std::map<int, FakeSwitchLedState> ledState_{{0, {}}, {1, {}}};
  sai_uint32_t counterRefreshInterval_{1};
  std::vector<int8_t> firmwarePathName_;
  int32_t firmwareLoadMethod_;
  int32_t firmwareLoadType_;
  int32_t hardwareAccessBus_;
  sai_uint64_t platformContext_;
  sai_uint32_t profileId_;
  sai_uint32_t switchId_;
  sai_uint32_t maxSystemCores_;
  std::vector<sai_system_port_config_t> sysPortConfigList_;
  sai_uint32_t switchType_;
  sai_pointer_t readFn_;
  sai_pointer_t writeFn_;
  sai_object_id_t defaultVlanId_;
  sai_uint32_t maxEcmpMemberCount_{4096};
  sai_uint32_t ecmpMemberCount_{64};
  bool creditWatchDogEnable_{true};
  sai_packet_action_t pfcDlrPacketAction_{SAI_PACKET_ACTION_DROP};
};

using FakeSwitchManager = FakeManager<sai_object_id_t, FakeSwitch>;

void populate_switch_api(sai_switch_api_t** switch_api);

} // namespace facebook::fboss
