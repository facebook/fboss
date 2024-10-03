// Copyright 2021-present Facebook. All Rights Reserved.
#include "Register.h"
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

using nlohmann::json;

namespace rackmon {

bool AddrRange::contains(uint8_t addr) const {
  for (auto item : range) {
    if (addr >= item.first && addr <= item.second) {
      return true;
    }
  }
  return false;
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

template <class T>
T combineRegister(
    const std::vector<uint16_t>& reg,
    RegisterEndian end,
    bool sign) {
  // Everything in modbus is Big-endian. So when we have a list
  // of registers forming a larger value; For example,
  // a 32bit value would be 2 16bit regs.
  // Then the first register would be the upper nibble of the
  // resulting 32bit value.
  T workValue = 0;
  if (end == BIG) {
    workValue =
        std::accumulate(reg.begin(), reg.end(), 0, [](T ac, uint16_t v) {
          return (ac << 16) + v;
        });

  } else {
    workValue =
        std::accumulate(reg.rbegin(), reg.rend(), 0, [](T ac, uint16_t v) {
          // Swap the bytes
          return (ac << 16) + (((v & 0xff) << 8) | ((v >> 8) & 0xff));
        });
  }
  if (sign) {
    // Ensure we truncate or sign-extend the 16bit value
    // appropriately if our value is 16bits.
    if (reg.size() == 1) {
      workValue = int16_t(workValue);
    } else if (reg.size() == 2) {
      workValue = int32_t(workValue);
    }
  } else {
    // Ensure we correctly interpret unsigned numbers.
    if (reg.size() == 2) {
      workValue = uint32_t(workValue);
    }
  }
  return workValue;
}

void RegisterValue::makeInteger(
    const std::vector<uint16_t>& reg,
    RegisterEndian end,
    bool sign) {
  // There are no practical ways to represent registers larger
  // than 64bits.
  if (reg.size() > 4) {
    throw std::out_of_range("Register does not fit as an integer");
  }
  bool isLong = false;
  if (type == RegisterValueType::LONG || (!sign && reg.size() > 1) ||
      reg.size() > 2) {
    isLong = true;
    // promote an integer to long if we know the value cannot fit
    // in a int32_t.
    if (type == RegisterValueType::INTEGER) {
      type = RegisterValueType::LONG;
    }
  }
  if (isLong) {
    value = combineRegister<int64_t>(reg, end, sign);
  } else {
    value = combineRegister<int32_t>(reg, end, sign);
  }
}

void RegisterValue::makeFloat(
    const std::vector<uint16_t>& reg,
    uint16_t precision,
    float scale,
    float shift,
    bool sign) {
  makeInteger(reg, RegisterEndian::BIG, sign);
  float intValue;
  if (std::holds_alternative<int64_t>(value)) {
    intValue = (float)std::get<int64_t>(value);
  } else {
    intValue = (float)std::get<int32_t>(value);
  }
  // Y = shift + scale * (X / 2^N)
  value = shift + (scale * (intValue / float(1 << precision)));
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
    case RegisterValueType::LONG:
      [[fallthrough]];
    case RegisterValueType::INTEGER:
      makeInteger(reg, desc.endian, desc.sign);
      break;
    case RegisterValueType::FLOAT:
      makeFloat(reg, desc.precision, desc.scale, desc.shift, desc.sign);
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

Register::Register(const RegisterDescriptor& d) : desc(d), value(d.length, 0) {}

Register::Register(const Register& other)
    : desc(other.desc), value(other.value), timestamp(other.timestamp) {}

Register::Register(Register&& other) noexcept
    : desc(other.desc),
      value(std::move(other.value)),
      timestamp(other.timestamp) {}

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

std::vector<uint16_t>::iterator RegisterStore::setRegister(
    std::vector<uint16_t>::iterator start,
    std::vector<uint16_t>::iterator end,
    time_t reloadTime) {
  std::unique_lock lk(historyMutex_);
  auto& reg = front();
  size_t size = reg.value.size();
  if ((reg.value.size() + start) > end) {
    throw std::out_of_range("Source not large enough to set register");
  }
  std::copy(start, start + size, reg.value.begin());
  reg.timestamp = reloadTime;
  ++(*this);
  return start + size;
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

RegisterStoreSpan::RegisterStoreSpan(RegisterStore* reg)
    : spanAddress_(reg->regAddr()),
      interval_(reg->interval()),
      span_(reg->length(), 0),
      registers_{reg},
      timestamp_(reg->back().timestamp) {}

bool RegisterStoreSpan::addRegister(RegisterStore* reg) {
  if (reg->interval() != interval_) {
    return false;
  }
  if (reg->regAddr() != spanAddress_ + span_.size()) {
    return false;
  }
  if (span_.size() + reg->length() > kMaxRegisterSpanLength) {
    return false;
  }
  span_.resize(span_.size() + reg->length());
  registers_.push_back(reg);
  return true;
}

bool RegisterStoreSpan::reloadPending(time_t currentTime) {
  return timestamp_ == 0 || (timestamp_ + interval_) <= currentTime;
}

std::vector<uint16_t>& RegisterStoreSpan::beginReloadSpan() {
  return span_;
}

void RegisterStoreSpan::endReloadSpan(time_t reloadTime) {
  timestamp_ = reloadTime;
  for (auto [source, target] = std::pair(span_.begin(), registers_.begin());
       source != span_.end() && target != registers_.end();
       ++target) {
    source = (*target)->setRegister(source, span_.end(), reloadTime);
  }
}

bool RegisterStoreSpan::buildRegisterSpanList(
    std::vector<RegisterStoreSpan>& list,
    RegisterStore& reg) {
  // Drop it from a plan if not enabled.
  if (!reg.isEnabled()) {
    return false;
  }
  if (!std::any_of(list.begin(), list.end(), [&reg](auto& span) {
        return span.addRegister(&reg);
      })) {
    list.emplace_back(&reg);
  }
  return true;
}

RegisterMapDatabase::Iterator& RegisterMapDatabase::Iterator::operator++() {
  if (it == end) {
    return *this;
  }
  ++it;
  if (addr.has_value()) {
    while (it != end) {
      if ((*it)->applicableAddresses.contains(addr.value())) {
        break;
      }
      ++it;
    }
  }
  return *this;
}

RegisterMapDatabase::Iterator RegisterMapDatabase::find(uint8_t addr) const {
  auto result = std::find_if(
      regmaps.begin(),
      regmaps.end(),
      [addr](const std::unique_ptr<RegisterMap>& m) {
        return m->applicableAddresses.contains(addr);
      });
  return RegisterMapDatabase::Iterator{result, regmaps.cend(), addr};
}

void RegisterMapDatabase::load(const nlohmann::json& j) {
  std::unique_ptr<RegisterMap> rmap = std::make_unique<RegisterMap>();
  *rmap = j;
  regmaps.push_back(std::move(rmap));
}

time_t RegisterMapDatabase::minMonitorInterval() const {
  time_t retVal = RegisterDescriptor::kDefaultInterval;
  for (const auto& regmap : regmaps) {
    for (const auto& desc : regmap->registerDescriptors) {
      if (desc.second.interval < retVal) {
        retVal = desc.second.interval;
      }
    }
  }
  return retVal;
}

void from_json(const json& j, AddrRange& a) {
  for (auto& r : j) {
    a.range.emplace_back(r);
  }
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
        {RegisterValueType::LONG, "LONG"},
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
  i.sign = j.value("sign", false);
  if (j.contains("interval")) {
    j.at("interval").get_to(i.interval);
  }
  if (i.format == RegisterValueType::FLOAT) {
    j.at("precision").get_to(i.precision);
    if (j.contains("scale")) {
      j.at("scale").get_to(i.scale);
    }
    if (j.contains("shift")) {
      j.at("shift").get_to(i.shift);
    }
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
      {RegisterValueType::LONG, "longValue"},
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

void from_json(const json& j, RegisterMap& m) {
  j.at("address_range").get_to(m.applicableAddresses);
  j.at("probe_register").get_to(m.probeRegister);
  j.at("name").get_to(m.name);
  m.parity = j.value("parity", Parity::EVEN);
  j.at("baudrate").get_to(m.baudrate);
  std::vector<RegisterDescriptor> tmp;
  j.at("registers").get_to(tmp);
  for (auto& i : tmp) {
    m.registerDescriptors[i.begin] = i;
  }
  if (j.contains("special_handlers")) {
    j.at("special_handlers").get_to(m.specialHandlers);
  }
}
void to_json(json& j, const RegisterMap& m) {
  j["address_range"] = m.applicableAddresses;
  j["probe_register"] = m.probeRegister;
  j["name"] = m.name;
  j["baudrate"] = m.baudrate;
  j["registers"] = {};
  std::transform(
      m.registerDescriptors.begin(),
      m.registerDescriptors.end(),
      std::back_inserter(j["registers"]),
      [](const auto& kv) { return kv.second; });
}

} // namespace rackmon
