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
#include "fboss/agent/hw/mlnx/MlnxPCIProfiles.h"

namespace facebook { namespace fboss {

const sx_pci_profile kPciProfileSingleEthSpectrum = {
  // profile enum
  PCI_PROFILE_EN_SINGLE_SWID,
  // tx_prof: <swid,etclass> -> <stclass,sdq>
  {
    {
      {0, 2}, // -0-best effort
      {1, 2}, // -1-low prio
      {2, 2}, // -2-medium prio
      {3, 2}, // -3
      {4, 2}, // -4-
      {5, 1}, // -5-high prio
      {6, 1}, // -6-critical prio
      {6, 1}  // -7-
    }
  },
  // emad_tx_prof
  {
    0, 0
  },
  // swid_type
  {
    SX_KU_L2_TYPE_ETH,
    SX_KU_L2_TYPE_DONT_CARE,
    SX_KU_L2_TYPE_DONT_CARE,
    SX_KU_L2_TYPE_DONT_CARE,
    SX_KU_L2_TYPE_DONT_CARE,
    SX_KU_L2_TYPE_DONT_CARE,
    SX_KU_L2_TYPE_DONT_CARE,
    SX_KU_L2_TYPE_DONT_CARE
  },
  // ipoip_router_port_enable
  {
    0,
  },
  // max_pkey
  0,
  // rdq_count
  {
      33,
      0,
      0,
      0,
      0,
      0,
      0,
      0
  },
  // rdq
  {
    {
      // swid 0 - ETH
      0,
      1,
      2,
      3,
      4,
      5,
      6,
      7,
      8,
      9,
      10,
      11,
      12,
      13,
      14,
      15,
      16,
      17,
      18,
      19,
      20,
      21,
      22,
      23,
      24,
      25,
      26,
      27,
      28,
      29,
      30,
      31,
      32
    },
  },
  // emad_rdq
  33,
  // rdq_properties
  {
    // SWID 0
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -0-best effort priority
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -1-low priority
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -2-medium priority
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -3-high priority
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -4-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -5-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    },// -6-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    },// -7-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -8-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -9-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -10-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -11-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -12-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -13-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -14-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -15-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -16-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -17-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -18-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -19-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -20-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -21-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -22-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -23-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -24-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -25-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -26-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -27-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -28-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -29-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -30-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -31-
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -32-mirror agent
    {
      RDQ_DEFAULT_NUMBER_OF_ENTRIES,
      RDQ_ETH_LARGE_SIZE,
      RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT,
      0
    }, // -33-emad
  },
  // cpu_egress_tclass per SDQ
  {
    2, // -0-EMAD SDQ
    1, // -1-Control SDQ
    0, // -2-Data SDQ
    0, // -3-
    0, // -4-
    0, // -5-
    0, // -6-
    0, // -7-
    0, // -8-
    0, // -9-
    0, // -10-
    0, // -11-
    0, // -12-
    0, // -13-
    0, // -14-
    0, // -15-
    0, // -16-
    0, // -17-
    0, // -18-
    0, // -19-
    0, // -20-
    0, // -21-
    0, // -22-
    0 // -23-55
  },
  // dev_id
  0,
};

const ku_profile kSinglePartEthDeviceProfileSpectrum = {
  // dev id
  0,
  0x70073ff,    //  bit 9 and bits 10-11 are turned off
  0,
  0,
  64,
  32,
  7000,
  0,
  64,
  127,
  400,
  2,
  1,
  3,
  // max_fid_offset_flood_tables
  0,
  // fid_offset_table_size
  0,
  // max_per_fid_flood_table
  0,
  // per_fid_table_size
  0,
  // max_fid
  0,
  0,
  0,
  0,
  0,
  0,
  0x10000, //  64K
  0x20000, //  128K
  0xC000, //  48K
  {
    1,
    KU_SWID_TYPE_ETHERNET
  },
  {
    1,
    KU_SWID_TYPE_DISABLED
  },
  {
    1,
    KU_SWID_TYPE_DISABLED
  },
  {
    1,
    KU_SWID_TYPE_DISABLED
  },
  {
    1,
    KU_SWID_TYPE_DISABLED
  },
  {
    1,
    KU_SWID_TYPE_DISABLED
  },
  {
    1,
    KU_SWID_TYPE_DISABLED
  },
  {
    1,
    KU_SWID_TYPE_DISABLED
  },
  0,
  0,
  0,
  0,
  0,
  {
    0,
  },
  SXD_CHIP_TYPE_SPECTRUM,
  0,
};

}} // facebook::fboss


