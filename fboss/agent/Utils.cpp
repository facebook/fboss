/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Utils.h"

#include <sys/resource.h>
#include <sys/syscall.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

#include <folly/FileUtil.h>
#include <folly/Subprocess.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>

#include <boost/filesystem/operations.hpp>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include <re2/re2.h>
#include <chrono>
#include <iostream>

using folly::IPAddressV4;
using folly::IPAddressV6;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

DEFINE_string(mac, "", "The local MAC address for this switch");
DEFINE_string(mgmt_if, "eth0", "name of management interface");
DEFINE_uint64(
    ingress_egress_buffer_pool_size,
    0,
    "Common ingress/egress buffer pool size override");

namespace facebook::fboss {

namespace {

UnicastRoute makeUnicastRouteHelper(
    const folly::CIDRNetwork& nw,
    RouteForwardAction action,
    AdminDistance admin) {
  UnicastRoute nr;
  nr.dest() = toIpPrefix(nw);
  nr.action() = action;
  nr.adminDistance() = admin;
  return nr;
}

template <typename AddrT>
AddrT getIPAddress(InterfaceID intfID, const Interface::AddressesType addrs) {
  for (auto iter : std::as_const(*addrs)) {
    auto address = folly::IPAddress(iter.first);
    if constexpr (std::is_same_v<AddrT, IPAddressV4>) {
      if (address.isV4()) {
        return address.asV4();
      }
    } else {
      if (address.isV6()) {
        return address.asV6();
      }
    }
  }
  throw FbossError("Cannot find IP address for interface ", intfID);
}

} // namespace
void utilCreateDir(folly::StringPiece path) {
  try {
    boost::filesystem::create_directories(path.str());
  } catch (...) {
    throw SysError(errno, "failed to create directory \"", path, "\"");
  }
}

IPAddressV4 getAnyIntfIP(const std::shared_ptr<SwitchState>& state) {
  IPAddressV4 intfIp;
  for (auto intfIter : std::as_const(*state->getInterfaces())) {
    const auto& intf = intfIter.second;
    for (auto iter : std::as_const(*intf->getAddresses())) {
      auto address = folly::IPAddress(iter.first);
      if (address.isV4()) {
        return address.asV4();
      }
    }
  }
  throw FbossError("Cannot find IPv4 address on any interface");
}

IPAddressV6 getAnyIntfIPv6(const std::shared_ptr<SwitchState>& state) {
  IPAddressV6 intfIp;
  for (auto intfIter : std::as_const(*state->getInterfaces())) {
    const auto& intf = intfIter.second;
    for (auto iter : std::as_const(*intf->getAddresses())) {
      auto address = folly::IPAddress(iter.first);
      if (address.isV6()) {
        return address.asV6();
      }
    }
  }
  throw FbossError("Cannot find IPv6 address on any interface");
}

IPAddressV4 getSwitchVlanIP(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan) {
  IPAddressV4 switchIp;
  auto vlanInterface = state->getInterfaces()->getInterfaceInVlan(vlan);

  return getIPAddress<IPAddressV4>(
      vlanInterface->getID(), vlanInterface->getAddresses());
}

IPAddressV4 getSwitchIntfIP(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intfID) {
  auto interface = state->getInterfaces()->getInterface(intfID);

  return getIPAddress<IPAddressV4>(intfID, interface->getAddresses());
}

IPAddressV6 getSwitchVlanIPv6(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan) {
  auto vlanInterface = state->getInterfaces()->getInterfaceInVlan(vlan);

  return getIPAddress<IPAddressV6>(
      vlanInterface->getID(), vlanInterface->getAddresses());
}

IPAddressV6 getSwitchIntfIPv6(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intfID) {
  auto interface = state->getInterfaces()->getInterface(intfID);

  return getIPAddress<IPAddressV6>(intfID, interface->getAddresses());
}

void incNiceValue(const uint32_t increment) {
  if (increment == 0) {
    XLOG(WARNING) << "Nice value increment is 0. Returning now.";
    return;
  }
  pid_t tid;
  tid = syscall(SYS_gettid);
  // Need to reset errno because getpriority sometimes returns -1 for real
  auto oldErrno = errno;
  errno = 0;

  int niceness = getpriority(PRIO_PROCESS, tid);
  if (niceness == -1 && errno) {
    XLOG(ERR) << "Error while getting current thread niceness: "
              << strerror(errno);
  } else if (setpriority(PRIO_PROCESS, tid, niceness + increment) == -1) {
    XLOG(ERR) << "Error while setting thread niceness: " << strerror(errno);
  }

  errno = oldErrno;
}

bool dumpStateToFile(const std::string& filename, const folly::dynamic& json) {
  return folly::writeFile(folly::toPrettyJson(json), filename.c_str());
}

bool isValidThriftStateFile(
    const std::string& follyStateFileName,
    const std::string& thriftStateFileName) {
  // Folly state is dumped before thrift state. Hence if folly state has a newer
  // timestamp than thrift state, thrift state should be dumped by previous
  // warmboot exits. This could happen during trunk->prod->trunk or
  // prod->trunk->prod.
  if (boost::filesystem::exists(thriftStateFileName) &&
      boost::filesystem::last_write_time(thriftStateFileName) >=
          boost::filesystem::last_write_time(follyStateFileName)) {
    return true;
  }
  XLOG(DBG2) << "Thrift state file does not exist or has timestamp older "
             << "than folly state. Using only folly file.";
  return false;
}

std::string getLocalHostname() {
  const size_t kHostnameMaxLen = 256; // from gethostname man page
  char hostname[kHostnameMaxLen];
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    XLOG(ERR) << "gethostname() failed";
    return "";
  }

