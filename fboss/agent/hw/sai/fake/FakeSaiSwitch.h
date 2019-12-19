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

extern "C" {
#include <sai.h>
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
  sai_object_id_t id;

 private:
  folly::MacAddress srcMac_;
  bool inited_{false};
  bool shellEnabled_{false};
  sai_uint32_t ecmpSeed_{0};
  sai_uint32_t lagSeed_{0};
  sai_int32_t ecmpAlgorithm_{SAI_HASH_ALGORITHM_CRC};
  sai_int32_t lagAlgorithm_{SAI_HASH_ALGORITHM_CRC};
  sai_object_id_t ecmpHashV4_{0};
  sai_object_id_t ecmpHashV6_{0};
  std::vector<int8_t> hwInfo_;
};

using FakeSwitchManager = FakeManager<sai_object_id_t, FakeSwitch>;

void populate_switch_api(sai_switch_api_t** switch_api);

} // namespace facebook::fboss
