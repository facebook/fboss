/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgePlatform.h"

#include <folly/Memory.h>
#include <folly/Subprocess.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/platforms/wedge/WedgePort.h"

DEFINE_string(volatile_state_dir, "/dev/shm/fboss",
              "Directory for storing volatile state");
DEFINE_string(persistent_state_dir, "/var/facebook/fboss",
              "Directory for storing persistent state");
DEFINE_string(mac, "",
              "The local MAC address for this switch");

using folly::MacAddress;
using folly::make_unique;
using folly::Subprocess;
using std::string;

namespace facebook { namespace fboss {

WedgePlatform::WedgePlatform() {
  auto config = loadConfig();
  BcmAPI::init(config);
  initLocalMac();
  hw_.reset(new BcmSwitch(this));
}

WedgePlatform::~WedgePlatform() {
}

HwSwitch* WedgePlatform::getHwSwitch() const {
  return hw_.get();
}

void WedgePlatform::initSwSwitch(SwSwitch* sw) {
  // TODO(5049438): Add QSFP support for wedge, and populate the QSFP data
  // structures here.
}

MacAddress WedgePlatform::getLocalMac() const {
  return localMac_;
}

string WedgePlatform::getVolatileStateDir() const {
  return FLAGS_volatile_state_dir;
}

string WedgePlatform::getPersistentStateDir() const {
  return FLAGS_persistent_state_dir;
}

void WedgePlatform::onUnitAttach() {
  // TODO: Load the LED microprocessor code.
}

WedgePlatform::InitPortMap WedgePlatform::initPorts() {
  InitPortMap ports;

  // Wedge has 16 QSFPs, each mapping to 4 physical ports.
  int portNum = 0;
  for (int mod = 0; mod < 16; ++mod) {
    // Eventually we should define objects for each of the QSFPs here.
    for (int channel = 0; channel < 4; ++channel) {
      ++portNum;
      PortID portID(portNum);
      opennsl_port_t bcmPortNum = portNum;

      auto port = make_unique<WedgePort>(portID);
      ports.emplace(bcmPortNum, port.get());
      ports_.emplace(portID, std::move(port));
    }
  }

  return ports;
}

void WedgePlatform::initLocalMac() {
  if (!FLAGS_mac.empty()) {
    localMac_ = MacAddress(FLAGS_mac);
    return;
  }

  // TODO(t4543375): Get the base MAC address from the BMC.
  //
  // For now, we take the MAC address from eth0, and enable the
  // "locally administered" bit.  This MAC should be unique, and it's fine for
  // us to use a locally administered address for now.
  std::vector<std::string> cmd{"/sbin/ip", "address", "ls", "eth0"};
  Subprocess p(cmd, Subprocess::pipeStdout());
  auto out = p.communicate();
  p.waitChecked();
  auto idx = out.first.find("link/ether ");
  if (idx == std::string::npos) {
    throw std::runtime_error("unable to determine local mac address");
  }
  MacAddress eth0Mac(out.first.substr(idx + 11, 17));
  localMac_ = MacAddress::fromHBO(eth0Mac.u64HBO() | 0x0000020000000000);
}

}} // facebook::fboss
