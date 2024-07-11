// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/TransceiverValidator.h"

DEFINE_bool(
    enable_tcvr_validation_scuba_logging,
    false,
    "Enable scuba logging for transceiver validation feature in qsfp_service");

namespace facebook::fboss {
TransceiverValidator::TransceiverValidator(
    const std::vector<VendorConfig>& tcvrConfigs) {
  for (auto vendorConfig : tcvrConfigs) {
    tcvrValMap_[vendorConfig.vendorName().value()] =
        std::move(vendorConfig.partNumberToTransceiverAttributes().value());
  }
};
} // namespace facebook::fboss
