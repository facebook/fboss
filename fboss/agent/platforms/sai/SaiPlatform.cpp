/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include "SaiPlatform.h"
#include "SaiPort.h"
#include "fboss/agent/hw/sai/SaiSwitch.h"

using std::string;
using folly::MacAddress;

#ifdef DEBUG_
DEFINE_string(volatile_state_dir, "/tmp/fboss",
              "Directory for storing volatile state");
DEFINE_string(persistent_state_dir, "/tmp/fboss",
              "Directory for storing persistent state");
#else
DEFINE_string(volatile_state_dir, "/dev/shm/fboss",
              "Directory for storing volatile state");
DEFINE_string(persistent_state_dir, "/var/facebook/fboss",
              "Directory for storing persistent state");
#endif

DEFINE_string(mac, "",
              "The local MAC address for this switch");

namespace facebook { namespace fboss {

SaiPlatform::SaiPlatform() {
  VLOG(4) << "Entering " << __FUNCTION__;

  initLocalMac();
  // TODO: Create SaiSwitch object here
  hw_.reset(new SaiSwitch(this));
}

SaiPlatform::~SaiPlatform() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

HwSwitch *SaiPlatform::getHwSwitch() const {
  VLOG(4) << "Entering " << __FUNCTION__;
  return hw_.get();
}

void SaiPlatform::initSwSwitch(SwSwitch *sw) {
}

MacAddress SaiPlatform::getLocalMac() const {
  return localMac_;
}

string SaiPlatform::getVolatileStateDir() const {
  return FLAGS_volatile_state_dir;
}

string SaiPlatform::getPersistentStateDir() const {
  return FLAGS_persistent_state_dir;
}

SaiPlatform::InitPortMap SaiPlatform::initPorts() {
  VLOG(4) << "Entering " << __FUNCTION__;

  InitPortMap ports;

  const sai_object_list_t &pl = hw_->GetSaiPortList();

  for (uint32_t nPort = 0; nPort < pl.count; ++nPort) {
    PortID portID(nPort);
    auto saiPort = folly::make_unique<SaiPort>(portID);

    ports.emplace(pl.list[nPort], saiPort.get());
    ports_.emplace(portID, std::move(saiPort));
  }

  return ports;
}

void SaiPlatform::initLocalMac() {
  if (!FLAGS_mac.empty()) {
    localMac_ = MacAddress(FLAGS_mac);
    return;
  }

  // TODO: Get the base MAC address from the SAI or autogenerate it here.
  MacAddress eth0Mac("00:11:22:33:44:55");
  localMac_ = MacAddress::fromHBO(eth0Mac.u64HBO() | 0x0000020000000000);
}

void SaiPlatform::getProductInfo(ProductInfo& info) {
  
}

}} // facebook::fboss
