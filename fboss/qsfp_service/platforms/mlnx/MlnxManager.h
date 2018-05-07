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

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

#include "fboss/qsfp_service/TransceiverManager.h"

extern "C" {
#include <sx/sdk/sx_swid.h>
#include <sx/sdk/sx_dev.h>
#include <complib/sx_log.h>
}

namespace facebook { namespace fboss {

class MlnxManager : public TransceiverManager {
 public:

  static const sx_swid_t kDefaultSwid = 0;
  static const sx_dev_id_t kDefaultDeviceId = 1;

  using TransceiverMap = std::map<int32_t, TransceiverInfo>;
  using PortMap = std::map<int32_t, PortStatus>;

  /**
   * ctor, initializes register access
   */
  MlnxManager();

  /**
   * dtor, deinit register access
   */
  ~MlnxManager() override;

  /**
   * Inits a transceiver id to MlnxQsfp object map
   */
  void initTransceiverMap() override;

  /**
   * Gets transceiver info from parsed EEPROM memory
   *
   * @param[out] info transceiver id to TransceiverInfo map
   * @param[in] ids Vector of transceiver ids
   */
  void getTransceiversInfo(TransceiverMap& info,
    std::unique_ptr<std::vector<int32_t>> ids) override;

  /**
   * Gets raw EEPROM data
   *
   * @param[out] Output map to fill in data
   * @param[in] ids vector of transceiver ids to get data from
   */
  void getTransceiversRawDOMData(std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) override;

  /**
   * Asks QsfpModule to customize transceiver based on speed
   *
   * @param idx transceiver id
   * @param speed at which ports associated
   *        with transceiver id @idx are running
   */
  void customizeTransceiver(int32_t idx, cfg::PortSpeed speed) override;

  /**
   * Sync ports info.
   * FBOSS agent calls this API to update admin/oper state and speed of ports
   *
   * @param info Transceiver info map
   * @param ports Port map with port status info (admin/oper state and speed)
   */
  void syncPorts(TransceiverMap& info, std::unique_ptr<PortMap> ports) override;

  /**
   * Returns the number of QSFP modules in system
   *
   * @ret number of QSFP modules
   */
  int getNumQsfpModules() override;

  /**
   * Update cached transceiver's EEPROM data for all modules
   */
  void refreshTransceivers() override;

 private:
  // Forbidden copy constructor and assignment operator
  MlnxManager(MlnxManager const &) = delete;
  MlnxManager& operator=(MlnxManager const &) = delete;

  //private fields

  boost::container::flat_set<uint8_t> modules_;
};

}} // facebook::fboss
