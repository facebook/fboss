// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/hw/sai/api/TamApi.h"

#include "fboss/lib/CommonFileUtils.h"

extern "C" {
#include <experimental/sai_attr_ext.h>
}

DEFINE_string(
    wb_downgrade_target_sdk_version_file,
    "/var/facebook/fboss/wb_downgrade_target_sdk_version.json",
    "The SDK version to warm boot downgrade to.");

namespace {
std::string eventName(sai_switch_event_type_t type) {
  switch (type) {
    case SAI_SWITCH_EVENT_TYPE_NONE:
      return "SAI_SWITCH_EVENT_TYPE_NONE";
    case SAI_SWITCH_EVENT_TYPE_ALL:
      return "SAI_SWITCH_EVENT_TYPE_ALL";
    case SAI_SWITCH_EVENT_TYPE_STABLE_FULL:
      return "SAI_SWITCH_EVENT_TYPE_STABLE_FULL";
    case SAI_SWITCH_EVENT_TYPE_STABLE_ERROR:
      return "SAI_SWITCH_EVENT_TYPE_STABLE_ERROR";
    case SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN:
      return "SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN";
    case SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE:
      return "SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN";
    case SAI_SWITCH_EVENT_TYPE_PARITY_ERROR:
      return "SAI_SWITCH_EVENT_TYPE_PARITY_ERROR";
#if defined(TAJO_SDK_EBRO) || defined(TAJO_SDK_VERSION_24_4_90)
    case SAI_SWITCH_EVENT_TYPE_LACK_OF_RESOURCES:
      return "SAI_SWITCH_EVENT_TYPE_LACK_OF_RESOURCES";
#endif
#if defined(TAJO_SDK_GTE_24_4_90)
    case SAI_SWITCH_EVENT_TYPE_MAX:
      return "SAI_SWITCH_EVENT_TYPE_MAX";
#endif
  }
  return folly::to<std::string>("unknown event type: ", type);
}

#if defined(TAJO_SDK_GTE_24_4_90)
std::string correctionType(sai_tam_switch_event_ecc_err_type_t type) {
  switch (type) {
    case SAI_TAM_SWITCH_EVENT_ECC_ERR_TYPE_ECC_COR:
      return "ECC_COR";
    case SAI_TAM_SWITCH_EVENT_ECC_ERR_TYPE_ECC_UNCOR:
      return "ECC_UNCOR";
    case SAI_TAM_SWITCH_EVENT_ECC_ERR_TYPE_PARITY:
      return "PARITY";
  }
  return "correction-type-unknown";
}
#else
std::string correctionType(sai_tam_switch_event_ecc_err_type_e type) {
  switch (type) {
    case ECC_COR:
      return "ECC_COR";
    case ECC_UNCOR:
      return "ECC_UNCOR";
    case PARITY:
      return "PARITY";
  }
  return "correction-type-unknown";
}
#endif
#if defined(TAJO_SDK_VERSION_1_42_8)
std::string lackOfResourceType(
    const sai_tam_switch_event_lack_of_resources_err_type_e& type) {
  switch (type) {
    case LACK_OF_RESOURCES:
      return "SMS_OUT_OF_BANK";
  }
  return "resource-type-unknown";
}
#endif

#if defined(TAJO_SDK_GTE_24_4_90)
std::string lackOfResourceType(
    const sai_tam_switch_event_lack_of_resources_err_type_t& type) {
  switch (type) {
    case SAI_TAM_SWITCH_EVENT_LACK_OF_RESOURCES_ERR_TYPE_LACK_OF_RESOURCES:
      return "SMS_OUT_OF_BANK";
  }
  return "resource-type-unknown";
}
#endif
} // namespace

