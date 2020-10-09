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

#include "FakeManager.h"

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
  bool isShellEnabled() const {
    return shellEnabled_;
  }
  sai_object_id_t ecmpHashV4Id() const {
    return ecmpHashV4_;
  }
  sai_object_id_t ecmpHashV6Id() const {
    return ecmpHashV6_;
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
  void setDscpToTc(sai_object_id_t oid) {
    qosMapDscpToTc_ = oid;
  }
  void setTcToQueue(sai_object_id_t oid) {
    qosMapTcToQueue_ = oid;
  }
  sai_object_id_t dscpToTc() const {
    return qosMapDscpToTc_;
  }
  sai_object_id_t tcToQueue() const {
    return qosMapTcToQueue_;
  }
  sai_uint32_t getMacAgingTime() const {
    return macAgingTime_;
  }
  void setMacAgingTime(sai_uint32_t time) {
    macAgingTime_ = time;
  }
  void setEcnEctThresholdEnable(bool enabled) {
    ecnEctThresholdEnable_ = enabled;
  }
  bool getEcnEctThresholdEnable() {
    return ecnEctThresholdEnable_;
  }
  sai_object_id_t getIngressAcl() {
    return ingressAcl_;
  }
  void setIngressAcl(sai_object_id_t oid) {
    ingressAcl_ = oid;
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
  sai_object_id_t qosMapDscpToTc_{SAI_NULL_OBJECT_ID};
  sai_object_id_t qosMapTcToQueue_{SAI_NULL_OBJECT_ID};
  std::vector<int8_t> hwInfo_;
  bool restartWarm_{false};
  sai_uint32_t macAgingTime_{0};
  bool ecnEctThresholdEnable_{false};
  sai_object_id_t ingressAcl_{SAI_NULL_OBJECT_ID};
  struct FakeSwitchLedState {
    bool reset{};
    std::array<uint8_t, 256> program{};
    std::map<sai_uint32_t, sai_uint32_t> data{};
  };
  std::map<int, FakeSwitchLedState> ledState_{{0, {}}, {1, {}}};
};

using FakeSwitchManager = FakeManager<sai_object_id_t, FakeSwitch>;

void populate_switch_api(sai_switch_api_t** switch_api);

} // namespace facebook::fboss
