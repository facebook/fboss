/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <unordered_map>

#include <folly/IPAddress.h>
#include <folly/SocketAddress.h>

#include "fboss/agent/if/gen-cpp2/sflow_types.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class BcmSflowExporter {
 public:
  /*
   * Constructor for an sFlow collector.
   *
   * @param[in] address    IP:port of the collector.
   */
  explicit BcmSflowExporter(const folly::SocketAddress& address);
  ~BcmSflowExporter();

  /*
   * Send out the data in vec using UDP.  Called after init().
   */
  ssize_t sendUDPDatagram(iovec* vec, const size_t iovec_len);

 private:
  // no copy or assignment
  BcmSflowExporter(BcmSflowExporter const&) = delete;
  BcmSflowExporter& operator=(BcmSflowExporter const&) = delete;

  const folly::SocketAddress address_;
  int socket_{-1};
};

class BcmSflowExporterTable {
 public:
  BcmSflowExporterTable() = default;
  ~BcmSflowExporterTable() = default;

  bool contains(const std::shared_ptr<SflowCollector>& collector) const;
  size_t size() const;
  void addExporter(const std::shared_ptr<SflowCollector>& collector);
  void removeExporter(const std::string& ID);

  void updateSamplingRates(PortID id, int64_t inRate, int64_t outRate);

  void sendToAll(const SflowPacketInfo& info);

 private:
  // no copy or assignment
  BcmSflowExporterTable(BcmSflowExporterTable const&) = delete;
  BcmSflowExporterTable& operator=(BcmSflowExporterTable const&) = delete;

  std::unordered_map<std::string, std::unique_ptr<BcmSflowExporter>> map_;
  std::unordered_map<
      PortID,
      std::pair<int64_t /* ingress rate */, int64_t /* egress rate */>>
      port2samplingRates_;
  folly::IPAddress localIP_;
};

} // namespace facebook::fboss
