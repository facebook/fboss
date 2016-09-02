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
#include "fboss/lib/usb/WedgeI2CBus.h"
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
DEFINE_string(mgmt_if, "eth0",
              "name of management interface");

using folly::MacAddress;
using folly::make_unique;
using folly::Subprocess;
using std::string;

namespace {
constexpr auto kNumWedge40Qsfps = 16;
}

namespace facebook { namespace fboss {

WedgePlatform::WedgePlatform(std::unique_ptr<WedgeProductInfo> productInfo,
                             uint8_t numQsfps)
    : productInfo_(std::move(productInfo)),
      numQsfpModules_(numQsfps) {
  // Make sure we actually set the number of qsfp modules
  CHECK(numQsfpModules_ > 0);
}

// default to kNumWedge40Qsfps if numQsfps not specified
WedgePlatform::WedgePlatform(std::unique_ptr<WedgeProductInfo> productInfo)
    : WedgePlatform(std::move(productInfo), kNumWedge40Qsfps) {}

void WedgePlatform::init() {
  auto config = loadConfig();
  BcmAPI::init(config);
  initLocalMac();
  auto mode = getMode();
  bool isLc = mode == WedgePlatformMode::LC ||
    mode == WedgePlatformMode::GALAXY_LC;
  hw_.reset(new BcmSwitch(this, isLc ? BcmSwitch::HALF_HASH :
        BcmSwitch::FULL_HASH));
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

void WedgePlatform::onUnitAttach(int unit) {}

void WedgePlatform::getProductInfo(ProductInfo& info) {
  productInfo_->getInfo(info);
}

WedgePlatformMode WedgePlatform::getMode() const {
  return productInfo_->getMode();
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
  std::vector<std::string> cmd{"/sbin/ip", "address", "ls", FLAGS_mgmt_if};
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

std::unique_ptr<BaseWedgeI2CBus> WedgePlatform::getI2CBus() {
  return make_unique<WedgeI2CBus>();
}

void WedgePlatform::initTransceiverMap(SwSwitch* sw) {
  // If we can't get access to the USB devices, don't bother to
  // create the QSFP objects;  this is likely to be a permanent
  // error.

  try {
    wedgeI2CBusLock_ = make_unique<WedgeI2CBusLock>(getI2CBus());
  } catch (const LibusbError& ex) {
    LOG(ERROR) << "failed to initialize USB to I2C interface";
    return;
  }

  // Wedge port 0 is the CPU port, so the first port associated with
  // a QSFP+ is port 1.  We start the transceiver IDs with 0, though.

  for (int idx = 0; idx < numQsfpModules_; idx++) {
    std::unique_ptr<WedgeQsfp> qsfpImpl =
      make_unique<WedgeQsfp>(idx, wedgeI2CBusLock_.get());
    for (int channel = 0; channel < QsfpModule::CHANNEL_COUNT; ++channel) {
      qsfpImpl->setChannelPort(ChannelID(channel),
          PortID(idx * QsfpModule::CHANNEL_COUNT + channel + 1));
    }
    std::unique_ptr<QsfpModule> qsfp =
      make_unique<QsfpModule>(std::move(qsfpImpl));
    auto qsfpPtr = qsfp.get();
    sw->addTransceiver(TransceiverID(idx), move(qsfp));
    LOG(INFO) << "making QSFP for " << idx;
    for (int channel = 0; channel < QsfpModule::CHANNEL_COUNT; ++channel) {
      PortID portId = fbossPortForQsfpChannel(idx, channel);
      sw->addTransceiverMapping(portId,
          ChannelID(channel), TransceiverID(idx));
      (getPort(portId))->setQsfp(qsfpPtr);
    }
  }
}

PortID WedgePlatform::fbossPortForQsfpChannel(int transceiver, int channel) {
  return PortID(transceiver * QsfpModule::CHANNEL_COUNT + channel + 1);
}

WedgePort* WedgePlatform::getPort(PortID id) {
  auto iter = ports_.find(id);
  if (iter == ports_.end()) {
    throw FbossError("Cannot find the Wedge port object for BCM port ", id);
  }
  return iter->second.get();
}

}} // facebook::fboss