namespace facebook::fboss {

void SaiSwitch::tamEventCallback(
    sai_object_id_t /*tam_event_id*/,
    sai_size_t /*buffer_size*/,
    const void* buffer,
    uint32_t /*attr_count*/,
    const sai_attribute_t* /*attr_list*/) {
  auto eventDesc = static_cast<const sai_tam_event_desc_t*>(buffer);
  if (eventDesc->type != (sai_tam_event_type_t)SAI_TAM_EVENT_TYPE_SWITCH) {
    // not a switch type event
    return;
  }
  auto eventType = eventDesc->event.switch_event.type;
  std::stringstream sstream;
  sstream << "received switch event=" << eventName(eventType);
  switch (eventType) {
    case SAI_SWITCH_EVENT_TYPE_PARITY_ERROR: {
      auto errorType = eventDesc->event.switch_event.data.parity_error.err_type;
      switch (errorType) {
#if defined(TAJO_SDK_GTE_24_4_90)
        case SAI_TAM_SWITCH_EVENT_ECC_ERR_TYPE_ECC_COR:
          getSwitchStats()->corrParityError();
          break;
        case SAI_TAM_SWITCH_EVENT_ECC_ERR_TYPE_ECC_UNCOR:
        case SAI_TAM_SWITCH_EVENT_ECC_ERR_TYPE_PARITY:
          getSwitchStats()->uncorrParityError();
          break;
#else
        case ECC_COR:
          getSwitchStats()->corrParityError();
          break;
        case ECC_UNCOR:
        case PARITY:
          getSwitchStats()->uncorrParityError();
          break;
#endif
      }
      sstream << ", correction type=" << correctionType(errorType);
    } break;
#if defined(TAJO_SDK_EBRO) || defined(TAJO_SDK_GTE_24_4_90)
    case SAI_SWITCH_EVENT_TYPE_LACK_OF_RESOURCES:
      // Log error for now!
      XLOG(ERR) << lackOfResourceType(eventDesc->event.switch_event.data
                                          .lack_of_resources.err_type)
                << " error detected by SDK!";
      break;
#endif
    case SAI_SWITCH_EVENT_TYPE_ALL:
    case SAI_SWITCH_EVENT_TYPE_STABLE_FULL:
    case SAI_SWITCH_EVENT_TYPE_STABLE_ERROR:
    case SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN:
    case SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE:
      getSwitchStats()->asicError();
      break;
    case SAI_SWITCH_EVENT_TYPE_NONE:
#if defined(TAJO_SDK_GTE_24_4_90)
    case SAI_SWITCH_EVENT_TYPE_MAX:
#endif
      // no-op
      break;
  }
  XLOG(WARNING) << sstream.str();
}

void SaiSwitch::switchEventCallback(
    sai_size_t /*buffer_size*/,
    const void* /*buffer*/,
    uint32_t /*event_type*/) {
  // noop;
}

void SaiSwitch::checkAndSetSdkDowngradeVersion() const {
  if (!checkFileExists(FLAGS_wb_downgrade_target_sdk_version_file)) {
    return;
  }

  std::string fileData;
  folly::readFile(FLAGS_wb_downgrade_target_sdk_version_file.c_str(), fileData);
  if (fileData.empty()) {
    throw FbossError(
        "Could not read contents of file ",
        FLAGS_wb_downgrade_target_sdk_version_file);
  }
  auto versionInfoData = folly::parseJson(fileData);
  if (!versionInfoData.isObject()) {
    throw FbossError(
        "Downgrade version info read from ",
        FLAGS_wb_downgrade_target_sdk_version_file,
        " not an object");
  }
  const auto& it = versionInfoData.items().begin();
  std::string sdkKey = it->first.asString();
  if (sdkKey.compare("sdkVersion")) {
    throw FbossError(
        "Expected key 'sdkVersion' not found in ",
        FLAGS_wb_downgrade_target_sdk_version_file);
  }
  auto downgradeVersion = it->second.asString();

  std::vector<int8_t> targetVersion;
  std::copy(
      downgradeVersion.c_str(),
      downgradeVersion.c_str() + downgradeVersion.size() + 1,
      std::back_inserter(targetVersion));
  SaiApiTable::getInstance()->switchApi().setAttribute(
      saiSwitchId_,
      SaiSwitchTraits::Attributes::WarmBootTargetVersion{targetVersion});
  XLOG(DBG2) << "Downgrade SDK version set as " << downgradeVersion;
}
} // namespace facebook::fboss
