// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPortUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss::utility {

TransceiverInfo getTransceiverInfo(cfg::PortProfileID profileID) {
  TransceiverInfo info;
  info.present_ref() = true;
  info.transceiver_ref() = TransceiverType::QSFP;

  Cable cable;
  auto mediaType = getMediaType(profileID);
  cable.transmitterTech_ref() = mediaType;
  if (mediaType == TransmitterTechnology::COPPER) {
    cable.length_ref() = 1.0;
  } else if (mediaType == TransmitterTechnology::OPTICAL) {
    cable.length_ref() = 2000;
  }
  info.cable_ref() = cable;

  // Prepare mediaInterface and managementInterface
  auto speed = getSpeed(profileID);
  MediaInterfaceCode mediaInterface;
  TransceiverManagementInterface mgmtInterface;
  if (speed == cfg::PortSpeed::FOURHUNDREDG) {
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

  info.transceiverManagementInterface_ref() = mgmtInterface;
  TransceiverSettings settings;
  std::vector<MediaInterfaceId> mediaInterfaces;
  for (auto i = 0; i < 4; i++) {
    MediaInterfaceId lane;
    lane.lane_ref() = i;
    lane.code_ref() = mediaInterface;
    mediaInterfaces.push_back(lane);
  }
  settings.mediaInterface_ref() = mediaInterfaces;
  info.settings_ref() = settings;

  return info;
}

} // namespace facebook::fboss::utility
