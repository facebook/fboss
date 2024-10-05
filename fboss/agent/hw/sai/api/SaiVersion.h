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

#ifndef IS_OSS

#if !defined(SAI_VERSION)
#define SAI_VERSION(major, minor, micro) \
  (100000 * (major) + 1000 * (minor) + (micro))
#endif

#if !defined(SAI_API_VERSION)
#if (!defined(SAI_VER_MAJOR)) || (!defined(SAI_VER_MINOR)) || \
    (!defined(SAI_VER_RELEASE))
#error "SAI version not defined"
#endif

#define SAI_API_VERSION \
  SAI_VERSION(SAI_VER_MAJOR, SAI_VER_MINOR, SAI_VER_RELEASE)
#endif

#endif

/*
 *  Aggregate SDK Version flags
 *
 *  Purpose of this is try to minimize the SDK version flags scattering around
 * the code base. It's error prone and hard to maintain.
 *
*  Apart from these, individual flags are available for versions greater than
 * certain SDK. They will be useful for features being supported since a
 * particular SDK version.

 *  High-level BCM-SDK flags:
 *   - BRCM_SAI_SDK_XGS: Flags for all XGS SDK versions
 *   - BRCM_SAI_SDK_DNX: Flags for DNX SDK versions
 *   - BRCM_SAI_SDK_XGS_AND_DNX: Flags for all BRCM-SAI SDK versions
 */
#if defined(SAI_VERSION_8_2_0_0_ODP) ||                                        \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||                                    \
    defined(SAI_VERSION_9_2_0_0_ODP) || defined(SAI_VERSION_9_0_EA_SIM_ODP) || \
    defined(SAI_VERSION_10_0_EA_ODP) ||                                        \
    defined(SAI_VERSION_10_0_EA_SIM_ODP) ||                                    \
    defined(SAI_VERSION_10_2_0_0_ODP) || defined(SAI_VERSION_11_0_EA_ODP) ||   \
    defined(SAI_VERSION_11_0_EA_SIM_ODP) || defined(SAI_VERSION_11_3_0_0_ODP)
#define BRCM_SAI_SDK_XGS
#endif

#if defined(SAI_VERSION_10_0_EA_DNX_ODP) ||     \
    defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) || \
    defined(SAI_VERSION_11_0_EA_DNX_ODP) ||     \
    defined(SAI_VERSION_11_0_EA_DNX_SIM_ODP) || \
    defined(SAI_VERSION_11_3_0_0_DNX_ODP) ||    \
    defined(SAI_VERSION_12_0_EA_DNX_ODP)
#define BRCM_SAI_SDK_DNX
#endif

#if defined(BRCM_SAI_SDK_XGS) || defined(BRCM_SAI_SDK_DNX)
#define BRCM_SAI_SDK_XGS_AND_DNX
#endif

#if defined(SAI_VERSION_12_0_EA_DNX_ODP)
#define BRCM_SAI_SDK_GTE_12_0
#endif

#if defined(SAI_VERSION_12_0_EA_DNX_ODP)
#define BRCM_SAI_SDK_DNX_GTE_12_0
#endif

#if defined BRCM_SAI_SDK_GTE_12_0 || defined(SAI_VERSION_11_0_EA_ODP) || \
    defined(SAI_VERSION_11_0_EA_SIM_ODP) ||                              \
    defined(SAI_VERSION_11_0_EA_DNX_ODP) ||                              \
    defined(SAI_VERSION_11_0_EA_DNX_SIM_ODP) ||                          \
    defined(SAI_VERSION_11_3_0_0_DNX_ODP) || defined(SAI_VERSION_11_3_0_0_ODP)
#define BRCM_SAI_SDK_GTE_11_0
#endif

#if defined BRCM_SAI_SDK_DNX_GTE_12_0 ||    \
    defined(SAI_VERSION_11_0_EA_DNX_ODP) || \
    defined(SAI_VERSION_11_3_0_0_DNX_ODP)
#define BRCM_SAI_SDK_DNX_GTE_11_0
#endif

#if defined BRCM_SAI_SDK_GTE_11_0 || defined(SAI_VERSION_10_0_EA_ODP) || \
    defined(SAI_VERSION_10_0_EA_SIM_ODP) ||                              \
    defined(SAI_VERSION_10_0_EA_DNX_ODP) ||                              \
    defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) ||                          \
    defined(SAI_VERSION_10_2_0_0_ODP)
#define BRCM_SAI_SDK_GTE_10_0
#endif

#if defined(BRCM_SAI_SDK_GTE_10_0) || defined BRCM_SAI_GTE_11_0 || \
    defined(SAI_VERSION_9_2_0_0_ODP) || defined(SAI_VERSION_9_0_EA_SIM_ODP)
#define BRCM_SAI_SDK_GTE_9_2
#endif

/*
 *  High-level TAJO flags:
 *   - TAJO_SDK_EBRO: Flags for all EBRO(GB) SDK
 *   - TAJO_SDK_MORGAN: Flags for Morgan(G200) SDK
 *   - TAJO_P4_WB_SDK: Flags for P4 WB SDK
 *   - TAJO_SDK_GTE_24_4_90: Flags for SDK >= 24.4.90
 *   - TAJO_SDK: Flags for all TAJO SDK
 */
#if defined(TAJO_SDK_VERSION_24_4_90) || defined(TAJO_SDK_VERSION_24_6_1) || \
    defined(TAJO_SDK_VERSION_24_7_0) || defined(TAJO_SDK_VERSION_24_8_3001)
#define TAJO_P4_WB_SDK
#endif

#if defined(TAJO_SDK_VERSION_1_42_8) || defined(TAJO_SDK_VERSION_24_4_90) || \
    defined(TAJO_SDK_VERSION_24_6_1) || defined(TAJO_SDK_VERSION_24_7_0) ||  \
    defined(TAJO_SDK_VERSION_24_8_3001)
#define TAJO_SDK_EBRO
#endif

#if defined(TAJO_SDK_VERSION_24_4_90) || defined(TAJO_SDK_VERSION_24_6_1) || \
    defined(TAJO_SDK_VERSION_24_7_0) || defined(TAJO_SDK_VERSION_24_8_3001)
#define TAJO_SDK_GTE_24_4_90
#endif

#if defined(TAJO_SDK_EBRO) || defined(TAJO_SDK_MORGAN)
#define TAJO_SAI_SDK
#endif
