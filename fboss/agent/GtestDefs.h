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

/*
 * googletest-1.10 deprecates TYPED_TEST_CASE and replaces it with
 * TYPED_TEST_SUITE
 *
 * OSS build uses googletest-1.10, however fbcode build uses
 * googletest-1.8
 *
 * To accommodate both builds as warning free, map
 * TYPED_TEST_SUITE to TYPED_TEST_CASE if it's not already defined (in gtest.h)
 */
#ifndef TYPED_TEST_SUITE
#define TYPED_TEST_SUITE TYPED_TEST_CASE
#endif
