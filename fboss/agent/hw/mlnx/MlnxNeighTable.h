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

#include <memory>

namespace facebook { namespace fboss {

class MlnxSwitch;

struct MlnxNeighKey;
class MlnxNeigh;

/**
 * This class is a table for MlnxNeigh objects
 * Contains mapping between MlnxNeighKey (Interface Id, neighbor ip address)
 * and MlnxNeigh instance
 */
class MlnxNeighTable {
 public:
  // NeighMapT - type for container
  using NeighMapT =
    boost::container::flat_map<MlnxNeighKey, std::unique_ptr<MlnxNeigh>>;

  /**
   * ctor, creates an empty map
   *
   * @param hw Pointer to MlnxSwitch
   */
  MlnxNeighTable(MlnxSwitch* hw);

  /**
   * dtor, deletes table and all neighbors
   */
  ~MlnxNeighTable();

  /**
   * Adds a neighbor to table
   *
   * @param swNeigh SW neighbor entry object to add
   */
  template <typename NeighEntryT>
  void addNeigh(const std::shared_ptr<NeighEntryT>& swNeigh);

  /**
   * Deletes a neighbor to table
   *
   * @param swNeigh SW neighbor entry object to delete
   */
  template <typename NeighEntryT>
  void deleteNeigh(const std::shared_ptr<NeighEntryT>& swNeigh);

 private:
  // privatge fields

  MlnxSwitch* hw_ {nullptr};
  NeighMapT neighbors_ {};
};

}} // facebook::fboss
