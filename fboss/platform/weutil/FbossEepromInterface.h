// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace facebook::fboss::platform {

using EepromContents = std::vector<
    std::pair<std::string /* eeprom key */, std::string /* eeprom value */>>;

class FbossEepromInterface {
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
    std::string fieldName;
    entryType fieldType;
    std::optional<int> length{std::nullopt};
    std::string value{};
  };

  static FbossEepromInterface createEepromInterface(int version);

  void setField(int typeCode, const std::string& value);

  const std::map<int, EepromFieldEntry>& getFieldDictionary() const;

  EepromContents getContents() const;

  int getVersion() const;
  std::string getProductionState() const;
  std::string getProductionSubState() const;
  std::string getVariantVersion() const;

 private:
  FbossEepromInterface() = default;

  std::map<int, EepromFieldEntry> fieldMap_{};
  int version_{0};
};

} // namespace facebook::fboss::platform
