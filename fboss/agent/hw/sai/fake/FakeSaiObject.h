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

extern "C" {
#include <sai.h>
}

sai_status_t sai_get_object_count(
    sai_object_id_t switch_id,
    sai_object_type_t object_type,
    uint32_t* count);
