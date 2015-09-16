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

#include <utility>
#include "fboss/agent/types.h"
#include <boost/container/flat_map.hpp>

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

typedef std::pair<ChannelID, TransceiverID> TransceiverIdx;
typedef boost::container::flat_map<PortID, TransceiverIdx>
        PortTransceiverMap;
typedef boost::container::flat_map<TransceiverID,
                                   std::unique_ptr<Transceiver>> Transceivers;

class TransceiverMap {
 public:
  TransceiverMap();
  ~TransceiverMap();

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
   * Add the transceiver to the list of devices, returning the ID
   * for later use.
   */
  void addTransceiver(TransceiverID idx, std::unique_ptr<Transceiver> module) {
    transceivers_.emplace(std::make_pair(idx, std::move(module)));
  }
  /*
   * This function is used to create the transceiver mapping. Port
   * and Transceiver Module object Map.
   */
  void addTransceiverMapping(PortID portID, ChannelID channel,
                             TransceiverID transceiver);
  /*
   * This function returns an iterator traversing the list of transceivers.
   */
  Transceivers::const_iterator begin() const;
  /*
   * This function returns the end of the list of transceivers iterator.
   */
  Transceivers::const_iterator end() const;

 private:
  // Forbidden copy constructor and assignment operator
  TransceiverMap(TransceiverMap const &) = delete;
  TransceiverMap& operator=(TransceiverMap const &) = delete;

  PortTransceiverMap transceiverMap_;
  Transceivers transceivers_;
};

}} // facebook::fboss
