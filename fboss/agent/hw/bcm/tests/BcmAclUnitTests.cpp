/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <optional>
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/types.h"

using facebook::fboss::utility::isBcmQualFieldStateSame;
using facebook::fboss::utility::isBcmQualFieldWithMaskStateSame;
using folly::IPAddress;

namespace {

const uint32_t kDefaultData = 100;
const uint32_t kMismatchData = 101;
const uint32_t kDefaultMask = 0xFF;
constexpr auto kDefaultIp = "192.168.0.0/24";
constexpr auto kMismatchIp = "192.169.0.0/24";
const int kDefaultUnit = 0;
const bcm_field_entry_t kDefaultEntry = 1;
constexpr auto kDefaultAclMsg = "Acl1";
constexpr auto kDefaultQualMsg = "Qual1";

int bcmFieldQualDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    uint32_t* data,
    uint32_t* mask) {
  *data = kDefaultData;
  *mask = kDefaultMask;
  return 0;
}
int bcmFieldQualMismatchDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    uint32_t* data,
    uint32_t* mask) {
  *data = kMismatchData;
  *mask = kDefaultMask;
  return 0;
}
int bcmFieldQualNotExistDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    uint32_t* /* data */,
    uint32_t* /* mask */) {
  // Do nothing
  return 0;
}

int bcmFieldQualModDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    uint32_t* modData,
    uint32_t* modMask,
    uint32_t* data,
    uint32_t* mask) {
  *modData = kDefaultData;
  *modMask = kDefaultMask;
  *data = kDefaultData;
  *mask = kDefaultMask;
  return 0;
}
int bcmFieldQualMismatchModDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    uint32_t* modData,
    uint32_t* modMask,
    uint32_t* data,
    uint32_t* mask) {
  *modData = kMismatchData;
  *modMask = kDefaultMask;
  *data = kMismatchData;
  *mask = kDefaultMask;
  return 0;
}
int bcmFieldQualNotExistModDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    uint32_t* /* modData */,
    uint32_t* /* modMask */,
    uint32_t* /* data */,
    uint32_t* /* mask */) {
  // Do nothing
  return 0;
}

int bcmFieldQualArrayDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    bcm_ip6_t* addr,
    bcm_ip6_t* mask) {
  facebook::fboss::networkToBcmIp6(
      IPAddress::createNetwork(kDefaultIp), addr, mask);
  return 0;
}
int bcmFieldQualMismatchArrayDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    bcm_ip6_t* addr,
    bcm_ip6_t* mask) {
  facebook::fboss::networkToBcmIp6(
      IPAddress::createNetwork(kMismatchIp), addr, mask);
  return 0;
}
int bcmFieldQualNotExistArrayDataMaskGet(
    int /* unit */,
    bcm_field_entry_t /* entry */,
    bcm_ip6_t* addr,
    bcm_ip6_t* mask) {
  memset(addr, 0, sizeof(bcm_ip6_t));
  memset(mask, 0, sizeof(bcm_ip6_t));
  return 0;
}
} // namespace

TEST(BcmAclUnitTests, AclDataMaskStateSame) {
  // qual field exists in both hw and sw
  std::optional<uint32_t> data{kDefaultData};
  EXPECT_TRUE(isBcmQualFieldStateSame(
      bcmFieldQualDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field only exist in sw
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualNotExistDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field mismatch
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualMismatchDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));

  // qual field doesn't exist in either hw or sw
  data = std::nullopt;
  EXPECT_TRUE(isBcmQualFieldStateSame(
      bcmFieldQualNotExistDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field only exist in hw
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));
}

TEST(BcmAclUnitTests, AclDataMaskStateSame2) {
  // qual field exists in both hw and sw
  std::optional<uint32_t> data{kDefaultData};
  std::optional<uint32_t> mask{kDefaultMask};
  EXPECT_TRUE(isBcmQualFieldWithMaskStateSame(
      bcmFieldQualDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      mask,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field only exist in sw
  EXPECT_FALSE(isBcmQualFieldWithMaskStateSame(
      bcmFieldQualNotExistDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      mask,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field mismatch
  EXPECT_FALSE(isBcmQualFieldWithMaskStateSame(
      bcmFieldQualMismatchDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      mask,
      kDefaultAclMsg,
      kDefaultQualMsg));

  // qual field doesn't exist in either hw or sw
  data = std::nullopt;
  mask = std::nullopt;
  EXPECT_TRUE(isBcmQualFieldWithMaskStateSame(
      bcmFieldQualNotExistDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      mask,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field only exist in hw
  EXPECT_FALSE(isBcmQualFieldWithMaskStateSame(
      bcmFieldQualDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      data,
      mask,
      kDefaultAclMsg,
      kDefaultQualMsg));
}

TEST(BcmAclUnitTests, AclModDataMaskStateSame) {
  // qual field exists in both hw and sw
  std::optional<uint32_t> modData{kDefaultData};
  std::optional<uint32_t> data{kDefaultData};
  EXPECT_TRUE(isBcmQualFieldStateSame(
      bcmFieldQualModDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      modData,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field only exist in sw
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualNotExistModDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      modData,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field mismatch
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualMismatchModDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      modData,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));

  // qual field doesn't exist in either hw or sw
  modData = std::nullopt;
  data = std::nullopt;
  EXPECT_TRUE(isBcmQualFieldStateSame(
      bcmFieldQualNotExistModDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      modData,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field only exist in hw
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualModDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      modData,
      data,
      kDefaultAclMsg,
      kDefaultQualMsg));
}

TEST(BcmAclUnitTests, AclArrayDataMaskStateSame) {
  // qual field exists in both hw and sw
  bcm_ip6_t addr, mask;
  facebook::fboss::networkToBcmIp6(
      IPAddress::createNetwork(kDefaultIp), &addr, &mask);
  EXPECT_TRUE(isBcmQualFieldStateSame(
      bcmFieldQualArrayDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      true,
      addr,
      mask,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field only exist in sw
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualNotExistArrayDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      true,
      addr,
      mask,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field mismatch
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualMismatchArrayDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      true,
      addr,
      mask,
      kDefaultAclMsg,
      kDefaultQualMsg));

  // qual field doesn't exist in either hw or sw
  bcm_ip6_t emptyAddr = {}, emptyMask = {};
  EXPECT_TRUE(isBcmQualFieldStateSame(
      bcmFieldQualNotExistArrayDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      false,
      emptyAddr,
      emptyMask,
      kDefaultAclMsg,
      kDefaultQualMsg));
  // qual field only exist in hw
  EXPECT_FALSE(isBcmQualFieldStateSame(
      bcmFieldQualArrayDataMaskGet,
      kDefaultUnit,
      kDefaultEntry,
      false,
      emptyAddr,
      emptyMask,
      kDefaultAclMsg,
      kDefaultQualMsg));
}
