// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/MinipackPimController.h"

#include "fboss/agent/FbossError.h"

namespace {
constexpr uint32_t kFacebookFpgaPimTypeBase = 0xFF000000;
constexpr uint32_t kFacebookFpgaPimTypeReg = 0x0;
} // namespace

namespace facebook::fboss {

MinipackPimController::PimType MinipackPimController::getPimType() const {
  uint32_t curPimTypeReg =
      io_->read(kFacebookFpgaPimTypeReg) & kFacebookFpgaPimTypeBase;
  switch (curPimTypeReg) {
    case static_cast<uint32_t>(MinipackPimController::PimType::MINIPACK_16Q):
    case static_cast<uint32_t>(MinipackPimController::PimType::MINIPACK_16O):
      return static_cast<MinipackPimController::PimType>(curPimTypeReg);
    default:
      throw FbossError(
          "Unrecoginized pim type with register value:", curPimTypeReg);
  }
}

} // namespace facebook::fboss
