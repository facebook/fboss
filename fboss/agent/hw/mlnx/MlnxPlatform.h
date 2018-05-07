/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "fboss/agent/Platform.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/types.h"

extern "C" {
#include <sx/sdk/sx_api_port.h>
}

#include <boost/container/flat_map.hpp>

namespace facebook { namespace fboss {

class MlnxSwitch;
class MlnxPlatformPort;

/**
 * MlnxPlatform specifies additional APIs that must be provided by platforms
 * based on mlnx chips.
 */
class MlnxPlatform : public Platform {
 public:
  using PortMap = boost::container::flat_map<PortID,
    std::unique_ptr<MlnxPlatformPort>>;
  using InitPortMap = boost::container::flat_map<
    sx_port_log_id_t, MlnxPlatformPort*>;

  /**
   * ctor
   */
  MlnxPlatform();

  /**
   * dtor
   */
  virtual ~MlnxPlatform() override;

  /**
   * Initialize the platform
   */
  void init();

  /**
   * Initialize localMac_
   */
  void initLocalMac();

  /**
   * Get initial port map
   *
   * @return Port map log_port -> MlnxPlatformPort instance
   */
  InitPortMap initPorts();

  /**
   * Returns pointer to HwSwitch instance
   *
   * @ret Pointer to HwSwitch
   */
  HwSwitch* getHwSwitch() const override;

  /**
   * Should be called by the hardware code
   * after initialization to perform
   * platform specific initialization
   *
   * @param sw Pointer to Software Switch instance
   */
  void onHwInitialized(SwSwitch* sw) override;

  /**
   * Creates Thrift Handler
   *
   * @param sw Pointer to Software Switch instance
   * @return ThriftHandler instance
   */
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  /**
   * Returns local mac address of this switch
   * (Specificaly, the mac of swid0_eth device)
   *
   * @ret switch mac address
   */
  folly::MacAddress getLocalMac() const override;
  
  /**
   * Returns path to a file with command line flags
   * NOTE: this is needed for sdk logger to sync
   * glog's logging options via gflags
   *
   * @ret path to flags file
   */
  std::string getCmdLineFlagsFile();

  /**
   * Returns volatile state directory
   *
   * @ret volatile state directory
   */
  std::string getVolatileStateDir() const override;

  /**
   * Returns persistent state directory
   *
   * @ret persistent state directory
   */
  std::string getPersistentStateDir() const override;

  /**
   * Fills ProductInfo fields
   *
   * @prarm info ProductInfo instance
   */
  void getProductInfo(ProductInfo& info) override;

  /**
   * Returst the transceiver associated with @portId
   *
   * @pararm portId port ID
   * @ret transceiverIdxThrift object
   */
  TransceiverIdxThrift getPortMapping(PortID portId) const override;

  /**
   * Returns MlnxPlatformPort by portId
   *
   * @param portId Port ID
   * @return MlnxPlatformPort pointer
   */
  PlatformPort* getPlatformPort(PortID portId) const override;

  /**
   * Returns MlnxPlatformPort by portId
   *
   * @param portId Port ID
   * @return MlnxPlatformPort pointer
   */
  MlnxPlatformPort* getPort(PortID portId) const;

 private:
  // Forbidden copy constructor and assignment operator
  MlnxPlatform(MlnxPlatform const &) = delete;
  MlnxPlatform& operator=(MlnxPlatform const &) = delete;

  /**
   * Write logging flags to a file
   */
  static void dumpCommandLineFlagsToFile(const std::string& filePath);

  folly::MacAddress localMac_;
  std::unique_ptr<MlnxSwitch> hw_;
  PortMap ports_;
};

}} // facebook::fboss
