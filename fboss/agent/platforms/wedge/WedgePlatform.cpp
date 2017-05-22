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
DEFINE_string(mgmt_if, "eth0",
              "name of management interface");

using folly::MacAddress;
using std::make_unique;
using folly::Subprocess;
using std::string;

namespace {
constexpr auto kNumWedge40Qsfps = 16;
}

namespace facebook { namespace fboss {

WedgePlatform::WedgePlatform(std::unique_ptr<WedgeProductInfo> productInfo)
    : productInfo_(std::move(productInfo)) {}

void WedgePlatform::init() {
  auto config = loadConfig();
  BcmAPI::init(config);
  initLocalMac();
  auto mode = getMode();
  bool isLc = (mode == WedgePlatformMode::GALAXY_LC);
  hw_.reset(new BcmSwitch(this, isLc ? BcmSwitch::HALF_HASH :
        BcmSwitch::FULL_HASH));
}

WedgePlatform::~WedgePlatform() {}

WedgePlatform::InitPortMap WedgePlatform::initPorts() {
  portMapping_ = createPortMapping();

  InitPortMap mapping;
  for (const auto& kv: *portMapping_) {
    opennsl_port_t id = kv.first;
    mapping[id] = kv.second.get();
  }
  return mapping;
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
  Subprocess p(cmd, Subprocess::Options().pipeStdout());
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

TransceiverIdxThrift WedgePlatform::getPortMapping(PortID portId) const {
  auto info = TransceiverIdxThrift();
  auto port = getPort(portId);

  auto transceiver = port->getTransceiverID();
  if (transceiver) {
    info.transceiverId = static_cast<int32_t>(*transceiver);
    info.__isset.transceiverId = true;
  }
  auto channel = port->getChannel();
  if (channel) {
    info.channelId = static_cast<int32_t>(*channel);
    info.__isset.channelId = true;
  }
  return info;
}

WedgePort* WedgePlatform::getPort(PortID id) const {
  return portMapping_->getPort(id);
}
WedgePort* WedgePlatform::getPort(TransceiverID id) const {
  return portMapping_->getPort(id);
}

}} // facebook::fboss
