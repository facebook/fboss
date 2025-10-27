/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiUdfManager.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/test/utils/UdfTestUtils.h"

namespace {
constexpr auto kInvalidUdfGroupId = -1;
constexpr auto kInvalidUdfPacketMatcherId = -1;

}; // namespace

namespace facebook::fboss::utility {

bool HwTestThriftHandler::validateUdfConfig(
    std::unique_ptr<::std::string> udfGroupName,
    std::unique_ptr<::std::string> udfPacketMatchName) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  const facebook::fboss::SaiUdfManager& udfManager =
      saiSwitch->managerTable()->udfManager();
  const auto& udfApi = SaiApiTable::getInstance()->udfApi();
  const auto udfGroupIter =
      udfManager.getUdfGroupHandles().find(*udfGroupName.get());
  if (udfGroupIter == udfManager.getUdfGroupHandles().cend()) {
    XLOG(ERR) << "udfGroupIter: Actual: " << " not found in udfGroupHandles "
              << " Expected: " << *udfGroupName.get();
    return false;
  }
  if (udfGroupIter != udfManager.getUdfGroupHandles().cend()) {
    auto saiUdfGroup = udfGroupIter->second->udfGroup;
    if (*udfGroupName.get() == utility::kUdfHashDstQueuePairGroupName) {
      // Verify UdfGroup attributes
      if (udfApi.getAttribute(
              saiUdfGroup->adapterKey(),
              SaiUdfGroupTraits::Attributes::Type{}) !=
          SAI_UDF_GROUP_TYPE_HASH) {
        XLOG(ERR) << "udfApi.getAttribute: Actual: "
                  << udfApi.getAttribute(
                         saiUdfGroup->adapterKey(),
                         SaiUdfGroupTraits::Attributes::Type{})
                  << " Expected: " << SAI_UDF_GROUP_TYPE_HASH;
        return false;
      }

      auto offsetInBytes = utility::kUdfHashDstQueuePairStartOffsetInBytes;
      auto fieldSizeInBytes = utility::kUdfHashDstQueuePairFieldSizeInBytes;
      if (hwSwitch_->getPlatform()->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_CHENAB) {
        offsetInBytes = utility::kChenabUdfHashDstQueuePairStartOffsetInBytes;
        fieldSizeInBytes = utility::kChenabUdfHashDstQueuePairFieldSizeInBytes;
      }

      if (udfApi.getAttribute(
              saiUdfGroup->adapterKey(),
              SaiUdfGroupTraits::Attributes::Length{}) != fieldSizeInBytes) {
        XLOG(ERR) << "udfApi.getAttribute: Actual: "
                  << udfApi.getAttribute(
                         saiUdfGroup->adapterKey(),
                         SaiUdfGroupTraits::Attributes::Length{})
                  << " Expected: " << fieldSizeInBytes;
        return false;
      }

      // Verify Udf attributes
      auto saiUdf = udfGroupIter->second->udfs[*udfPacketMatchName.get()]->udf;
      if (udfApi.getAttribute(
              saiUdf->adapterKey(), SaiUdfTraits::Attributes::Offset{}) !=
          offsetInBytes) {
        XLOG(ERR) << "udfApi.getAttribute: Actual: "
                  << udfApi.getAttribute(
                         saiUdf->adapterKey(),
                         SaiUdfTraits::Attributes::Offset{})
                  << " Expected: " << offsetInBytes;
        return false;
      }
      if (udfApi.getAttribute(
              saiUdf->adapterKey(), SaiUdfTraits::Attributes::Base{}) !=
          SAI_UDF_BASE_L4) {
        XLOG(ERR) << "udfApi.getAttribute: Actual: "
                  << udfApi.getAttribute(
                         saiUdf->adapterKey(), SaiUdfTraits::Attributes::Base{})
                  << " Expected: " << SAI_UDF_BASE_L4;
        return false;
      }

    } else if (*udfGroupName.get() == utility::kUdfAclRoceOpcodeGroupName) {
      // Verify UdfGroup attributes
      if (udfApi.getAttribute(
              saiUdfGroup->adapterKey(),
              SaiUdfGroupTraits::Attributes::Type{}) !=
          SAI_UDF_GROUP_TYPE_GENERIC) {
        XLOG(ERR) << "udfApi.getAttribute: Actual: "
                  << udfApi.getAttribute(
                         saiUdfGroup->adapterKey(),
                         SaiUdfGroupTraits::Attributes::Type{})
                  << " Expected: " << SAI_UDF_GROUP_TYPE_GENERIC;
        return false;
      }
      if (udfApi.getAttribute(
              saiUdfGroup->adapterKey(),
              SaiUdfGroupTraits::Attributes::Length{}) !=
          utility::kUdfAclRoceOpcodeFieldSizeInBytes) {
        XLOG(ERR) << "udfApi.getAttribute: Actual: "
                  << udfApi.getAttribute(
                         saiUdfGroup->adapterKey(),
                         SaiUdfGroupTraits::Attributes::Length{})
                  << " Expected: "
                  << utility::kUdfAclRoceOpcodeFieldSizeInBytes;
        return false;
      }

      // Verify Udf attributes
      auto saiUdf = udfGroupIter->second->udfs[*udfPacketMatchName.get()]->udf;
      if (udfApi.getAttribute(
              saiUdf->adapterKey(), SaiUdfTraits::Attributes::Offset{}) !=
          utility::kUdfAclRoceOpcodeStartOffsetInBytes) {
        XLOG(ERR) << "udfApi.getAttribute: Actual: "
                  << udfApi.getAttribute(
                         saiUdf->adapterKey(),
                         SaiUdfTraits::Attributes::Offset{})
                  << " Expected: "
                  << utility::kUdfAclRoceOpcodeStartOffsetInBytes;
        return false;
      }
      if (udfApi.getAttribute(
              saiUdf->adapterKey(), SaiUdfTraits::Attributes::Base{}) !=
          SAI_UDF_BASE_L4) {
        XLOG(ERR) << "udfApi.getAttribute: Actual: "
                  << udfApi.getAttribute(
                         saiUdf->adapterKey(), SaiUdfTraits::Attributes::Base{})
                  << " Expected: " << SAI_UDF_BASE_L4;
        return false;
      }
    } else {
      XLOG(ERR) << "Unsupported UDF Group name " << *udfGroupName.get()
                << ". Cannot validate.";
      return false;
    }
  }

