/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AsicUtils.h"

namespace facebook::fboss {
const HwAsic& getHwAsicForAsicType(const cfg::AsicType& asicType) {
  /*
   * hwAsic is used to invoke methods such as getMaxPorts,
   * getVirtualDevices. For these methods, following attributes don't
   * matter. Hence set to some pre-defined values.
   * Using pre-defined values (instead of deriving dynamically from dsfNode)
   * allows us to use static hwAsic objects here.
   */
  int64_t switchId = 0;
  cfg::SwitchInfo switchInfo;
  switchInfo.switchMac() = "02:00:00:00:0F:0B";
  switchInfo.switchIndex() = 0;

  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_JERICHO2: {
      switchInfo.switchType() = cfg::SwitchType::VOQ;
      static Jericho2Asic jericho2Asic{switchId, switchInfo};
      return jericho2Asic;
    }
    case cfg::AsicType::ASIC_TYPE_JERICHO3: {
      switchInfo.switchType() = cfg::SwitchType::VOQ;
      static Jericho3Asic jericho3Asic{switchId, switchInfo};
      return jericho3Asic;
    }
    case cfg::AsicType::ASIC_TYPE_RAMON: {
      switchInfo.switchType() = cfg::SwitchType::FABRIC;
      static RamonAsic ramonAsic{switchId, switchInfo};
      return ramonAsic;
    }
    case cfg::AsicType::ASIC_TYPE_RAMON3: {
      switchInfo.switchType() = cfg::SwitchType::FABRIC;
      static Ramon3Asic ramon3Asic{switchId, switchInfo};
      return ramon3Asic;
    }
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_CHENAB:
      break;
  }

  throw FbossError(
      "Invalid Asic Type: ", apache::thrift::util::enumNameSafe(asicType));
}

uint32_t getFabricPortsPerVirtualDevice(const cfg::AsicType asicType) {
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
      return 192;
    case cfg::AsicType::ASIC_TYPE_RAMON:
      return 192;
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      return 160;
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      return 256;
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_CHENAB:
      throw FbossError(
          "Fabric ports are not applicable for: ",
          apache::thrift::util::enumNameSafe(asicType));
  }

  throw FbossError(
      "Invalid Asic Type: ", apache::thrift::util::enumNameSafe(asicType));
}

int getMaxNumberOfFabricPorts(const cfg::AsicType asicType) {
  return getFabricPortsPerVirtualDevice(asicType) *
      getHwAsicForAsicType(asicType).getVirtualDevices();
}
} // namespace facebook::fboss
