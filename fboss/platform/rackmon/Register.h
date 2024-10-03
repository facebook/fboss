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
  LONG = 5,
};

enum RegisterEndian { BIG, LITTLE };

// Fully describes a Register (Retrieved from register map JSON)
struct RegisterDescriptor {
  static constexpr time_t kDefaultInterval = 3 * 60;
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

  // true if we expect sign bit (signed)
  bool sign = false;

  // If the register stores flags, this provides the desc.
  FlagsDescType flags{};

  // Monitoring interval
  time_t interval = kDefaultInterval;
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
  using ValueType = std::variant<
      int32_t,
      int64_t,
      float,
      std::string,
      std::vector<uint8_t>,
      FlagsType>;

  // Dictates which of the variants in value to expect
  RegisterValueType type = RegisterValueType::INTEGER;
  ValueType value{};

  // The timestamp of when the value was read
  uint32_t timestamp = 0;

  RegisterValue() = default;
  explicit RegisterValue(
      const std::vector<uint16_t>& reg,
      const RegisterDescriptor& desc,
      uint32_t tstamp);
  explicit RegisterValue(const std::vector<uint16_t>& reg);
  operator std::string();

 private:
  void makeString(const std::vector<uint16_t>& reg);
  void makeHex(const std::vector<uint16_t>& reg);
  void
  makeInteger(const std::vector<uint16_t>& reg, RegisterEndian end, bool sign);
  void makeFloat(
      const std::vector<uint16_t>& reg,
      uint16_t precision,
      float scale,
      float shift,
      bool sign);
  void makeFlags(
      const std::vector<uint16_t>& reg,
      const RegisterDescriptor::FlagsDescType& flagsDesc);
};
void to_json(nlohmann::json& j, const RegisterValue& m);

// Container of a instance of a register at a given point in time.
struct Register {
  // Reference to the register descriptor.
  const RegisterDescriptor& desc;

  // These point to the current value.
  std::vector<uint16_t> value;

  // Timestamp of reading. 0 is considered invalid.
  uint32_t timestamp = 0;

  explicit Register(const RegisterDescriptor& d);
  Register(const Register& other);
  Register(Register&& other) noexcept;

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
    value = other.value;
    timestamp = other.timestamp;
  }
  void operator=(Register&& other) {
    value = std::move(other.value);
    timestamp = other.timestamp;
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

  std::vector<uint16_t>::iterator setRegister(
      std::vector<uint16_t>::iterator start,
      std::vector<uint16_t>::iterator end,
      time_t reloadTime = std::time(nullptr));

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

  size_t length() const {
    return desc_.length;
  }

  const RegisterDescriptor& descriptor() const {
    return desc_;
  }

  const std::string& name() const {
    return desc_.name;
  }

  time_t interval() const {
    return desc_.interval;
  }

  // Returns a string formatted representation of the historical record.
  operator std::string() const;

  // Returns the historical record of the values
  operator RegisterStoreValue() const;

  // Add the JSON conversion methods as friends.
  friend void to_json(nlohmann::json& j, const RegisterStore& m);
};
void to_json(nlohmann::json& j, const RegisterStore& m);

// Group of registers which are at contiguous register locations.
class RegisterStoreSpan {
  static constexpr uint16_t kMaxRegisterSpanLength = 120;
  uint16_t spanAddress_ = 0;
  time_t interval_ = 0;
  std::vector<uint16_t> span_{};
  std::vector<RegisterStore*> registers_{};
  time_t timestamp_ = 0;

 public:
  explicit RegisterStoreSpan(RegisterStore* reg);
  bool addRegister(RegisterStore* reg);
  std::vector<uint16_t>& beginReloadSpan();
  void endReloadSpan(time_t reloadTime);
  uint16_t getSpanAddress() const {
    return spanAddress_;
  }
  size_t length() const {
    return span_.size();
  }
  bool reloadPending(time_t currentTime);
  static bool buildRegisterSpanList(
      std::vector<RegisterStoreSpan>& list,
      RegisterStore& reg);
};

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

// Storage for address ranges. Provides comparision operators
// to allow for it to be used as a key in a map --> This allows
// for us to do quick lookups of addr to register map to use.
struct AddrRange {
  // vector of pair of start and end address.
  std::vector<std::pair<uint8_t, uint8_t>> range{};
  AddrRange() = default;
  explicit AddrRange(const std::vector<std::pair<uint8_t, uint8_t>>& addrRange)
      : range(addrRange) {}

  bool contains(uint8_t) const;
};

// Container of an entire register map. This is the memory
// representation of each JSON register map descriptors
// at /etc/rackmon.d.
struct RegisterMap {
  AddrRange applicableAddresses;
  std::string name;
  uint16_t probeRegister;
  uint32_t baudrate;
  Parity parity;
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

  struct Iterator {
    std::vector<std::unique_ptr<RegisterMap>>::const_iterator it;
    std::vector<std::unique_ptr<RegisterMap>>::const_iterator end;
    const std::optional<uint8_t> addr{};
    bool operator!=(struct Iterator const& other) const {
      return it != other.it;
    }
    bool operator==(struct Iterator const& other) const {
      return it == other.it;
    }
    Iterator& operator++();
    const RegisterMap& operator*() {
      if (it == end) {
        throw std::out_of_range("Getting info from end");
      }
      return **it;
    }
  };

  Iterator begin() {
    return Iterator{regmaps.begin(), regmaps.end()};
  }
  Iterator end() {
    return Iterator{regmaps.end(), regmaps.end()};
  }
  Iterator begin() const {
    return Iterator{regmaps.cbegin(), regmaps.cend()};
  }
  Iterator end() const {
    return Iterator{regmaps.cend(), regmaps.cend()};
  }
  Iterator find(uint8_t addr) const;

  const RegisterMap& at(uint8_t addr) const {
    return *find(addr);
  }

  // Loads a configuration JSON into the DB.
  void load(const nlohmann::json& j);

  time_t minMonitorInterval() const;
};

// JSON conversion
void from_json(const nlohmann::json& j, RegisterMap& m);
void from_json(const nlohmann::json& j, AddrRange& a);
void from_json(const nlohmann::json& j, RegisterDescriptor& i);

void from_json(nlohmann::json& j, const RegisterMap& m);
void from_json(nlohmann::json& j, const AddrRange& a);
void from_json(nlohmann::json& j, const RegisterDescriptor& i);

} // namespace rackmon
