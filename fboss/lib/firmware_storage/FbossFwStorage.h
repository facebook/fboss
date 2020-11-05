// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fcntl.h>
#include <folly/FileUtil.h>
#include <folly/init/Init.h>
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <yaml-cpp/yaml.h>
#include <vector>
#include "fboss/lib/firmware_storage/FbossFirmware.h"

// Reference to default firmware manifest file
DECLARE_string(default_firmware_manifest);

namespace facebook::fboss {

/*
 * FbossFwStorage
 *
 * A class representing the firmware storage in the FBOSS platform. It parses
 * the yaml file containing descrption of product/modules and their supported
 * firmware information. This class allows user to create FbossFirmware objects
 * for a given module and firmware version.
 *
 * In a typical usage, the caller calls the static function initStorage() with
 * the firmware manifest file (a yaml file with all module/product description).
 * This returns FbossFwStorage object to caller. Then caller can use
 * getFwVersionList() to get the list of all the firmware version records for a
 * product/module name. Based on that the user can call getFirmware() with
 * product name and firmware version string to get the FbossFirmware object.
 * Later the FbossFirmware object can be used to extract firmware information
 * like properties, firmware image payload etc.
 */
class FbossFwStorage {
 public:
  // Record of firmware for a given product. This contains product/module name,
  // its description and map of version id to version record
  struct FwIdentifier {
    std::string prodName;
    std::string description;
    std::unordered_map<std::string, struct FbossFirmware::FwAttributes>
        versionRecord;
  };

  // Parses yaml file and creates FbossFwStorage object
  static FbossFwStorage initStorage(
      std::string fwConfigFile = FLAGS_default_firmware_manifest);

  // Provides the list of firmware versions for a module/product
  const struct FwIdentifier& getFwVersionList(const std::string& name) const;

  // Returns the FbossFirmware object for a given module and f/w version
  std::unique_ptr<FbossFirmware> getFirmware(
      const std::string& name,
      const std::string& version);

 protected:
  // Contructor: needs the YAML::Node the root node of Yaml file parsing
  // This needs to be protected because exposed functon getFirmwareVersionList
  // is supposed to create the FbossFwStorage object
  explicit FbossFwStorage(const YAML::Node& fwInfoRoot, std::string fwDir);

  // Directory for  firmware files
  std::string firmwareDir_;

  // We parse YAML file and store the map of product/module  name to firmware
  // record localy in this object.
  std::unordered_map<std::string, struct FwIdentifier> firmwareRecord_;
};

} // namespace facebook::fboss
