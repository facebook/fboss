// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <vector>
#include "Modbus.h"
#include "Register.h"

namespace rackmon {

struct DeviceLocation {
  uint8_t addr;
  Modbus& interface;
  bool operator<(const DeviceLocation& other) const {
    if (addr != other.addr) {
      return addr < other.addr;
    }
    return interface.name() < other.interface.name();
  }

  friend std::ostream& operator<<(std::ostream& os, const DeviceLocation& me) {
    os << "0x" << std::hex << +me.addr;
    if (me.interface.getPort().has_value()) {
      os << " on port: " << std::dec << (int)me.interface.getPort().value();
    }
    return os;
  }
};

class DeviceLocationIterator {
  using iterator_category = std::forward_iterator_tag;
  using value_type = DeviceLocation;

 private:
  const std::vector<std::unique_ptr<RegisterMap>>& regmaps;
  const std::vector<std::unique_ptr<Modbus>>& interfaces;
  size_t regMapIndex = 0;
  size_t rangeIndex = 0;
  uint8_t rangeCounter = 0;
  size_t interfacesIndex = 0;

  DeviceLocationIterator(
      const std::vector<std::unique_ptr<RegisterMap>>& regmaps,
      const std::vector<std::unique_ptr<Modbus>>& interfaces,
      size_t regMapIndex)
      : regmaps(regmaps), interfaces(interfaces), regMapIndex(regMapIndex) {}

 public:
  DeviceLocationIterator() = delete;

  DeviceLocationIterator(
      const RegisterMapDatabase& registerMapDb,
      const std::vector<std::unique_ptr<Modbus>>& interfaces)
      : regmaps(registerMapDb.regmaps), interfaces(interfaces) {
    if (regmaps.empty()) {
      throw std::runtime_error("Empty register map is not allowed");
    }
    if (interfaces.empty()) {
      throw std::runtime_error("Empty interface list is not allowed");
    }
  }

  DeviceLocation operator*() {
    return {
        static_cast<uint8_t>(
            regmaps.at(regMapIndex)
                ->applicableAddresses.range.at(rangeIndex)
                .first +
            rangeCounter),
        *(interfaces.at(interfacesIndex).get())};
  }

  DeviceLocationIterator end() const {
    return DeviceLocationIterator(regmaps, interfaces, regmaps.size());
  }

  DeviceLocationIterator& operator++() {
    if (*this == this->end()) {
      throw std::out_of_range("There are no more device locations");
    }

    if (++interfacesIndex == interfaces.size()) {
      interfacesIndex = 0;

      auto curRange = regmaps.at(regMapIndex)->applicableAddresses.range;
      uint8_t start = curRange.at(rangeIndex).first;
      uint8_t finish = curRange.at(rangeIndex).second;
      if (++rangeCounter + start > finish) {
        rangeCounter = 0;

        if (++rangeIndex >= curRange.size()) {
          rangeIndex = 0;

          ++regMapIndex;
        }
      }
    }
    return *this;
  }

  bool operator==(const DeviceLocationIterator& other) const {
    return &regmaps == &other.regmaps && &interfaces == &other.interfaces &&
        regMapIndex == other.regMapIndex && rangeIndex == other.rangeIndex &&
        rangeCounter == other.rangeCounter &&
        interfacesIndex == other.interfacesIndex;
  }

  bool operator!=(const DeviceLocationIterator& other) const {
    return !(*this == other);
  }
};

} // namespace rackmon