  return hostname;
}

std::vector<ClientID> AllClientIDs() {
  return std::vector<ClientID>{
      ClientID::BGPD,
      ClientID::STATIC_ROUTE,
      ClientID::INTERFACE_ROUTE,
      ClientID::LINKLOCAL_ROUTE,
      ClientID::STATIC_INTERNAL,
      ClientID::OPENR,
  };
}

std::string switchRunStateStr(SwitchRunState runState) {
  return apache::thrift::util::enumNameSafe(runState);
}

folly::MacAddress getLocalMacAddress() {
  static std::mutex macComputeLock;
  static std::optional<folly::MacAddress> localMac;
  std::lock_guard g(macComputeLock);
  if (localMac) {
    return *localMac;
  }
  if (!FLAGS_mac.empty()) {
    localMac = folly::MacAddress(FLAGS_mac);
    return *localMac;
  }
  // TODO(t4543375): Get the base MAC address from the BMC.
  //
  // For now, we take the MAC address from eth0, and enable the
  // "locally administered" bit.  This MAC should be unique, and it's fine for
  // us to use a locally administered address for now.
  steady_clock::time_point begin = steady_clock::now();
  std::vector<std::string> cmd{"/sbin/ip", "address", "ls", FLAGS_mgmt_if};
  folly::Subprocess p(cmd, folly::Subprocess::Options().pipeStdout());
  auto out = p.communicate();
  p.waitChecked();
  auto idx = out.first.find("link/ether ");
  if (idx == std::string::npos) {
    throw std::runtime_error("unable to determine local mac address");
  }
  folly::MacAddress eth0Mac(out.first.substr(idx + 11, 17));
  localMac = folly::MacAddress::fromHBO(eth0Mac.u64HBO() | 0x0000020000000000);
  XLOG(DBG2) << "Calculating local mac address=" << *localMac << " took "
             << duration_cast<milliseconds>(steady_clock::now() - begin).count()
             << "ms";
  return *localMac;
}

