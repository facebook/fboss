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

#include "fboss/agent/types.h"
#include <boost/container/flat_map.hpp>
#include "fboss/agent/SfpModule.h"

namespace facebook { namespace fboss {

class SfpModule;

/*
 * The SfpMap class is used to store the Sfp Module to port
 * mapping. This is used by the SwSwitch to keep track
 * of the SFPs per port.
 * The SFP Module objects exist for the lifetime of the SfpMap.
 * The callers will have to implement locking before access
 * to the SfpMap.
 * The entire SfpMap gets initialized by the platform code
 * depending on the number of port the platform has and the Sfp
 * objects are updated by the Sfp detection thread.
 */
typedef boost::container::flat_map<PortID,
                                   std::unique_ptr<SfpModule>> PortSfpMap;

class SfpMap {
 public:
  SfpMap();
  virtual ~SfpMap();

  /*
   * This function returns a pointer to the SFP Module object for
   * the port number.
   *
   * Returns: Nullptr if no Sfp object for the port is found
   *          and needs to be checked by the caller.
   */
  SfpModule* sfpModule(PortID portID) const;
  /*
   * This function is used to create the SFP mapping. Port
   * and Sfp Module object Map.
   */
  void createSfp(PortID portID, std::unique_ptr<SfpModule>& sfpModule);

  /*
   * This function returns the beginning of the Port to Sfp Module
   * Map iterator
   */
  PortSfpMap::const_iterator begin() const;

  /*
   * This function returns the end of the Port to Sfp Module
   * Map iterator
   */
  PortSfpMap::const_iterator end() const;

 private:
  // Forbidden copy constructor and assignment operator
  SfpMap(SfpMap const &) = delete;
  SfpMap& operator=(SfpMap const &) = delete;

  PortSfpMap sfpMap_;

};

}} // facebook::fboss
