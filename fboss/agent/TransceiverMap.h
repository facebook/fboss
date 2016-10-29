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

#include <mutex>
#include <utility>

#include <boost/container/flat_map.hpp>

#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

class Transceiver;

/*
 * The TransceiverMap class is used to store the list of transceivers,
 * and the mapping from port to transceiver and channel number.
 * This is used by the SwSwitch to keep track of the SFPs per port.
 * The SFP Module objects exist for the lifetime of the TransceiverMap.
 * The callers will have to implement locking before access
 * to the TransceiverMap.
 * The entire TransceiverMap gets initialized by the platform code
 * depending on the number of ports the platform has and the transceiver
 * objects are updated by the transceiver detection thread.
 */

using TransceiverIdx = std::pair<ChannelID, TransceiverID>;
using PortTransceiverMap = boost::container::flat_map<PortID, TransceiverIdx>;
using Transceivers = boost::container::flat_map<
  TransceiverID, std::unique_ptr<Transceiver>>;

class TransceiverMap {
 public:
  TransceiverMap() = default;
  ~TransceiverMap() = default;

  /*
   * This function returns a pair with the channel number and
   * transceiver number given the port number.
   *
   * Returns: Nullptr if no transceiver object for the port is found
   *          and needs to be checked by the caller.
   */
  TransceiverIdx transceiverMapping(PortID portID) const;

  /*
   * Return a pointer to the transceiver
   */
  Transceiver* transceiver(TransceiverID id) const;

  /*
   * Add the transceiver to the list of devices
   */
  void addTransceiver(TransceiverID idx, std::unique_ptr<Transceiver> module);

  /*
   * This function is used to create the transceiver mapping. Port
   * and Transceiver Module object Map.
   */
  void addTransceiverMapping(
      PortID portID, ChannelID channel, TransceiverID transceiver);

  /*
   * Thread safe method to iterate over exsting transceivers.
   *
   * NOTE: Do not call other APIs of this class inside of the callback as it
   * can cause deadlock.
   */
  void iterateTransceivers(
      std::function<void(TransceiverID, Transceiver*)> callback) const;

 private:
  // Forbidden copy constructor and assignment operator
  TransceiverMap(TransceiverMap const &) = delete;
  TransceiverMap& operator=(TransceiverMap const &) = delete;

  // Mutex to protect transceiverMap_ and transceivers_
  mutable std::mutex mutex_;

  PortTransceiverMap transceiverMap_;
  Transceivers transceivers_;
};

}} // facebook::fboss
