/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/RestClient.h"

#include <sstream>

#include "fboss/agent/FbossError.h"
#include "folly/FileUtil.h"
#include "folly/logging/xlog.h"

#include <curl/curl.h>

namespace facebook::fboss {
RestClient::RestClient(std::string hostname, int port)
    : hostname_(hostname), port_(port) {
  createEndpoint();
}

RestClient::RestClient(folly::IPAddress ipAddress, int port)
    : ipAddress_(ipAddress), port_(port) {
  createEndpoint();
}
RestClient::RestClient(
    folly::IPAddress ipAddress,
    int port,
    std::string interface)
    : ipAddress_(ipAddress), interface_(interface), port_(port) {
  createEndpoint();
}

void RestClient::createEndpoint() {
  if (!hostname_.empty()) {
    endpoint_ = hostname_;
  } else {
    /* check the IP address type */
    if (ipAddress_.isV6()) {
      endpoint_ = "[" + ipAddress_.str() + "]";
    } else {
      endpoint_ = ipAddress_.str();
    }
  }
}

void RestClient::setTimeout(std::chrono::milliseconds timeout) {
  timeout_ = timeout;
}

std::string RestClient::requestWithOutput(
    std::string path,
    std::string postData) {
  CURL* curl;
  CURLcode resp;
  std::stringbuf write_buffer;
  auto endpoint = endpoint_ + path;
  /* for curl errors */
  char error[CURL_ERROR_SIZE];

  curl = curl_easy_init();
  if (curl) {
    /* Set the curl options */
    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl, CURLOPT_PORT, port_);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_.count());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, RestClient::writer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_buffer);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    /* if an interface is specified use that */
    if (!interface_.empty()) {
      curl_easy_setopt(curl, CURLOPT_INTERFACE, interface_.c_str());
    }

    struct curl_slist* headers = nullptr;
    if (!postData.empty()) {
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
      headers = curl_slist_append(nullptr, "Content-Type: application/json");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    resp = curl_easy_perform(curl);
    if (headers) {
      curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    if (resp == CURLE_OK) {
      return write_buffer.str();
    } else {
      throw FbossError("Error querying api: ", endpoint, " error: ", error);
    }
  }
  throw FbossError("Error initializing curl interface");
}

bool RestClient::request(std::string path) {
  auto ret = requestWithOutput(path);
  std::size_t status = ret.find("done");
  if (status != std::string::npos) {
    return true;
  }
  return false;
}

size_t RestClient::writer(
    char* buffer,
    size_t size,
    size_t entries,
    std::stringbuf* writer_buffer) {
  std::streamsize data_put = writer_buffer->sputn(buffer, size * entries);
  return data_put;
}

} // namespace facebook::fboss
