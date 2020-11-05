// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/firmware_storage/FbossFwStorage.h"

DEFINE_bool(
    dump_fw_info,
    false,
    "Dump Recommended firmware version for the module, use with --module_name");
DEFINE_string(module_name, "", "Module part number in string format");

using namespace facebook::fboss;

/*
 * main
 *
 * This is useful only if this module is build as binary.
 * CLI:
 *   firmware_storage --dump_fw_info --module_name <module-name>
 */
int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);

  // Check the arguments
  if (!FLAGS_dump_fw_info || FLAGS_module_name.empty()) {
    printf("Pl specify --dump_fw_info with --module_name <module-name>\n");
    return 0;
  }

  // Parse the yaml config and get the FbossFwStorage object
  FbossFwStorage fbossFwStorage = FbossFwStorage::initStorage();

  // Get list of all firmware versions supported by the given product/module
  auto fwId = fbossFwStorage.getFwVersionList(FLAGS_module_name);

  // Print the version record of all the versions
  for (const auto& version : fwId.versionRecord) {
    auto verName = version.first;
    printf(
        "\nGetting info from product %s version %s\n",
        FLAGS_module_name.c_str(),
        verName.c_str());

    // Get the FbossFirmware object for each firmware versions and print info
    // from them
    std::unique_ptr<FbossFirmware> fbossFw =
        fbossFwStorage.getFirmware(FLAGS_module_name.c_str(), verName.c_str());
    // Load the image in local buffer of FbossFirmware object
    fbossFw->load();
    // Dump the firmware info from FbossFirmware object
    fbossFw->dumpFwInfo();
  }
}
