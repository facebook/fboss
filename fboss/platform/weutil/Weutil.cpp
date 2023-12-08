// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include "fboss/platform/weutil/Weutil.h"
#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/weutil/WeutilDarwin.h"
#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/if/gen-cpp2/weutil_config_types.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <ios>
#include <string>

namespace facebook::fboss::platform {

WeutilInfo parseConfig(std::string eeprom, std::string configFile) {
  // In this method, we get the config file from ConfigLib,
  // then translate that thrift structure to a plain structure.
  // The reason of this is because we will share the same WeutilImpl
  // in the BMC codebase, where we do not have access to Thrift/Folly.

  // This is thrift based config datastructure,
  // to be translated as the plain config above
  weutil_config::WeutilConfig thriftConfig;

  WeutilInfo info;

  // First, get the config file as string (JSON format)
  std::string weutilConfigJson;
  // Check if conf file name is set, if not, set the default name
  if (configFile.empty()) {
    XLOG(INFO) << "No config file was provided. Inferring from config_lib";
    weutilConfigJson = ConfigLib().getWeutilConfig();
  } else {
    XLOG(INFO) << "Using config file: " << configFile;
    if (!folly::readFile(configFile.c_str(), weutilConfigJson)) {
      throw std::runtime_error(
          "Can not find weutil config file: " + configFile);
    }
  }

  // Secondly, deserialize the JSON string to thrift struct
  apache::thrift::SimpleJSONSerializer::deserialize<
      weutil_config::WeutilConfig>(weutilConfigJson, thriftConfig);
  XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      thriftConfig);

  // Now, if the eeprom name is empty, use the chassis eeprom name
  if (eeprom.empty()) {
    eeprom = *thriftConfig.chassisEepromName();
  }
  std::transform(eeprom.begin(), eeprom.end(), eeprom.begin(), ::tolower);

  // Finally, translate the module name into the actual eeprom path
  for (auto& [eepromName, eepromConfig] : *thriftConfig.fruEepromList()) {
    std::string fruName = eepromName;
    std::transform(fruName.begin(), fruName.end(), fruName.begin(), ::tolower);
    if (fruName == eeprom) {
      info.eepromPath = *eepromConfig.path();
      info.offset = *eepromConfig.offset();
    }
    info.modules.push_back(fruName);
  }

  // Then return the info.
  return info;
}

std::unique_ptr<WeutilInterface> get_plat_weutil(
    const std::string& eeprom,
    const std::string& configFile) {
  facebook::fboss::PlatformProductInfo prodInfo{FLAGS_fruid_filepath};
  prodInfo.initialize();

  switch (prodInfo.getType()) {
    case PlatformType::PLATFORM_DARWIN:
      return std::make_unique<WeutilDarwin>(eeprom);
      break;

    default:
      WeutilInfo info = parseConfig(eeprom, configFile);
      return std::make_unique<WeutilImpl>(info);
      break;
  }
}

std::unique_ptr<WeutilInterface> get_meta_eeprom_handler(std::string path) {
  // Note that we pass path as the eeprom name, since we will skip the
  // eeprom name to the path translation (done by calling verify option method.)
  WeutilInfo info;
  info.eepromPath = std::move(path);
  // Offset is not used in this execution path,
  // but we need to make our infer bot happy.
  info.offset = 0;
  std::unique_ptr<WeutilImpl> pWeutilImpl = std::make_unique<WeutilImpl>(info);
  return std::move(pWeutilImpl);
}

} // namespace facebook::fboss::platform