std::vector<NextHopThrift> thriftNextHopsFromAddresses(
    const std::vector<network::thrift::BinaryAddress>& addrs) {
  std::vector<NextHopThrift> nhs;
  nhs.reserve(addrs.size());
  for (const auto& addr : addrs) {
    NextHopThrift nh;
    nh.address() = addr;
    nh.weight() = 0;
    nhs.emplace_back(std::move(nh));
  }
  return nhs;
}

IpPrefix toIpPrefix(const folly::CIDRNetwork& nw) {
  IpPrefix pfx;
  pfx.ip() = network::toBinaryAddress(nw.first);
  pfx.prefixLength() = nw.second;
  return pfx;
}

UnicastRoute makeDropUnicastRoute(
    const folly::CIDRNetwork& nw,
    AdminDistance admin) {
  return makeUnicastRouteHelper(nw, RouteForwardAction::DROP, admin);
}
UnicastRoute makeToCpuUnicastRoute(
    const folly::CIDRNetwork& nw,
    AdminDistance admin) {
  return makeUnicastRouteHelper(nw, RouteForwardAction::TO_CPU, admin);
}
UnicastRoute makeUnicastRoute(
    const folly::CIDRNetwork& nw,
    const std::vector<folly::IPAddress>& nhops,
    AdminDistance admin) {
  auto route = makeUnicastRouteHelper(nw, RouteForwardAction::NEXTHOPS, admin);
  std::vector<network::thrift::BinaryAddress> addrs;
  addrs.reserve(nhops.size());
  std::for_each(
      nhops.begin(), nhops.end(), [&addrs](const folly::IPAddress& ip) {
        addrs.emplace_back(facebook::network::toBinaryAddress(ip));
      });
  route.nextHops() = thriftNextHopsFromAddresses(addrs);
  return route;
}

PortID getPortID(
    SystemPortID sysPortId,
    const std::shared_ptr<SwitchState>& state) {
  auto mySwitchId = state->getSwitchSettings()->getSwitchId();
  CHECK(mySwitchId.has_value());
  auto sysPortRange = state->getDsfNodes()
                          ->getDsfNodeIf(SwitchID(*mySwitchId))
                          ->getSystemPortRange();
  CHECK(sysPortRange.has_value());
  return PortID(static_cast<int64_t>(sysPortId) - *sysPortRange->minimum());
}

SystemPortID getSystemPortID(
    const PortID& portId,
    const std::shared_ptr<SwitchState>& state) {
  auto switchId = state->getSwitchSettings()->getSwitchId();
  CHECK(switchId.has_value());
  assert(
      state->getSwitchSettings()->getSwitchType(*switchId) ==
      cfg::SwitchType::VOQ);

  auto mySwitchId = state->getSwitchSettings()->getSwitchId();
  CHECK(mySwitchId.has_value());
  auto sysPortRange = state->getDsfNodes()
                          ->getDsfNodeIf(SwitchID(*mySwitchId))
                          ->getSystemPortRange();
  CHECK(sysPortRange.has_value());
  auto systemPortId = static_cast<int64_t>(portId) + *sysPortRange->minimum();
  CHECK_LE(systemPortId, *sysPortRange->maximum());
  return SystemPortID(systemPortId);
}

std::vector<PortID> getPortsForInterface(
    InterfaceID intfId,
    const std::shared_ptr<SwitchState>& state) {
  auto intf = state->getInterfaces()->getInterfaceIf(intfId);
  if (!intf) {
    return {};
  }
  std::vector<PortID> ports;
  switch (intf->getType()) {
    case cfg::InterfaceType::VLAN: {
      auto vlanId = intf->getVlanID();
      auto vlan = state->getVlans()->getVlanIf(vlanId);
      if (vlan) {
        for (const auto& memberPort : vlan->getPorts()) {
          ports.push_back(PortID(memberPort.first));
        }
      }
    } break;
    case cfg::InterfaceType::SYSTEM_PORT:
      ports.push_back(getPortID(intf->getSystemPortID().value(), state));
      break;
  }
  return ports;
}

