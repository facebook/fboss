// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/util/wedge_qsfp_util.h"

#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/lib/usb/GalaxyI2CBus.h"

#include <gflags/gflags.h>
#include <sysexits.h>

DEFINE_string(platform, "", "Platform on which we are running."
             " One of (galaxy, wedge100, wedge)");

using namespace facebook::fboss;

std::pair<std::unique_ptr<TransceiverI2CApi>, int>  getTransceiverAPI() {
  if (FLAGS_platform.size()) {
     if (FLAGS_platform == "galaxy") {
        return std::make_pair(std::make_unique<GalaxyI2CBus>(), 0);
     } else if (FLAGS_platform == "wedge100") {
        return std::make_pair(std::make_unique<Wedge100I2CBus>(), 0);
     } else if (FLAGS_platform == "wedge") {
        return std::make_pair(std::make_unique<WedgeI2CBus>(), 0);
     } else {
       fprintf(stderr, "Unknown platform %s\n", FLAGS_platform.c_str());
       return std::make_pair(nullptr, EX_USAGE);
     }
   }
  // TODO(klahey):  Should probably verify the other chip architecture.
  if (isTrident2()) {
     return std::make_pair(std::make_unique<WedgeI2CBus>(), 0);
  }
  return std::make_pair(std::make_unique<Wedge100I2CBus>(), 0);
}
