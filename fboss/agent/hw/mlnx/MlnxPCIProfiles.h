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

extern "C" {
#include <sx/sxd/kernel_user.h>
#include <sx/sdk/sx_dev.h>
}

namespace facebook { namespace fboss {

/*
 * RDQ PROPERTIES
 */
#define RDQ_ETH_DEFAULT_SIZE    4200
// the needed value is 10000 but added more for align
#define RDQ_ETH_LARGE_SIZE            10240
#define RDQ_DEFAULT_NUMBER_OF_ENTRIES 1024
#define RDQ_MAD_NUMBER_OF_ENTRIES     RDQ_DEFAULT_NUMBER_OF_ENTRIES

// weights
#define RDQ_ETH_SINGLE_SWID_DEFAULT_WEIGHT 10
#define RDQ_ETH_MULTI_SWID_DEFAULT_WEIGHT  10

#define RDQ_ETH_MULTI_SWID_NUMBER  9
#define RDQ_ETH_SINGLE_SWID_NUMBER 3

#define DEVICE_PROFILE_MASK_SET_MAX_VEPA_CHANNELS 0x01
#define DEVICE_PROFILE_MASK_SET_MAX_LAG           0x02
#define DEVICE_PROFILE_MASK_SET_MAX_PORTS_PER_LAG 0x04
#define DEVICE_PROFILE_MASK_SET_MAX_MID           0x08
#define DEVICE_PROFILE_MASK_SET_MAX_PGT           0x10
#define DEVICE_PROFILE_MASK_SET_MAX_SYSTEM_PORT   0x20
#define DEVICE_PROFILE_MASK_SET_MAX_ACTIVE_VLANS  0x40
#define DEVICE_PROFILE_MASK_SET_MAX_REGIONS       0x80
#define DEVICE_PROFILE_MASK_SET_FID_BASED         0x100
#define DEVICE_PROFILE_MASK_SET_MAX_FLOOD_TABLES  0x200
#define DEVICE_PROFILE_MASK_SET_MAX_IB_MC         0x1000
#define DEVICE_PROFILE_MASK_SET_MAX_PKEY          0x2000

/*
 * SYSTEM PROFILE
 */

enum sys_profile_t {
    SYS_PROFILE_EN_SINGLE_SWID,
    SYS_PROFILE_EN_MULTI_SWID,
};

extern const sx_pci_profile kPciProfileSingleEthSpectrum;
extern const ku_profile kSinglePartEthDeviceProfileSpectrum;

}}

