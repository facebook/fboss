// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <nlohmann/json.hpp>
#include <map>
#include <mutex>
#include <optional>
#include <utility>
#include <variant>
#include <vector>
#include "UARTDevice.h"

namespace rackmon {

// Describes how we intend on interpreting the value stored
// in a register.
enum class RegisterValueType {
  INTEGER = 0,
  STRING = 1,
  FLOAT = 2,
  FLAGS = 3,
  HEX = 4,
};

enum RegisterEndian { BIG, LITTLE };

// Fully describes a Register (Retrieved from register map JSON)
struct RegisterDescriptor {
  using FlagDescType = std::tuple<uint8_t, std::string>;
  using FlagsDescType = std::vector<FlagDescType>;
  // Starting address of the Register
  uint16_t begin = 0;
  // Length of the Register
  uint16_t length = 0;
  // Name of the Register
  std::string name{};
  // How deep is the historical record? If keep is 6, rackmon
  // will keep the latest 6 read values for later retrieval.
  uint16_t keep = 1;
  // This is a caveat of the 'keep' value. This forces us
  // to keep a value only if it changed from the previous read
  // value. Useful for state information.
  bool storeChangesOnly = false;

  // Endianness of the values, default to modbus defined (big).
  RegisterEndian endian = RegisterEndian::BIG;

  // Describes how to interpret the contents of the register.
  RegisterValueType format = RegisterValueType::HEX;

  // If the register stores a FLOAT type, this provides
  // the precision.
  uint16_t precision = 0;

  // Scale floating point value.
  float scale = 1.0;

  // Shift floating point value.
  float shift = 0.0;

  // If the register stores flags, this provides the desc.
  FlagsDescType flags{};
};

struct FlagType {
  bool value;
  std::string name;
  uint8_t bitOffset;
  bool operator==(const FlagType& other) const {
    return value == other.value && name == other.name &&
        bitOffset == other.bitOffset;
  }
};

struct RegisterValue {
  using FlagsType = std::vector<FlagType>;
  using ValueType = std::
      variant<int32_t, float, std::string, std::vector<uint8_t>, FlagsType>;

  // Dictates which of the variants in value to expect
  RegisterValueType type = RegisterValueType::INTEGER;
  ValueType value{};

  // The timestamp of when the value was read
  uint32_t timestamp = 0;

  RegisterValue() {}
  explicit RegisterValue(
      const std::vector<uint16_t>& reg,
      const RegisterDescriptor& desc,
      uint32_t tstamp);
  explicit RegisterValue(const std::vector<uint16_t>& reg);
  operator std::string();

 private:
  void makeString(const std::vector<uint16_t>& reg);
  void makeHex(const std::vector<uint16_t>& reg);
  void makeInteger(const std::vector<uint16_t>& reg, RegisterEndian end);
  void makeFloat(
      const std::vector<uint16_t>& reg,
      uint16_t precision,
      float scale,
      float shift);
  void makeFlags(
      const std::vector<uint16_t>& reg,
      const RegisterDescriptor::FlagsDescType& flagsDesc);
};
void to_json(nlohmann::json& j, const RegisterValue& m);

// Container of the register value/timestamp of a specific sampling.
struct RegisterReading {
  // Timestamp when the register was read.
  uint32_t timestamp = 0;
  // Value of the register when read.
  std::vector<uint16_t> value;
  explicit RegisterReading(uint16_t size) : value(size_t(size)) {}
};

// Container of a instance of a register at a given point in time.
struct Register {
 private:
  // In order to provide atomic updates of the registers without holding
  // a lock for up to 100s of milli-seconds, every register now has active
  // and inactive contents. We update the inactive content lock-less
  // and using a lock swapActive() which flips isFirstActive and
  // updateReferences() so the timestamp/value pair now point to the new
  // values without copies.
  std::pair<RegisterReading, RegisterReading> value_;
  bool isFirstActive = true;
  // Update references after changes to isFirstActive
  void updateReferences();

 public:
  // Reference to the register descriptor.
  const RegisterDescriptor& desc;

  // These point to the current active value/timestamp pair.
  uint32_t& timestamp;
  std::vector<uint16_t>& value;

  explicit Register(const RegisterDescriptor& d);
  Register(const Register& other);
  Register(Register&& other) noexcept;

  RegisterReading& getActive() {
    return isFirstActive ? value_.first : value_.second;
  }
  RegisterReading& getInactive() {
    return isFirstActive ? value_.second : value_.first;
  }
  void swapActive();

  // equals operator works only on valid register reads. Register
  // with a zero timestamp is considered as invalid.
  bool operator==(const Register& other) const {
    return timestamp != 0 && other.timestamp != 0 && value == other.value;
  }
  // Same but opposite of the equals operator.
  bool operator!=(const Register& other) const {
    return timestamp == 0 || other.timestamp == 0 || value != other.value;
  }

  void operator=(const Register& other) {
    value_ = other.value_;
    isFirstActive = other.isFirstActive;
    updateReferences();
  }
  void operator=(Register&& other) {
    value_ = std::move(other.value_);
    isFirstActive = other.isFirstActive;
    updateReferences();
  }

