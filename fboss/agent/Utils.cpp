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

#include <folly/FileUtil.h>
#include <folly/Subprocess.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>

#include <boost/filesystem/operations.hpp>
#include <thrift/lib/cpp/util/EnumUtils.h>

using folly::IPAddressV4;
using folly::IPAddressV6;

DEFINE_string(mac, "", "The local MAC address for this switch");
DEFINE_string(mgmt_if, "eth0", "name of management interface");

namespace facebook::fboss {

void utilCreateDir(folly::StringPiece path) {
  try {
    boost::filesystem::create_directories(path.str());
  } catch (...) {
    throw SysError(errno, "failed to create directory \"", path, "\"");
  }
}

IPAddressV4 getSwitchVlanIP(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan) {
  IPAddressV4 switchIp;
  auto vlanInterface = state->getInterfaces()->getInterfaceInVlan(vlan);
  auto& addresses = vlanInterface->getAddresses();
  for (const auto& address : addresses) {
    if (address.first.isV4()) {
      switchIp = address.first.asV4();
      return switchIp;
    }
  }
  throw FbossError("Cannot find IP address for vlan", vlan);
}

IPAddressV6 getSwitchVlanIPv6(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan) {
  IPAddressV6 switchIp;
  auto vlanInterface = state->getInterfaces()->getInterfaceInVlan(vlan);
  auto& addresses = vlanInterface->getAddresses();
  for (const auto& address : addresses) {
    if (address.first.isV6()) {
      switchIp = address.first.asV6();
      return switchIp;
    }
  }
  throw FbossError("Cannot find IPv6 address for vlan ", vlan);
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
  if (!FLAGS_mac.empty()) {
    return folly::MacAddress(FLAGS_mac);
  }

  // TODO(t4543375): Get the base MAC address from the BMC.
  //
  // For now, we take the MAC address from eth0, and enable the
  // "locally administered" bit.  This MAC should be unique, and it's fine for
  // us to use a locally administered address for now.
  std::vector<std::string> cmd{"/sbin/ip", "address", "ls", FLAGS_mgmt_if};
  folly::Subprocess p(cmd, folly::Subprocess::Options().pipeStdout());
  auto out = p.communicate();
  p.waitChecked();
  auto idx = out.first.find("link/ether ");
  if (idx == std::string::npos) {
    throw std::runtime_error("unable to determine local mac address");
  }
  folly::MacAddress eth0Mac(out.first.substr(idx + 11, 17));
  return folly::MacAddress::fromHBO(eth0Mac.u64HBO() | 0x0000020000000000);
}

} // namespace facebook::fboss
