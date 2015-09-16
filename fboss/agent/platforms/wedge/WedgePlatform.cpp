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

#include <future>

DEFINE_string(volatile_state_dir, "/dev/shm/fboss",
              "Directory for storing volatile state");
DEFINE_string(persistent_state_dir, "/var/facebook/fboss",
              "Directory for storing persistent state");
DEFINE_string(mac, "",
              "The local MAC address for this switch");
DEFINE_string(fruid_filepath, "/dev/shm/fboss/fruid.json",
              "File for storing the fruid data");
DEFINE_string(mode, "wedge",
              "The mode the FBOSS controller is running as, wedge, lc, or fc");

using folly::MacAddress;
using folly::make_unique;
using folly::Subprocess;
using std::string;

namespace facebook { namespace fboss {

WedgePlatform::WedgePlatform()
  : productInfo_(FLAGS_fruid_filepath) {
  auto config = loadConfig();
  BcmAPI::init(config);
  initLocalMac();
  hw_.reset(new BcmSwitch(this));
  productInfo_.initialize();
}

WedgePlatform::~WedgePlatform() {
}

HwSwitch* WedgePlatform::getHwSwitch() const {
  return hw_.get();
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
}

void WedgePlatform::getProductInfo(ProductInfo& info) {
  productInfo_.getInfo(info);
}

WedgePlatform::InitPortMap WedgePlatform::initPorts() {
  InitPortMap ports;
  enum {
    WEDGE,
    LC,
    FC,
  } mode;

  if (FLAGS_mode == "wedge") {
    mode = WEDGE;
  } else if (FLAGS_mode == "lc") {
    mode = LC;
  } else if (FLAGS_mode == "fc") {
    mode = FC;
  } else {
    throw std::runtime_error("invalide mode " + FLAGS_mode);
  }
  // Wedge has 16 QSFPs, each mapping to 4 physical ports.
  int portNum = 0;

  auto add_ports = [&](int n_ports) {
    while (n_ports--) {
      ++portNum;
      PortID portID(portNum);
      opennsl_port_t bcmPortNum = portNum;

      auto port = make_unique<WedgePort>(portID);
      ports.emplace(bcmPortNum, port.get());
      ports_.emplace(portID, std::move(port));
    }
  };

  if (mode == WEDGE || mode == LC) {
    // Front panel are 16 4x10G ports
    add_ports(16 * 4);
    if (mode == LC) {
      // On LC, another 16 ports for back plane ports
      add_ports(16);
    }
  } else {
    // On FC, 32 40G ports
    add_ports(32);
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
