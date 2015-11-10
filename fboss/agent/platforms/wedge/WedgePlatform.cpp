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
#include <glog/logging.h>
#include "fboss/lib/usb/UsbError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/QsfpModule.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/platforms/wedge/WedgePort.h"
#include "fboss/agent/platforms/wedge/WedgeQsfp.h"

#include <future>

DEFINE_string(volatile_state_dir, "/dev/shm/fboss",
              "Directory for storing volatile state");
DEFINE_string(persistent_state_dir, "/var/facebook/fboss",
              "Directory for storing persistent state");
DEFINE_string(mac, "",
              "The local MAC address for this switch");
DEFINE_string(fruid_filepath, "/dev/shm/fboss/fruid.json",
              "File for storing the fruid data");

using folly::MacAddress;
using folly::make_unique;
using folly::Subprocess;
using std::string;

namespace facebook { namespace fboss {

WedgePlatform::WedgePlatform()
  : productInfo_(FLAGS_fruid_filepath) {
  productInfo_.initialize();
  initMode();
  auto config = loadConfig();
  BcmAPI::init(config);
  initLocalMac();
  hw_.reset(new BcmSwitch(this, mode_ == LC ?
        BcmSwitch::HALF_HASH : BcmSwitch::FULL_HASH));
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

  auto add_port = [&](int num) {
      PortID portID(num);
      opennsl_port_t bcmPortNum = num;

      auto port = make_unique<WedgePort>(portID);
      ports.emplace(bcmPortNum, port.get());
      ports_.emplace(portID, std::move(port));
  };

  // Wedge has 16 QSFPs, each mapping to 4 physical ports.
  int portNum = 0;

  auto add_ports = [&](int n_ports) {
    while (n_ports--) {
      ++portNum;
      add_port(portNum);
    }
  };

  auto add_ports_stride = [&](int n_ports, int start, int stride) {
    int curr = start;
    while (n_ports--) {
      add_port(curr);
      curr += stride;
    }
  };

  if (mode_ == WEDGE || mode_ == LC) {
    // Front panel are 16 4x10G ports
    add_ports(16 * 4);
    if (mode_ == LC) {
      // On LC, another 16 ports for back plane ports
      add_ports(16);
    }
  } else if (mode_ == FC) {
    // On FC, 32 40G ports
    add_ports(32);
  } else {
    add_ports_stride(8 * 4, 1, 1);
    add_ports_stride(8 * 4, 34, 1);
    add_ports_stride(8 * 4, 68, 1);
    add_ports_stride(8 * 4, 102, 1);
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

void WedgePlatform::initTransceiverMap(SwSwitch* sw) {
  const int MAX_WEDGE_MODULES = 16;

  // If we can't get access to the USB devices, don't bother to
  // create the QSFP objects;  this is likely to be a permanent
  // error.

  try {
    wedgeI2CBusLock_ = make_unique<WedgeI2CBusLock>();
  } catch (const LibusbError& ex) {
    LOG(ERROR) << "failed to initialize USB to I2C interface";
    return;
  }

  // Wedge port 0 is the CPU port, so the first port associated with
  // a QSFP+ is port 1.  We start the transceiver IDs with 0, though.

  for (int idx = 0; idx < MAX_WEDGE_MODULES; idx++) {
    std::unique_ptr<WedgeQsfp> qsfpImpl =
      make_unique<WedgeQsfp>(idx, wedgeI2CBusLock_.get());
    for (int channel = 0; channel < QsfpModule::CHANNEL_COUNT; ++channel) {
      qsfpImpl->setChannelPort(ChannelID(channel),
          PortID(idx * QsfpModule::CHANNEL_COUNT + channel + 1));
    }
    std::unique_ptr<QsfpModule> qsfp =
      make_unique<QsfpModule>(std::move(qsfpImpl));
    sw->addTransceiver(TransceiverID(idx), std::move(qsfp));
    LOG(INFO) << "making QSFP for " << idx;
    for (int channel = 0; channel < QsfpModule::CHANNEL_COUNT; ++channel) {
      sw->addTransceiverMapping(PortID(idx * QsfpModule::CHANNEL_COUNT +
          channel + 1), ChannelID(channel), TransceiverID(idx));
    }
  }
}

}} // facebook::fboss