bool isAnyInterfacePortInLoopbackMode(
    std::shared_ptr<SwitchState> swState,
    const std::shared_ptr<Interface> interface) {
  // walk all ports for the given interface and ensure that there are no
  // loopbacks configured This is mostly for the agent tests for which we dont
  // want to flood grat arp when we are in loopback resulting in these pkts
  // getting looped back forever
  for (auto portId : getPortsForInterface(interface->getID(), swState)) {
    auto port = swState->getPorts()->getPortIf(portId);
    if (port && port->getLoopbackMode() != cfg::PortLoopbackMode::NONE) {
      XLOG(DBG2) << "Port: " << port->getName()
                 << " in interface: " << interface->getID()
                 << " is in loopback mode";
      return true;
    }
  }
  return false;
}

StopWatch::StopWatch(std::optional<std::string> name, bool json)
    : name_(name), json_(json), startTime_(std::chrono::steady_clock::now()) {}

StopWatch::~StopWatch() {
  if (!name_) {
    return;
  }
  auto durationMillseconds = msecsElapsed().count();
  if (json_) {
    folly::dynamic time = folly::dynamic::object;
    time[*name_] = durationMillseconds;
    std::cout << time << std::endl;
  } else {
    XLOG(DBG2) << *name_ << " : " << durationMillseconds;
  }
}

void enableExactMatch(std::string& yamlCfg) {
  std::string globalSt("global:\n");
  std::string emSt("fpem_mem_entries:");
  std::string emWidthSt("fpem_mem_entries_width:");
  std::size_t glPos = yamlCfg.find(globalSt);
  std::size_t emPos = yamlCfg.find(emSt);
  std::size_t emWidthPos = yamlCfg.find(emWidthSt);
  static const re2::RE2 emPattern(
      "(fpem_mem_entries: )(0x[0-9a-fA-F]+|[0-9]+)(\n)");
  static const re2::RE2 emWidthPattern(
      "(fpem_mem_entries_width: )(0x[0-9a-fA-F]+|[0-9]+)(\n)");
  if (glPos != std::string::npos) {
    if (emPos == std::string::npos && emWidthPos == std::string::npos) {
      yamlCfg.replace(
          glPos,
          globalSt.length(),
          "global:\n      fpem_mem_entries_width: 1\n      fpem_mem_entries: 65536\n");
    } else if (emPos != std::string::npos && emWidthPos == std::string::npos) {
      yamlCfg.replace(
          glPos,
          globalSt.length(),
          "global:\n      fpem_mem_entries_width: 1\n");
      re2::RE2::Replace(&yamlCfg, emPattern, "fpem_mem_entries: 65536\n");
    } else {
      if (emPos != std::string::npos) {
        re2::RE2::Replace(&yamlCfg, emPattern, "fpem_mem_entries: 65536\n");
      }
      if (emWidthPos != std::string::npos) {
        re2::RE2::Replace(
            &yamlCfg, emWidthPattern, "fpem_mem_entries_width: 1\n");
      }
    }
  }
}

std::shared_ptr<NdpEntry> getNeighborEntryForIP(
    const std::shared_ptr<SwitchState>& state,
    const std::shared_ptr<Interface>& intf,
    const folly::IPAddressV6& ipAddr) {
  std::shared_ptr<NdpEntry> entry{nullptr};

  switch (intf->getType()) {
    case cfg::InterfaceType::VLAN: {
      auto vlanID = intf->getVlanID();
      auto vlan = state->getVlans()->getVlanIf(vlanID);
      if (vlan) {
        entry = vlan->getNdpTable()->getEntryIf(ipAddr);
      }
      break;
    }
    case cfg::InterfaceType::SYSTEM_PORT: {
      const auto& nbrTable = intf->getNdpTable();
      entry = nbrTable->getEntryIf(ipAddr);
      break;
    }
  }

  return entry;
}

} // namespace facebook::fboss
