// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <array>
#include <cstdint>
#include <exception>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace rackmon {

struct CRCError : public std::runtime_error {
  CRCError(uint16_t exp, uint16_t got)
      : std::runtime_error(
            "CRC Exception: " + std::to_string(got) +
            " != " + std::to_string(exp)) {}
};

struct Msg {
  // Modbus defines a max size of 253
  static constexpr size_t kMaxModbusLength = 253;
  std::array<uint8_t, kMaxModbusLength> raw;
  size_t len = 0;
  uint8_t& addr = raw[0];

  virtual ~Msg() {}

  void clear() {
    len = 0;
  }
  // Push to the end
  Msg& operator<<(uint8_t d);
  Msg& operator<<(uint16_t d);
  Msg& operator<<(uint32_t d);
  template <typename T>
  Msg& operator<<(std::vector<T>& d) {
    for (T v : d) {
      *this << v;
    }
    return *this;
  }

  // Pop from the end.
  Msg& operator>>(uint8_t& d);
  Msg& operator>>(uint16_t& d);
  Msg& operator>>(uint32_t& d);
  template <typename T>
  Msg& operator>>(std::vector<T>& d) {
    for (auto it = d.rbegin(); it != d.rend(); it++) {
      *this >> *it;
    }
    return *this;
  }

  constexpr auto begin() noexcept {
    return raw.begin();
  }
  auto end() noexcept {
    return raw.begin() + len;
  }
  constexpr auto begin() const noexcept {
    return raw.cbegin();
  }
  auto end() const noexcept {
    return raw.cbegin() + len;
  }
  Msg() {}
  Msg(const Msg& other) {
    len = other.len;
    std::copy(other.begin(), other.end(), begin());
  }
  template <size_t size>
  explicit Msg(const std::array<uint8_t, size>& other) {
    if (size > raw.size()) {
      throw std::overflow_error("array too big");
    }
    len = size;
    std::copy(other.begin(), other.end(), raw.begin());
  }
  Msg& operator=(const Msg& other) {
    len = other.len;
    std::copy(other.begin(), other.end(), begin());
    return *this;
  }
  bool operator==(const Msg& other) const {
    if (len != other.len) {
      return false;
    }
    return std::equal(begin(), end(), other.raw.begin());
  }
  bool operator!=(const Msg& other) const {
    return !(*this == other);
  }

 protected:
  void finalize();
  void validate();

  // Truely private. Used by finalize() and validate().
  uint16_t crc16();

  // Modbus will call encode before transmitting and
  // decode right after receiving a response. Marking
  // this private and having Modbus as a friend to
  // avoid someone accidentally calling encode() or
  // decode() from the upper layers which can cause
  // checksums etc to be recomputed thus destroying
  // the message.
  virtual void encode() {
    finalize();
  }
  virtual void decode() {
    validate();
  }
  friend class Modbus;
  friend class Encoder;
};

// Allows for a more understandable test failures
// from gtest and potential debug prints.
std::ostream& operator<<(std::ostream& os, const Msg& msg);

// This template eventually allows us to use constant expressions
// for Msg, example: `Msg msg = 0x12ab_M;`
// We use this as a intermediate form of the constant expression
// to convert the chars into a byte array.
template <char... cs>
struct ConstMsg {
  template <typename In, typename Out>
  static constexpr auto parse_hex(In begin, In end, Out out) {
    auto hexByte = [](char u, char l) constexpr {
      auto hexNibble = [](char b) constexpr {
        if (b >= '0' && b <= '9') {
          return b - '0';
        }
        if (b >= 'A' && b <= 'F') {
          return 10 + (b - 'A');
        }
        if (b >= 'a' && b <= 'f') {
          return 10 + (b - 'a');
        }
        return 0;
      };
      return hexNibble(u) << 4 | hexNibble(l);
    };
    if (end - begin <= 2) {
      throw std::underflow_error("Literal too short");
    }
    if (begin[0] != '0' || begin[1] != 'x') {
      throw std::invalid_argument("literal does not start with 0x");
    }
    if ((end - begin) % 2 != 0) {
      throw std::invalid_argument("literal has odd length");
    }
    begin += 2;
    while (begin != end) {
      *out = hexByte(*begin, *(begin + 1));
      begin += 2;
      ++out;
    }
    return out;
  }
  static constexpr auto to_array() {
    constexpr std::array<char, sizeof...(cs)> data{cs...};
    std::array<std::uint8_t, (sizeof...(cs) / 2 - 1)> result{};
    parse_hex(data.begin(), data.end(), result.begin());
    return result;
  }
  constexpr operator std::array<std::uint8_t, (sizeof...(cs) / 2)>() const {
    return to_array();
  }
  operator Msg() const {
    constexpr auto tmp = to_array();
    return Msg(tmp);
  }
};

// User defined constant expression. Use ConstMsg as an intermediate
// store to hold the bytes and then convert to Msg.
template <char... cs>
auto operator"" _M() {
  static_assert(sizeof...(cs) % 2 == 0, "Must be an even number of chars");
  Msg msg = ConstMsg<cs...>{};
  return msg;
}

#ifdef __TEST__
class Encoder {
 public:
  Encoder() {}
  static void encode(Msg& msg) {
    msg.encode();
  }
  static void decode(Msg& msg) {
    msg.decode();
  }
  static void finalize(Msg& msg) {
    msg.finalize();
  }
};

// User defined constant expression. Use ConstMsg as an intermediate
// store to hold the bytes and then convert to Msg.
template <char... cs>
auto operator"" _EM() {
  Msg msg = ConstMsg<cs...>{};
  Encoder::encode(msg);
  return msg;
}
#endif

} // namespace rackmon
