// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPortUtils.h"

namespace facebook::fboss::utility {

TransceiverInfo getTransceiverInfo(PortID port, TransmitterTechnology tech) {
  TransceiverInfo info;

  info.present_ref() = true;
  info.transceiver_ref() = TransceiverType::QSFP;
  info.port_ref() = port;

  Cable cable;
  cable.transmitterTech_ref() = tech;
  cable.length_ref() = 1.0;
  info.cable_ref() = cable;

  return info;
}

} // namespace facebook::fboss::utility
