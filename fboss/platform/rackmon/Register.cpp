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
    uint16_t precision) {
  makeInteger(reg, RegisterEndian::BIG);
  int32_t intValue = std::get<int32_t>(value);
  // Y = X / 2^N
  value = float(intValue) / float(1 << precision);
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
    flagsValue.push_back(std::make_tuple(bitVal, name, pos));
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
      makeFloat(reg, desc.precision);
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

Register::operator RegisterValue() const {
  return RegisterValue(value, desc, timestamp);
}

RegisterStore::operator RegisterStoreValue() const {
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

void to_json(json& j, const RegisterValue& m) {
  j["type"] = m.type;
  j["time"] = m.timestamp;
  std::visit([&j](auto&& v) { j["value"] = v; }, m.value);
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
  j["readings"] = m.history;
}

void to_json(json& j, const RegisterStore& m) {
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
