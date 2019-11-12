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

#include <atomic>
#include <mutex>
#include <optional>

#include <boost/container/flat_map.hpp>
#include <folly/futures/SharedPromise.h>
#include <folly/futures/Future.h>
#include <folly/io/async/AsyncTimeout.h>
#include <folly/Synchronized.h>
#include <folly/Unit.h>
#include <folly/futures/Future.h>
#include <folly/futures/SharedPromise.h>
#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/EventBase.h>

#include "fboss/agent/types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

/*
 * This class is a helper for clients that want to exchange port state w/ qsfp
 * service. It is built around a single thrift call defined in qsfp.thrift:
 *
 *     map<i32, transceiver.TransceiverInfo> syncPorts(1: map<i32, ctrl.PortStatus> ports)
 *
 * Using this we can exchange port state from one class (presumably
 * the agent) and then receive transceiver information we may care about.
 *
 * The idea here is that agent will tell this class about any port
 * state changes and then internally the class is responsible for
 * communicating that information to qsfp_service.
 *
 * Ensuring qsfp_service data is up-to-date
 * ----------------------------------------
 * The class uses an always incrementing generation number and every
 * request and port change is associated w/ a specific generation
 * number. On a successful request, we update the remoteGen_ member
 * variable and we know we only need to send more requests if any port
 * generation number is > remoteGen_.
 *
 * Communication w/ qsfp_service
 * -----------------------------
 * We limit communication to a single active syncPorts request to
 * qsfp_service. This request has all ports s.t the generation number
 * for the latest change to that port is > remoteGen_.
 *
 * Detecting restarts
 * ------------------
 * We also need to handle potential restarts of the qsfp_service. In
 * this case, we periodically make aliveSince calls to qsfp_service
 * and store the last aliveSince. If this changes, we reset remoteGen_
 * back to zero so we will re-sync all ports.
 *
 * Threading model
 * ---------------
 * All thrift calls to qsfp_service are done on evb_. No guarantee for
 * init, get, dump, portChanged and portsChanged. We have one mutex
 * for each map we store (one for ports, one for transceivers).
 *
 */

namespace facebook {
namespace fboss {

class QsfpCache : private folly::AsyncTimeout {
 public:
  // types we exchange over thrift
  using PortMapThrift = std::map<int32_t, PortStatus>;
  using TcvrMapThrift = std::map<int32_t, TransceiverInfo>;

  QsfpCache() = default;

  /* Initializers. Sets the Eventbase and optionally the initial port
   * map to sync to qsfp_service.
   */
  void init(folly::EventBase* evb, const PortMapThrift& ports);
  void init(folly::EventBase* evb);

  /* Some ports have changed. Once this function is called the cache
   * will asynchronously ensure that the port change is successfully
   * communicated to qsfp_service.
   *
   * TODO: could change api to take Port SwitchState objects down the
   * line.  One annoyance w/ PortStatus is that it does not actually
   * store the PortID in the struct.
   */
  void portsChanged(const PortMapThrift& ports);
  void portChanged(int32_t id, const PortStatus& port);

  /*
   * Getters. These return the TransceiverInfo struct given a
   * TransceiverID. Three variants:
   *
   * get: throws an exception if id not in cache
   * getIf: returns empty Optional if id not in cache
   * futureGet: waits for active request, but throws if not in cache afterwards
   */
  TransceiverInfo get(TransceiverID tcvrId);
  std::optional<TransceiverInfo> getIf(TransceiverID tcvrId);
  folly::Future<TransceiverInfo> futureGet(TransceiverID tcvrId);

  // output state of the cache. Useful for debugging
  void dump();

 private:
  // Forbidden copy constructor and assignment operator
  QsfpCache(QsfpCache const &) = delete;
  QsfpCache& operator=(QsfpCache const &) = delete;

  void timeoutExpired() noexcept override;

  /* Makes a syncPorts call if any ports need to be communicated to
   * qsfp_service and there is no active request.
   */
  void maybeSync();

  // actually does a syncPorts call to qsfp_service
  folly::Future<folly::Unit> doSync(PortMapThrift&& portsToSync);

  // checks qsfp_service is alive and detects restarts
  folly::Future<folly::Unit> confirmAlive();

  /* Called after successful sync to update transceivers in to our
   * cache.
   */
  void updateCache(const TcvrMapThrift& tcvrs);

  // gets a new unique generation number
  uint32_t incrementGen();

  struct PortCacheValue {
    PortStatus port;
    uint32_t generation{0};
  };

  folly::Synchronized<
    std::unordered_map<TransceiverID, TransceiverInfo>> tcvrs_;
  folly::Synchronized<
    boost::container::flat_map<PortID, PortCacheValue>> ports_;

  std::optional<folly::SharedPromise<folly::Unit>> activeReq_;

  folly::EventBase* evb_{nullptr};

  // generation number that we know is synced to qsfp_service
  uint32_t remoteGen_{0};

  // last aliveSince from qsfp_service
  int64_t remoteAliveSince_{-1};

  std::atomic_bool initialized_{false};
};

class AutoInitQsfpCache : public QsfpCache {
 public:
  AutoInitQsfpCache();
  ~AutoInitQsfpCache();

 private:
  std::unique_ptr<std::thread> thread_{nullptr};
  folly::EventBase evb_;
};

} // namespace fboss
} // namespace facebook
