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

#include <memory>

namespace facebook { namespace fboss {

class TransceiverManager;
using ManagerInitFn = std::unique_ptr<TransceiverManager> (*) ();

}} // facebook::fboss

int qsfpMain(int *argc, char ***argv,
             facebook::fboss::ManagerInitFn initManager);