  // Verify UdfMatch attributes
  const auto UdfMatchIter =
      udfManager.getUdfMatchHandles().find(*udfPacketMatchName.get());
  if (UdfMatchIter == udfManager.getUdfMatchHandles().cend()) {
    XLOG(ERR) << "udfPacketMatchName=" << *udfPacketMatchName.get()
              << " not found in getUdfMatchHandles";
    return false;
  }
  if (UdfMatchIter != udfManager.getUdfMatchHandles().cend()) {
    auto saiUdfMatch = UdfMatchIter->second->udfMatch;
    if (udfApi.getAttribute(
            saiUdfMatch->adapterKey(),
            SaiUdfMatchTraits::Attributes::L3Type{}) !=
        AclEntryFieldU8(
            std::make_pair(
                static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
                static_cast<uint8_t>(SaiUdfManager::kMaskAny)))) {
      XLOG(ERR) << "saiUdfMatch->adapterKey(), "
                << "SaiUdfMatchTraits::Attributes::L3Type {} is not  " << "udp";
      return false;
    }
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    if (udfApi.getAttribute(
            saiUdfMatch->adapterKey(),
            SaiUdfMatchTraits::Attributes::L4DstPortType{}) !=
        AclEntryFieldU16(
            std::make_pair(kUdfL4DstPort, SaiUdfManager::kL4PortMask))) {
      XLOG(ERR) << "saiUdfMatch->adapterKey(), " << "L4DstPortType {} is not "
                << kUdfL4DstPort;
      return false;
    }
#endif
  }
  return true;
}

