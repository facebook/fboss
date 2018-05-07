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
extern "C" {
#include <complib/sx_log.h>
}

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <folly/Format.h>

#include <syslog.h>

/**
 * Logging callback for SDK
 * Performs init of glog, since this function is called from SDK process
 * and not from FBOSS agent process
 *
 * @param severity Severity level
 * @param module name SDK Module name
 * @param msg Log message
 */
extern "C" void log_cb(sx_log_severity_t severity,
  const char* module_name, char* msg);

/** 
 * Logging Callback used by SDK client
 *
 * @param severity Severity level
 * @param module name SDK Module name
 * @param msg Log message
 */
extern void loggingCallback(sx_log_severity_t severity, 
  const char* module_name, char* msg);

static bool LogInit = false;

void loggingCallback(sx_log_severity_t severity, const char* module_name, char* msg) {
  auto logString = folly::sformat("[{}] {}", module_name, msg);
  switch(severity) {
  case SX_LOG_NONE:
    break;
  case SX_LOG_INFO:
  case SX_LOG_NOTICE:
    LOG(INFO) << logString;
    break;
  case SX_LOG_WARNING:
    LOG(WARNING) << logString;
    break;
  case SX_LOG_ERROR:
    LOG(ERROR) << logString;
    break;
  default:
    LOG(INFO) << logString;
    break;
  }
}

void log_cb(sx_log_severity_t severity, const char* module_name, char* msg) {
  if (!LogInit) {
    openlog("SDK", 0, LOG_USER);

    // Read from flags file
    gflags::ReadFromFlagsFile("/tmp/fboss_cmd_flags", "SDK", false);

    // init glog
    google::InitGoogleLogging("SDK");

    LogInit = true;
  }

  loggingCallback(severity, module_name, msg);
}

