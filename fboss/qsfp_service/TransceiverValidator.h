// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/logging/xlog.h>
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_validation_types.h"

DECLARE_bool(enable_tcvr_validation_scuba_logging);

namespace facebook::fboss {

/*
 * TransceiverValidator
 *
 * This class is responsible for handling transceiver configuration validation,
 * specifically considering hardware, firmware, and programmed attributes.
 *
 * TransceiverValidator is initialized only if the `enable_tcvr_validation`
 * feature flag is turned on and the transceiverValidationConfig field in the
 * qsfp_config file is defined – validation data for this class is loaded from
 * this file directly.
 *
 * The class's constructor is passed in a list of VendorConfig objects defined
 * in transceiver_validation.thrift, which each contain a vendorName and a map
 * from vendorPartNumber to TransceiverSpec. We use this TransceiverSpec thrift
 * struct directly to increase readibility.
 *
 */
class TransceiverValidator {
 public:
  explicit TransceiverValidator(const std::vector<VendorConfig>& tcvrConfigs);

 private:
  /*
   * This data structure maps vendorName (string) to an inner map of
   * vendorPartNumber (string) to TransceiverSpec, which is a struct based on a
   * thrift class that contains the mediaInterfaceCode, firmwareVersions, and
   * supportedPortProfiles.
   */
  std::unordered_map<
      std::string,
      std::unordered_map<std::string, TransceiverAttributes>>
      tcvrValMap_;
};

} // namespace facebook::fboss
