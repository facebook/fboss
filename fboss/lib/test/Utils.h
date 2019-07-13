#pragma once
#include "common/network/IPAddressV4.h"
#include "common/network/IPAddressV6.h"

// Utility structs used in multiple test files
struct Prefix4 {
  Prefix4(const facebook::network::IPAddressV4& _ip, uint8_t _mask)
      : ip(_ip), mask(_mask) {}
  bool operator<(const Prefix4& r) const {
    return ip < r.ip || (ip == r.ip && mask < r.mask);
  }
  facebook::network::IPAddressV4 ip;
  uint8_t mask;
};

struct Prefix6 {
  Prefix6(const facebook::network::IPAddressV6& _ip, uint8_t _mask)
      : ip(_ip), mask(_mask) {}
  bool operator<(const Prefix6& r) const {
    return ip < r.ip || (ip == r.ip && mask < r.mask);
  }
  facebook::network::IPAddressV6 ip;
  uint8_t mask;
};
