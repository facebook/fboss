// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/phy/PhyUtils.h"

namespace facebook::fboss::utility {

bool isReedSolomonFec(phy::FecMode fec) {
  switch (fec) {
    case phy::FecMode::CL91:
    case phy::FecMode::RS528:
    case phy::FecMode::RS544:
    case phy::FecMode::RS544_2N:
      return true;
    case phy::FecMode::CL74:
    case phy::FecMode::NONE:
      return false;
  }
}

} // namespace facebook::fboss::utility