  // Returns true if the register contents is valid.
  operator bool() const {
    return timestamp != 0;
  }
  // Returns a string formatted value.
  operator std::string() const;

  // Returns the interpreted value of the register.
  operator RegisterValue() const;
};
void to_json(nlohmann::json& j, const Register& m);

// Container describing the register and its historical record.
struct RegisterStoreValue {
  uint16_t regAddr = 0;
  std::string name{};
  std::vector<RegisterValue> history{};
  RegisterStoreValue(uint16_t reg, const std::string& n)
      : regAddr(reg), name(n) {}
};
void to_json(nlohmann::json& j, const RegisterStoreValue& m);

// Container of values of a single register at multiple points in
// time. (RegisterDescriptor::keep defines the size of the depth
// of the historical record).
struct RegisterStore {
 private:
  // Reference to the register descriptor
  const RegisterDescriptor& desc_;
  // Address of the register.
  uint16_t regAddr_;
  // History of the register contents to keep. This is utilized as
  // a circular buffer with idx pointing to the current slot to
  // write.
  std::vector<Register> history_;
  int32_t idx_ = 0;
  mutable std::recursive_mutex historyMutex_{};
  // Allows for us to disable individual registers if the device
  // does not support it.
  bool enabled_ = true;

 public:
  explicit RegisterStore(const RegisterDescriptor& desc);
  RegisterStore(const RegisterStore& other);

  // API to get and set enable status.
  bool isEnabled();
  void disable();
  void enable();

  // Request to start loading new value into register
  std::vector<uint16_t>& beginReloadRegister();

  // Request to commit the loaded register
  void endReloadRegister(time_t reloadTime = std::time(nullptr));

  // Returns a reference to the last written value (Back of the list)
  Register& back();
  const Register& back() const;

  // Returns the front (Next to write) reference
  Register& front();

  // Advances the front.
  void operator++();

  // register address accessor
  uint16_t regAddr() const {
    return regAddr_;
  }

  const std::string& name() const {
    return desc_.name;
  }

  // Returns a string formatted representation of the historical record.
  operator std::string() const;

  // Returns the historical record of the values
  operator RegisterStoreValue() const;

  // Add the JSON conversion methods as friends.
  friend void to_json(nlohmann::json& j, const RegisterStore& m);
};
void to_json(nlohmann::json& j, const RegisterStore& m);

struct WriteActionInfo {
  std::optional<std::string> shell{};
  RegisterValueType interpret;
  std::optional<std::string> value{};
};
void from_json(const nlohmann::json& j, WriteActionInfo& action);

struct SpecialHandlerInfo {
  uint16_t reg;
  uint16_t len;
  int32_t period;
  std::string action;
  // XXX if we have more actions other than write,
  // this needs to become a std::variant<...>
  WriteActionInfo info;
};
void from_json(const nlohmann::json& j, SpecialHandlerInfo& m);

struct BaudrateConfig {
  bool isSet = false;
  uint16_t reg = 0;
  std::map<uint32_t, uint16_t> baudValueMap{};
};
void from_json(const nlohmann::json& j, BaudrateConfig& m);

// Storage for address ranges. Provides comparision operators
// to allow for it to be used as a key in a map --> This allows
// for us to do quick lookups of addr to register map to use.
struct AddrRange {
  // pair of start and end address.
  std::pair<uint8_t, uint8_t> range{};
  AddrRange() {}
  explicit AddrRange(uint8_t a, uint8_t b) : range(a, b) {}
  explicit AddrRange(uint8_t a) : range(a, a) {}

  bool contains(uint8_t) const;
};

// Container of an entire register map. This is the memory
// representation of each JSON register map descriptors
// at /etc/rackmon.d.
struct RegisterMap {
  AddrRange applicableAddresses;
  std::string name;
  uint16_t probeRegister;
  uint32_t defaultBaudrate;
  uint32_t preferredBaudrate;
  Parity parity;
  BaudrateConfig baudConfig{};
  std::vector<SpecialHandlerInfo> specialHandlers;
  std::map<uint16_t, RegisterDescriptor> registerDescriptors;
  const RegisterDescriptor& at(uint16_t reg) const {
    return registerDescriptors.at(reg);
  }
};

// Container of multiple register maps. It is keyed off
// the address range of each register map.
struct RegisterMapDatabase {
  std::vector<std::unique_ptr<RegisterMap>> regmaps{};

  // Returns a register map of a given address
  const RegisterMap& at(uint8_t addr) const;

  // Loads a configuration JSON into the DB.
  void load(const nlohmann::json& j);
};

// JSON conversion
void from_json(const nlohmann::json& j, RegisterMap& m);
void from_json(const nlohmann::json& j, AddrRange& a);
void from_json(const nlohmann::json& j, RegisterDescriptor& i);

void from_json(nlohmann::json& j, const RegisterMap& m);
void from_json(nlohmann::json& j, const AddrRange& a);
void from_json(nlohmann::json& j, const RegisterDescriptor& i);

} // namespace rackmon