bool HwTestThriftHandler::validateRemoveUdfGroup(
    std::unique_ptr<::std::string> udfGroupName,
    int udfGroupId) {
  // ensure that the udf group is removed from the switch
  // and getHwUdfGroupId returns -1 value
  if (getHwUdfGroupId(std::move(udfGroupName)) != -1) {
    XLOG(ERR) << "getHwUdfGroupId: Expected to fail but succeeded";
    return false;
  }

  // Verify SDK removed udf group
  const auto& udfApi = SaiApiTable::getInstance()->udfApi();
  // expect getAttribute to throw an error
  try {
    udfApi.getAttribute(
        static_cast<facebook::fboss::UdfGroupSaiId>(udfGroupId),
        SaiUdfGroupTraits::Attributes::Type{});
    XLOG(ERR) << "udfApi.getAttribute: Expected to throw an error but did not";
    return false;
  } catch (const SaiApiError&) {
    return true;
  }
  return true;
}

bool HwTestThriftHandler::validateRemoveUdfPacketMatcher(
    std::unique_ptr<::std::string> udfPackeMatchName,
    int32_t udfPacketMatcherId) {
  // ensure that the udf packet matcher is removed from the switch
  // and getHwUdfPacketMatcherId returns -1 value
  if (getHwUdfPacketMatcherId(std::move(udfPackeMatchName)) != -1) {
    XLOG(ERR) << "getHwUdfPacketMatcherId: Expected to fail but succeeded";
    return false;
  }
  // Verify SDK removed udf packet matcher
  const auto& udfApi = SaiApiTable::getInstance()->udfApi();

  // expect getAttribute to throw an error
  try {
    udfApi.getAttribute(
        static_cast<facebook::fboss::UdfMatchSaiId>(udfPacketMatcherId),
        SaiUdfMatchTraits::Attributes::L3Type{});
    XLOG(ERR) << "udfApi.getAttribute: Expected to throw an error but did not";
    return false;
  } catch (const SaiApiError&) {
    return true;
  }
  return false;
}

int32_t HwTestThriftHandler::getHwUdfGroupId(
    std::unique_ptr<::std::string> udfGroupName) {
  const auto& udfGroupHandles = static_cast<const SaiSwitch*>(hwSwitch_)
                                    ->managerTable()
                                    ->udfManager()
                                    .getUdfGroupHandles();
  const auto udfGroupIter = udfGroupHandles.find(*udfGroupName.get());
  if (udfGroupIter != udfGroupHandles.cend()) {
    return udfGroupIter->second->udfGroup->adapterKey();
  }
  XLOG(ERR) << "Cannot find UdfGroup " << *udfGroupName.get()
            << " in Sai Switch";
  return kInvalidUdfGroupId;
}

int32_t HwTestThriftHandler::getHwUdfPacketMatcherId(
    std::unique_ptr<::std::string> udfPackeMatchName) {
  const auto& udfMatchHandles = static_cast<const SaiSwitch*>(hwSwitch_)
                                    ->managerTable()
                                    ->udfManager()
                                    .getUdfMatchHandles();
  const auto udfMatchIter = udfMatchHandles.find(*udfPackeMatchName.get());
  if (udfMatchIter != udfMatchHandles.cend()) {
    return udfMatchIter->second->udfMatch->adapterKey();
  }
  XLOG(ERR) << "Cannot find UdfMatch " << *udfPackeMatchName.get()
            << " in Sai ";
  return kInvalidUdfPacketMatcherId;
}

bool HwTestThriftHandler::validateUdfIdsInQset(
    const int /*aclGroupId*/,
    const bool /*isSet*/) {
  // not supported on SAI yet.
  return true;
}

bool HwTestThriftHandler::validateUdfAclRoceOpcodeConfig(
    std::unique_ptr<::facebook::fboss::state::SwitchState> /*curState*/) {
  // not supported on SAI yet.
  return true;
}

} // namespace facebook::fboss::utility
