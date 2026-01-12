/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/platform_checks/checks/MacAddressCheck.h"

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <memory>
#include <stdexcept>

#include <folly/logging/xlog.h>
#include "fboss/platform/weutil/ConfigUtils.h"
#include "fboss/platform/weutil/FbossEepromInterface.h"

namespace facebook::fboss::platform::platform_checks {

CheckResult MacAddressCheck::run() {
  try {
    auto eth0Mac = getMacAddress("eth0");
    if (eth0Mac == folly::MacAddress::ZERO) {
      return makeError("eth0 interface has zero MAC address");
    }

    auto eepromMacList = getEepromMacAddressList();
    if (eepromMacList.empty()) {
      return makeError("No EEPROM MAC address found");
    }

    for (const auto& [eepromName, eepromMac] : eepromMacList) {
      if (eth0Mac != eepromMac) {
        std::string errorMsg =
            "MAC address mismatch: eth0=" + eth0Mac.toString() + " " +
            eepromName + "=" + eepromMac.toString();
        std::string remediationMsg = "RMA device to correct MAC address";
        return makeProblem(
            errorMsg, RemediationType::RMA_REQUIRED, remediationMsg);
      }
    }
  } catch (const std::exception& ex) {
    return makeError("Unexpected error: " + std::string(ex.what()));
  }
  return makeOK();
}

folly::MacAddress MacAddressCheck::getMacAddress(const std::string& interface) {
  auto sock_deleter = [](int* fd) {
    if (*fd >= 0) {
      close(*fd);
    }
    delete fd;
  };
  std::unique_ptr<int, decltype(sock_deleter)> sock(
      new int(socket(AF_INET, SOCK_DGRAM, 0)), sock_deleter);
  if (*sock < 0) {
    throw std::runtime_error("Failed to create socket");
  }

  struct ifreq ifr = {};
  std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
  if (ioctl(*sock, SIOCGIFHWADDR, &ifr) < 0) {
    throw std::runtime_error("Failed to get mac address");
  }
  if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
    throw std::runtime_error("Invalid mac address");
  }
  return folly::MacAddress::fromBinary(
      {reinterpret_cast<const unsigned char*>(ifr.ifr_hwaddr.sa_data), 6});
}

std::unordered_map<std::string, folly::MacAddress>
MacAddressCheck::getEepromMacAddressList() {
  std::unordered_map<std::string, folly::MacAddress> eepromMacList;
  auto fruEepromList = weutil::ConfigUtils().getFruEepromList();

  for (const auto& [eepromName, eeprom] : fruEepromList) {
    FbossEepromInterface eepromInterface(eeprom.path, eeprom.offset);
    std::string eepromMacStr =
        (*eepromInterface.getEepromContents().x86CpuMac());
    eepromMacStr = eepromMacStr.substr(0, eepromMacStr.find(','));
    if (!eepromMacStr.empty()) {
      auto eepromMac = folly::MacAddress::fromString(eepromMacStr);
      if (eepromMac == folly::MacAddress::ZERO) {
        // For Icetea/Icecube/tahansb800bc/ladakh800bcls, the MAC address is
        // actually in CHASSIS_EEPROM, the x86CpuMac field in COME_EEPROM is
        // zero.
        continue;
      }
      eepromMacList[eepromName] = eepromMac;
      XLOG(INFO) << "eepromName: " << eepromName << " x86CpuMac: " << eepromMac;
    }
  }

  return eepromMacList;
}

} // namespace facebook::fboss::platform::platform_checks
