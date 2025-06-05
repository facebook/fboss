// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace facebook::fboss::platform {

using EepromContents = std::vector<
    std::pair<std::string /* eeprom key */, std::string /* eeprom value */>>;

class FbossEepromInterface {
  virtual constexpr int getVersion() const = 0;

 public:
  enum entryType {
    FIELD_INVALID,
    FIELD_BE_UINT,
    FIELD_BE_HEX,
    FIELD_STRING,
    FIELD_MAC,
    FIELD_DATE
  };

  struct EepromFieldEntry {
    int typeCode;
    std::string fieldName;
    entryType fieldType;
    std::string* fieldPtr;
    std::optional<int> length{std::nullopt};
  };

  virtual std::vector<EepromFieldEntry> getFieldDictionary() = 0;

  EepromContents getContents() {
    EepromContents contents;
    contents.emplace_back("Version", std::to_string(getVersion()));
    for (const auto& entry : getFieldDictionary()) {
      if (!entry.fieldPtr) {
        continue;
      }
      if (entry.fieldType == FIELD_MAC) {
        // Format
        // (value): (00:00:00:00:00:00,222)
        // (value1);(value2): (00:00:00:00:00:00);(222)
        std::string value = *entry.fieldPtr;
        std::string value1 = value.substr(0, value.find(','));
        std::string value2 = value.substr(value.find(',') + 1);
        contents.emplace_back(entry.fieldName + " Base", value1);
        contents.emplace_back(entry.fieldName + " Address Size", value2);
      } else {
        contents.emplace_back(entry.fieldName, *entry.fieldPtr);
      }
    }
    return contents;
  }

  virtual std::string getProductionState() const = 0;
  virtual std::string getProductionSubState() const = 0;
  virtual std::string getVariantVersion() const = 0;

  virtual ~FbossEepromInterface() = default;
};

} // namespace facebook::fboss::platform
