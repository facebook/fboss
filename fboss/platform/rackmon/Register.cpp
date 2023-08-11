// Copyright 2021-present Facebook. All Rights Reserved.
#include "Register.h"
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

using nlohmann::json;

namespace rackmon {

bool AddrRange::contains(uint8_t addr) const {
  return addr >= range.first && addr <= range.second;
}

void RegisterValue::makeString(const std::vector<uint16_t>& reg) {
  value = "";
  auto& strValue = std::get<std::string>(value);
  // String is stored normally H L H L, so we
  // need reswap the bytes in each nibble.
  for (const auto& r : reg) {
    char ch = r >> 8;
    char cl = r & 0xff;
    if (ch == '\0') {
      break;
    }
    strValue += ch;
    if (cl == '\0') {
      break;
    }
    strValue += cl;
  }
}

void RegisterValue::makeHex(const std::vector<uint16_t>& reg) {
  value = std::vector<uint8_t>{};
  auto& hexValue = std::get<std::vector<uint8_t>>(value);
  for (uint16_t v : reg) {
    hexValue.push_back(v >> 8);
    hexValue.push_back(v & 0xff);
  }
}

void RegisterValue::makeInteger(
    const std::vector<uint16_t>& reg,
    RegisterEndian end) {
  // TODO We currently do not need more than 32bit values as per
  // our current/planned regmaps. If such a value should show up in the
  // future, then we might need to return std::variant<int32_t,int64_t>.
  if (reg.size() > 2) {
    throw std::out_of_range("Register does not fit as an integer");
  }
  // Everything in modbus is Big-endian. So when we have a list
  // of registers forming a larger value; For example,
  // a 32bit value would be 2 16bit regs.
  // Then the first register would be the upper nibble of the
  // resulting 32bit value.
  value = int32_t(0);
  if (end == BIG) {
    value =
        std::accumulate(reg.begin(), reg.end(), 0, [](int32_t ac, uint16_t v) {
          return (ac << 16) + v;
        });

  } else {
    value = std::accumulate(
        reg.rbegin(), reg.rend(), 0, [](int32_t ac, uint16_t v) {
          // Swap the bytes
          return (ac << 16) + (((v & 0xff) << 8) | ((v >> 8) & 0xff));
        });
  }
}

void RegisterValue::makeFloat(
    const std::vector<uint16_t>& reg,
    uint16_t precision,
    float scale,
    float shift) {
  makeInteger(reg, RegisterEndian::BIG);
  int32_t intValue = std::get<int32_t>(value);
  // Y = shift + scale * (X / 2^N)
  value = shift + (scale * (float(intValue) / float(1 << precision)));
}

void RegisterValue::makeFlags(
    const std::vector<uint16_t>& reg,
    const RegisterDescriptor::FlagsDescType& flagsDesc) {
  value = FlagsType{};
  auto& flagsValue = std::get<FlagsType>(value);
  for (const auto& [pos, name] : flagsDesc) {
    // The bit position is provided assuming the register contents
    // are combined together to form a big-endian larger integer.
    // Hence we need to reverse the register index. Each 16bit
    // word is already swapped by Msg, so we do not need to do that.
    uint16_t regIdx = reg.size() - (pos / 16) - 1;
    uint16_t regBit = pos % 16;
    bool bitVal = (reg[regIdx] & (1 << regBit)) != 0;
    flagsValue.push_back({bitVal, name, pos});
  }
}

RegisterValue::RegisterValue(
    const std::vector<uint16_t>& reg,
    const RegisterDescriptor& desc,
    uint32_t tstamp)
    : type(desc.format), timestamp(tstamp) {
  switch (desc.format) {
    case RegisterValueType::STRING:
      makeString(reg);
      break;
    case RegisterValueType::INTEGER:
      makeInteger(reg, desc.endian);
      break;
    case RegisterValueType::FLOAT:
      makeFloat(reg, desc.precision, desc.scale, desc.shift);
      break;
    case RegisterValueType::FLAGS:
      makeFlags(reg, desc.flags);
      break;
    case RegisterValueType::HEX:
      makeHex(reg);
  }
}

RegisterValue::RegisterValue(const std::vector<uint16_t>& reg)
    : type(RegisterValueType::HEX) {
  makeHex(reg);
}

Register::Register(const RegisterDescriptor& d)
    : value_{d.length, d.length},
      desc(d),
      timestamp(value_.first.timestamp),
      value(value_.first.value) {}

Register::Register(const Register& other)
    : value_(other.value_),
      isFirstActive(other.isFirstActive),
      desc(other.desc),
      timestamp(
          other.isFirstActive ? value_.first.timestamp
                              : value_.second.timestamp),
      value(other.isFirstActive ? value_.first.value : value_.second.value) {}

Register::Register(Register&& other) noexcept
    : value_(std::move(other.value_)),
      isFirstActive(other.isFirstActive),
      desc(other.desc),
      timestamp(
          other.isFirstActive ? value_.first.timestamp
                              : value_.second.timestamp),
      value(other.isFirstActive ? value_.first.value : value_.second.value) {}

void Register::updateReferences() {
  RegisterReading& active = getActive();
  value = active.value;
  timestamp = active.timestamp;
}

void Register::swapActive() {
  isFirstActive = !isFirstActive;
  updateReferences();
}

Register::operator RegisterValue() const {
  return RegisterValue(value, desc, timestamp);
}

RegisterStore::RegisterStore(const RegisterDescriptor& desc)
    : desc_(desc), regAddr_(desc.begin), history_(desc.keep, Register(desc)) {}

RegisterStore::RegisterStore(const RegisterStore& other)
    : desc_(other.desc_), regAddr_(other.regAddr_) {
  std::unique_lock lk(other.historyMutex_);
  history_ = other.history_;
  enabled_ = other.enabled_;
  idx_ = other.idx_;
}

bool RegisterStore::isEnabled() {
  std::unique_lock lk(historyMutex_);
  return enabled_;
}

void RegisterStore::enable() {
  std::unique_lock lk(historyMutex_);
  enabled_ = true;
}

void RegisterStore::disable() {
  std::unique_lock lk(historyMutex_);
  enabled_ = false;
}

std::vector<uint16_t>& RegisterStore::beginReloadRegister() {
  std::unique_lock lk(historyMutex_);
  return front().getInactive().value;
}

void RegisterStore::endReloadRegister(time_t reloadTime) {
  std::unique_lock lk(historyMutex_);
  front().getInactive().timestamp = reloadTime;
  // Update the front and bump indexs.
  front().swapActive();
  // If we care about changes only and the values
  // look the same, then ignore it.
  if (desc_.storeChangesOnly && front() == back()) {
    // Keep old value. Discard new value.
    front().swapActive();
    return;
  }
  ++(*this);
}

Register& RegisterStore::back() {
  std::unique_lock lk(historyMutex_);
  return idx_ == 0 ? history_.back() : history_[idx_ - 1];
}

const Register& RegisterStore::back() const {
  std::unique_lock lk(historyMutex_);
  return idx_ == 0 ? history_.back() : history_[idx_ - 1];
}

Register& RegisterStore::front() {
  std::unique_lock lk(historyMutex_);
  return history_[idx_];
}

void RegisterStore::operator++() {
  std::unique_lock lk(historyMutex_);
  idx_ = (idx_ + 1) % history_.size();
}

RegisterStore::operator RegisterStoreValue() const {
  std::unique_lock lk(historyMutex_);
  RegisterStoreValue ret(regAddr_, desc_.name);
  for (const auto& reg : history_) {
    if (reg) {
      ret.history.emplace_back(reg);
    }
  }
  return ret;
}

const RegisterMap& RegisterMapDatabase::at(uint8_t addr) const {
  const auto result = std::find_if(
      regmaps.begin(),
      regmaps.end(),
      [addr](const std::unique_ptr<RegisterMap>& m) {
        return m->applicableAddresses.contains(addr);
      });
  if (result == regmaps.end()) {
    throw std::out_of_range("not found: " + std::to_string(int(addr)));
  }
  return **result;
}

void RegisterMapDatabase::load(const nlohmann::json& j) {
  std::unique_ptr<RegisterMap> rmap = std::make_unique<RegisterMap>();
  *rmap = j;
  regmaps.push_back(std::move(rmap));
}

void from_json(const json& j, AddrRange& a) {
  a.range = j;
}
void to_json(json& j, const AddrRange& a) {
  j = a.range;
}

NLOHMANN_JSON_SERIALIZE_ENUM(
    RegisterEndian,
    {
        {RegisterEndian::BIG, "B"},
        {RegisterEndian::LITTLE, "L"},
    })

NLOHMANN_JSON_SERIALIZE_ENUM(
    RegisterValueType,
    {
        {RegisterValueType::HEX, "RAW"},
        {RegisterValueType::STRING, "STRING"},
        {RegisterValueType::INTEGER, "INTEGER"},
        {RegisterValueType::FLOAT, "FLOAT"},
        {RegisterValueType::FLAGS, "FLAGS"},
    })

NLOHMANN_JSON_SERIALIZE_ENUM(
    Parity,
    {{Parity::EVEN, "EVEN"}, {Parity::ODD, "ODD"}, {Parity::NONE, "NONE"}})

void from_json(const json& j, RegisterDescriptor& i) {
  j.at("begin").get_to(i.begin);
  j.at("length").get_to(i.length);
  j.at("name").get_to(i.name);
  i.keep = j.value("keep", 1);
  i.storeChangesOnly = j.value("changes_only", false);
  i.endian = j.value("endian", RegisterEndian::BIG);
  i.format = j.value("format", RegisterValueType::HEX);
  if (i.format == RegisterValueType::FLOAT) {
    j.at("precision").get_to(i.precision);
    i.scale = j.value("scale", 1.0);
    i.shift = j.value("shift", 0.0);
  } else if (i.format == RegisterValueType::FLAGS) {
    j.at("flags").get_to(i.flags);
    for (const auto& [pos, name] : i.flags) {
      if ((pos / 16) >= i.length) {
        throw std::out_of_range(name + " flag bit position would overflow");
      }
    }
  }
}

void to_json(json& j, const RegisterDescriptor& i) {
  j["begin"] = i.begin;
  j["length"] = i.length;
  j["name"] = i.name;
  j["keep"] = i.keep;
  j["changes_only"] = i.storeChangesOnly;
  j["format"] = i.format;
  j["endian"] = i.endian;
  if (i.format == RegisterValueType::FLOAT) {
    j["precision"] = i.precision;
  } else if (i.format == RegisterValueType::FLAGS) {
    j["flags"] = i.flags;
  }
}

void to_json(json& j, const FlagType& m) {
  j["name"] = m.name;
  j["value"] = m.value;
  j["bitOffset"] = m.bitOffset;
}

void to_json(json& j, const RegisterValue& m) {
  static const std::unordered_map<RegisterValueType, std::string> keyMap = {
      {RegisterValueType::INTEGER, "intValue"},
      {RegisterValueType::STRING, "strValue"},
      {RegisterValueType::FLOAT, "floatValue"},
      {RegisterValueType::FLAGS, "flagsValue"},
      {RegisterValueType::HEX, "rawValue"}};
  j["type"] = m.type;
  j["timestamp"] = m.timestamp;
  auto sub = keyMap.at(m.type);
  std::visit([&j, &sub](auto&& v) { j["value"][sub] = v; }, m.value);
}

void to_json(json& j, const Register& m) {
  j["time"] = m.timestamp;
  std::stringstream ss;
  for (auto r : m.value) {
    ss << std::hex << std::setw(4) << std::setfill('0') << r;
  }
  j["data"] = ss.str();
}

void to_json(json& j, const RegisterStoreValue& m) {
  j["regAddress"] = m.regAddr;
  j["name"] = m.name;
  j["history"] = m.history;
}

void to_json(json& j, const RegisterStore& m) {
  std::unique_lock lk(m.historyMutex_);
  j["begin"] = m.regAddr_;
  j["readings"] = {};
  for (const auto& reg : m.history_) {
    if (reg) {
      j["readings"].emplace_back(reg);
    }
  }
}

void from_json(const json& j, WriteActionInfo& action) {
  j.at("interpret").get_to(action.interpret);
  if (j.contains("shell")) {
    action.shell = j.at("shell");
  } else {
    action.shell = std::nullopt;
  }
  if (j.contains("value")) {
    action.value = j.at("value");
  } else {
    action.value = std::nullopt;
  }
  if (!action.shell && !action.value) {
    throw std::runtime_error("Bad special handler");
  }
}

void from_json(const json& j, SpecialHandlerInfo& m) {
  j.at("reg").get_to(m.reg);
  j.at("len").get_to(m.len);
  m.period = j.value("period", -1);
  j.at("action").get_to(m.action);
  if (m.action != "write") {
    throw std::runtime_error("Unsupported action: " + m.action);
  }
  j.at("info").get_to(m.info);
}

void from_json(const json& j, BaudrateConfig& m) {
  m.isSet = true;
  j.at("reg").get_to(m.reg);
  j.at("baud_value_map").get_to(m.baudValueMap);
}

void from_json(const json& j, RegisterMap& m) {
  j.at("address_range").get_to(m.applicableAddresses);
  j.at("probe_register").get_to(m.probeRegister);
  j.at("name").get_to(m.name);
  m.parity = j.value("parity", Parity::EVEN);
  j.at("preferred_baudrate").get_to(m.preferredBaudrate);
  j.at("default_baudrate").get_to(m.defaultBaudrate);
  std::vector<RegisterDescriptor> tmp;
  j.at("registers").get_to(tmp);
  for (auto& i : tmp) {
    m.registerDescriptors[i.begin] = i;
  }
  if (j.contains("special_handlers")) {
    j.at("special_handlers").get_to(m.specialHandlers);
  }
  if (j.contains("baud_config")) {
    j.at("baud_config").get_to(m.baudConfig);
  }
}
void to_json(json& j, const RegisterMap& m) {
  j["address_range"] = m.applicableAddresses;
  j["probe_register"] = m.probeRegister;
  j["name"] = m.name;
  j["preferred_baudrate"] = m.preferredBaudrate;
  j["default_baudrate"] = m.preferredBaudrate;
  j["registers"] = {};
  std::transform(
      m.registerDescriptors.begin(),
      m.registerDescriptors.end(),
      std::back_inserter(j["registers"]),
      [](const auto& kv) { return kv.second; });
}

} // namespace rackmon
