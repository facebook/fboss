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

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/AclEntry.h"

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include <optional>

#include <vector>

extern "C" {
#include <bcm/error.h>
#include <bcm/field.h>
}

namespace {
constexpr auto kNoneValue = "NONE";
} // unnamed namespace

namespace facebook::fboss::utility {

struct BcmAclMirrorActionParameters {
  std::optional<BcmMirrorHandle> ingressMirrorHandle;
  std::optional<BcmMirrorHandle> egressMirrorHandle;
};

struct BcmAclActionParameters {
  BcmAclMirrorActionParameters mirrors;
};

template <typename Param>
std::string getOptionalStr(const std::optional<Param>& param) {
  if (!param) {
    return kNoneValue;
  }
  // TODO(joseph5wu): need to print unsigned
  return folly::to<std::string>(param.value());
}

/**
 * Broadcom quailfier field state check
 * function template definition should be here!
 */
template <typename Func, typename Param>
bool isBcmQualFieldStateSame(
    Func getBcmQualifierFn,
    int unit,
    bcm_field_entry_t entry,
    const std::optional<Param>& swValue,
    const std::string& aclMsg,
    const std::string& qualMsg) {
  Param hwData{}, hwMask{};
  auto rv = getBcmQualifierFn(unit, entry, &hwData, &hwMask);
  bcmCheckError(rv, aclMsg, " failed to get ", qualMsg, " qualifier");

  // Broadcom will return 0 if value is not set. For us, we don't usually set
  // value to 0. Even when we set value to 0, the condition will still work.
  // Cause we use std::optional for S/W params. In this case if we don't set
  // in S/W, H/W will return 0 and it will match the first or condition. If we
  // set 0 in S/W, H/W will still return 0 and then it will match the second or.
  auto hwValue = hwData & hwMask;
  bool isNotExistInBoth = (!swValue && !hwValue);
  // only check match if exist in both
  bool isValueSameInBoth = swValue && (swValue.value() == hwValue);
  if (isNotExistInBoth || isValueSameInBoth) {
    return true;
  }
  XLOG(ERR) << aclMsg << " " << qualMsg << " qualifier doesn't match."
            << " Expected SW value=" << getOptionalStr(swValue)
            << ", actual HW data=" << hwData << ", mask=" << hwMask;
  return false;
}

template <typename Func, typename Param>
bool isBcmQualFieldWithMaskStateSame(
    Func getBcmQualifierFn,
    int unit,
    bcm_field_entry_t entry,
    const std::optional<Param>& swData,
    const std::optional<Param>& swMask,
    const std::string& aclMsg,
    const std::string& qualMsg) {
  Param hwData{}, hwMask{};
  auto rv = getBcmQualifierFn(unit, entry, &hwData, &hwMask);
  bcmCheckError(rv, aclMsg, " failed to get ", qualMsg, " qualifier");

  bool isNotExistInBoth = (!swData && !hwData) && (!swMask && !hwMask);
  // only check match if exist in both
  bool isValueSameInBoth = (swData && (swData.value() == hwData)) &&
      (swMask && (swMask.value() == hwMask));
  if (isNotExistInBoth || isValueSameInBoth) {
    return true;
  }
  XLOG(ERR) << aclMsg << " " << qualMsg << " qualifier doesn't match."
            << " Expected SW value=" << getOptionalStr(swData)
            << ", SW mask=" << getOptionalStr(swMask)
            << ", actual HW data=" << hwData << ", mask=" << hwMask;
  return false;
}

template <typename Func, typename ModuleParam, typename Param>
bool isBcmQualFieldStateSame(
    Func getBcmQualifierFn,
    int unit,
    bcm_field_entry_t entry,
    const std::optional<ModuleParam>& swModValue,
    const std::optional<Param>& swValue,
    const std::string& aclMsg,
    const std::string& qualMsg) {
  ModuleParam hwModData{}, hwModMask{};
  Param hwData{}, hwMask{};
  auto rv =
      getBcmQualifierFn(unit, entry, &hwModData, &hwModMask, &hwData, &hwMask);
  bcmCheckError(rv, aclMsg, " failed to get ", qualMsg, " qualifier");

  // Broadcom will return 0 if value is not set. For us, we don't usually set
  // value to 0. Even when we set value to 0, the condition will still work.
  // Cause we use std::optional for S/W params. In this case if we don't set
  // in S/W, H/W will return 0 and it will match the first or condition. If we
  // set 0 in S/W, H/W will still return 0 and then it will match the second or.
  auto hwModValue = hwModData & hwModMask;
  auto hwValue = hwData & hwMask;
  bool isNotExistInBoth =
      (!swModValue && !hwModValue) && (!swValue && !hwValue);
  // only check match if exist in both
  bool isValueSameInBoth = swModValue && swValue &&
      (swModValue.value() == hwModValue) && (swValue.value() == hwValue);
  if (isNotExistInBoth || isValueSameInBoth) {
    return true;
  }
  XLOG(ERR) << aclMsg << " " << qualMsg << " qualify doesn't match."
            << " Expected SW modValue=" << getOptionalStr(swModValue)
            << ", value=" << getOptionalStr(swValue)
            << ", actual HW modData=" << hwModData << ", modMask=" << hwModMask
            << ", data=" << hwData << ", mask=" << hwMask;
  return false;
}

template <typename Func, typename Param>
bool isBcmEnumQualFieldStateSame(
    Func getBcmQualifierFn,
    int unit,
    bcm_field_entry_t entry,
    const std::optional<Param>& swValue,
    const std::string& aclMsg,
    const std::string& qualMsg) {
  Param hwValue{};
  auto rv = getBcmQualifierFn(unit, entry, &hwValue);
  // In broadcom side, when it's asked to get a non-existing qualifier for an
  // enum, it will return BCM_E_INTERNAL.
  if (rv == BCM_E_INTERNAL && !swValue.has_value()) {
    return true;
  }

  bcmCheckError(rv, aclMsg, " failed to get ", qualMsg, " qualifier");
  if ((!swValue && !hwValue) || (swValue && swValue.value() == hwValue)) {
    return true;
  }
  XLOG(ERR) << aclMsg << " " << qualMsg << " qualifier doesn't match."
            << " Expected SW param=" << getOptionalStr(swValue)
            << ", actual HW param=" << hwValue;
  return false;
}

template <typename Param, size_t size>
bool checkArrayHasNonZero(Param (&param)[size]) {
  // as long as the param has one non-zero item, we consider it valid
  for (size_t i = 0; i < size; i++) {
    if (param[i]) {
      return true;
    }
  }
  return false;
}

/**
 * `bcm_field_qualify_SrcIp6` and `bcm_field_qualify_DstIp6` will use this func.
 *  Cause they use bcm_ip6_t, which is uint8[16]
 */
template <typename Func, typename Param, size_t size>
bool isBcmQualFieldStateSame(
    Func getBcmQualifierFn,
    int unit,
    bcm_field_entry_t entry,
    bool existInSW,
    const Param (&swData)[size],
    const Param (&swMask)[size],
    const std::string& aclMsg,
    const std::string& qualMsg) {
  Param hwData[size];
  Param hwMask[size];
  auto rv = getBcmQualifierFn(unit, entry, &hwData, &hwMask);
  bcmCheckError(rv, aclMsg, " failed to get ", qualMsg, " qualifier");

  bool isNotExistInBoth = !existInSW && !checkArrayHasNonZero(hwData) &&
      !checkArrayHasNonZero(hwMask);
  // only check match if exist in both
  bool isValueSameInBoth = existInSW;
  if (existInSW) {
    for (size_t i = 0; i < size; i++) {
      isValueSameInBoth &= (hwData[i] == swData[i]);
    }
    for (size_t i = 0; i < size; i++) {
      isValueSameInBoth &= (hwMask[i] == swMask[i]);
    }
  }
  if (isNotExistInBoth || isValueSameInBoth) {
    return true;
  }
  // TODO(joseph5wu) log an array
  XLOG(ERR) << aclMsg << " " << qualMsg << " qualify doesn't match.";
  return false;
}

template <typename Range>
std::string getRangeStr(
    const std::optional<Range>& range,
    const std::string& type) {
  if (!range) {
    return kNoneValue;
  }
  return folly::to<std::string>(
      type,
      "[min=",
      range.value().getMin(),
      ", max=",
      range.value().getMax(),
      "], invert=",
      range.value().getInvert());
}

bool isBcmRangeStateSame(
    int unit,
    bcm_field_entry_t entry,
    const std::shared_ptr<AclEntry>& acl,
    const std::string& aclMsg);

bool isActionStateSame(
    int unit,
    bcm_field_entry_t entry,
    const std::shared_ptr<AclEntry>& acl,
    const std::string& aclMsg,
    const BcmAclActionParameters& data);

template <typename Func>
bool isBcmIp6QualFieldStateSame(
    int unit,
    bcm_field_entry_t entry,
    const std::string& aclMsg,
    const std::string& qualMsg,
    Func getBcmQualifierFn,
    folly::CIDRNetwork swIp6) {
  bcm_ip6_t ip6Data, ip6Mask;
  if (swIp6.first) {
    facebook::fboss::networkToBcmIp6(swIp6, &ip6Data, &ip6Mask);
  }
  return isBcmQualFieldStateSame(
      getBcmQualifierFn,
      unit,
      entry,
      !swIp6.first.empty(),
      ip6Data,
      ip6Mask,
      aclMsg,
      qualMsg);
}

template <typename Func>
bool isBcmMacQualFieldStateSame(
    int unit,
    bcm_field_entry_t entry,
    const std::string& aclMsg,
    const std::string& qualMsg,
    Func getBcmQualifierFn,
    const std::optional<folly::MacAddress>& swMac) {
  bcm_mac_t swMacData, swMacMaskData;
  if (swMac) {
    macToBcm(*swMac, &swMacData);
    macToBcm(folly::MacAddress::BROADCAST, &swMacMaskData);
  }
  return isBcmQualFieldStateSame(
      getBcmQualifierFn,
      unit,
      entry,
      swMac.has_value(),
      swMacData,
      swMacMaskData,
      aclMsg,
      qualMsg);
}

template <typename Func>
bool isBcmPortQualFieldStateSame(
    int unit,
    bcm_field_entry_t entry,
    const std::string& aclMsg,
    const std::string& qualMsg,
    Func getBcmQualifierFn,
    std::optional<uint16_t> swPort) {
  // Actually we don't need to care module values
  std::optional<bcm_module_t> moduleD{std::nullopt};
  std::optional<bcm_port_t> portD{std::nullopt};
  if (swPort) {
    moduleD = 0x0;
    portD = swPort.value();
  }
  return isBcmQualFieldStateSame(
      getBcmQualifierFn, unit, entry, moduleD, portD, aclMsg, qualMsg);
}

bool aclStatExists(
    int unit,
    bcm_field_entry_t aclEntry,
    BcmAclStatHandle* statHandle);

bcm_field_qset_t getAclQset(HwAsic::AsicType asicType);

bcm_field_qset_t getGroupQset(int unit, bcm_field_group_t groupId);

void clearFPGroup(int unit, bcm_field_group_t gid);

void createFPGroup(
    int unit,
    bcm_field_qset_t qset,
    bcm_field_group_t gid,
    int g_pri);

bool qsetsEqual(const bcm_field_qset_t& lhs, const bcm_field_qset_t& rhs);

bool fpGroupExists(int unit, bcm_field_group_t gid);
int fpGroupNumAclEntries(int unit, bcm_field_group_t gid);
int fpGroupNumAclStatEntries(int unit, bcm_field_group_t gid);
std::vector<bcm_field_group_t> fpGroupsConfigured(int unit);

class FPGroupDesiredQsetCmp {
 public:
  FPGroupDesiredQsetCmp(
      int unit,
      bcm_field_group_t groupId,
      const bcm_field_qset_t& desiredQset);

  bool hasDesiredQset();

 private:
  /*
   * Unfortunately bcm_group_get on a group returns a
   * different QSET than the one it was configured with.
   * getEffectiveDesiredQset, is used to get the QSET that
   * the QSET that would take effect once a group is configured
   * with desiredQset_
   */
  bcm_field_qset_t getEffectiveDesiredQset();
  int unit_{-1};
  const bcm_field_group_t groupId_;
  const bcm_field_qset_t groupQset_;
  const bcm_field_qset_t desiredQset_;
};

} // namespace facebook::fboss::utility
