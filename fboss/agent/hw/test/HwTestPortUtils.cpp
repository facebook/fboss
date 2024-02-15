// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPortUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss::utility {

TransceiverInfo getTransceiverInfo(cfg::PortProfileID profileID) {
  TransceiverInfo info;
  info.tcvrState()->present() = true;
  info.tcvrState()->transceiver() = TransceiverType::QSFP;

  Cable cable;
  auto mediaType = getMediaType(profileID);
  cable.transmitterTech() = mediaType;
  if (mediaType == TransmitterTechnology::COPPER) {
    cable.length() = 1.0;
  } else if (mediaType == TransmitterTechnology::OPTICAL) {
    cable.length() = 2000;
  }
  info.tcvrState()->cable() = cable;

  // Prepare mediaInterface and managementInterface
  auto speed = getSpeed(profileID);
  MediaInterfaceCode mediaInterface;
  TransceiverManagementInterface mgmtInterface;
  if (speed == cfg::PortSpeed::EIGHTHUNDREDG) {
    mediaInterface = MediaInterfaceCode::FR4_2x400G;
    mgmtInterface = TransceiverManagementInterface::CMIS;
  } else if (speed == cfg::PortSpeed::FOURHUNDREDG) {
    mediaInterface = MediaInterfaceCode::FR4_400G;
    mgmtInterface = TransceiverManagementInterface::CMIS;
  } else if (speed == cfg::PortSpeed::TWOHUNDREDG) {
    mediaInterface = MediaInterfaceCode::FR4_200G;
    mgmtInterface = TransceiverManagementInterface::CMIS;
  } else if (static_cast<int>(speed) <= 100000) {
    mediaInterface = MediaInterfaceCode::CWDM4_100G;
    mgmtInterface = TransceiverManagementInterface::SFF;
  } else {
    throw FbossError(
        "Unrecoginized profile:",
        apache::thrift::util::enumNameSafe(profileID),
        ", and speed:",
        apache::thrift::util::enumNameSafe(speed));
  }

  info.tcvrState()->transceiverManagementInterface() = mgmtInterface;
  TransceiverSettings settings;
  std::vector<MediaInterfaceId> mediaInterfaces;
  for (auto i = 0; i < 4; i++) {
    MediaInterfaceId lane;
    lane.lane() = i;
    lane.code() = mediaInterface;
    mediaInterfaces.push_back(lane);
  }
  settings.mediaInterface() = mediaInterfaces;
  info.tcvrState()->settings() = settings;

  return info;
}

} // namespace facebook::fboss::utility
