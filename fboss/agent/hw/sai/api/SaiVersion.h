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
