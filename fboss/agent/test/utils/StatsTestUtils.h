// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <sstream>

template <typename MapTypeT>
std::string statsMapDelta(const MapTypeT& before, const MapTypeT& after) {
  std::stringstream ss;
  auto bitr = before.begin();
  auto aitr = after.begin();
  while (bitr != before.end() && aitr != after.end()) {
    if (*bitr == *aitr) {
      bitr++;
      aitr++;
    } else if (bitr->first < aitr->first) {
      ss << "Missing key in after: " << bitr->first << std::endl;
      bitr++;
    } else if (aitr->first < bitr->first) {
      ss << "Missing key in before: " << aitr->first << std::endl;
      aitr++;
    } else {
      CHECK_NE(aitr->second, bitr->second);
      ss << " Stats did not match for : " << aitr->first
         << " Before : " << bitr->second << std::endl
         << " After: " << aitr->second << std::endl;
      aitr++;
      bitr++;
    }
  }
  while (bitr != before.end()) {
    ss << "Missing key in after: " << bitr->first << std::endl;
    bitr++;
  }
  while (aitr != after.end()) {
    ss << "Missing key in before: " << bitr->first << std::endl;
    aitr++;
  }
  return ss.str();
}
