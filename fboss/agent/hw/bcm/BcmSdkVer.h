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

#if (!defined(BCM_VER_MAJOR)) || (!defined(BCM_VER_MINOR)) || \
    (!defined(BCM_VER_RELEASE))
#define IS_OPENNSA
#else
#define BCM_VERSION(major, minor, micro) \
  (100000 * (major) + 1000 * (minor) + (micro))
#define BCM_SDK_VERSION \
  BCM_VERSION(BCM_VER_MAJOR, BCM_VER_MINOR, BCM_VER_RELEASE)
#if (BCM_SDK_VERSION >= BCM_VERSION(6, 5, 26))
#define BCM_SDK_VERSION_GTE_6_5_26
#endif
#if (BCM_SDK_VERSION >= BCM_VERSION(6, 5, 28))
#define BCM_SDK_VERSION_GTE_6_5_28
#endif
#if (BCM_SDK_VERSION >= BCM_VERSION(6, 5, 29))
#define BCM_SDK_VERSION_GTE_6_5_29
#endif
#endif
