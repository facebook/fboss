// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
#include "fboss/agent/hw/sai/api/FirmwareApi.h"
#endif
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
using SaiFirmware = SaiObject<SaiFirmwareTraits>;
#endif

struct SaiFirmwareHandle {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  std::shared_ptr<SaiFirmware> firmware;
#endif
  SaiManagerTable* managerTable;
};

class SaiFirmwareManager {
 public:
  SaiFirmwareManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);

  // The current SAI API supports only a single Firmware.
  // In future, when multiple Firmwares are supported, SAI APIs will expose
  // additional APIs and get attrs (e.g. PATH) to distinguish between multiple
  // firmwares.
  static auto constexpr kFirmwareName = "firmware";

  const SaiFirmwareHandle* getFirmwareHandle(const std::string& name) const {
    return handles_.contains(name) ? handles_.at(name).get() : nullptr;
  }

  SaiFirmwareHandle* getFirmwareHandle(const std::string& name) {
    return handles_.contains(name) ? handles_.at(name).get() : nullptr;
  }

  std::optional<std::string> getFirmwareVersion() const;
  std::optional<FirmwareOpStatus> getFirmwareOpStatus() const;
  std::optional<FirmwareFuncStatus> getFirmwareFuncStatus() const;

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  folly::F14FastMap<std::string, std::unique_ptr<SaiFirmwareHandle>> handles_;
};

} // namespace facebook::fboss
