/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <string>

#include <folly/json/json.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/bgp/policy/CmdShowBgpPolicy.h"

using namespace ::testing;

namespace facebook::fboss {

TEST(CmdShowBgpPolicyTest, wikiDocHooks) {
  EXPECT_FALSE(CmdShowBgpPolicyTraits::description().empty());

  /*
   * sampleModel() serializes a real BgpPolicyStatement, so what is worth
   * pinning here is the PORTABLE encoding the wiki renders - the field names
   * and enum spellings a switch would return - rather than the particular
   * policy and term names the sample happens to choose.
   */
  const auto model = CmdShowBgpPolicy::sampleModel();
  ASSERT_TRUE(model.isObject());
  for (const auto* field :
       {"name", "policy_version", "policy_entries", "result"}) {
    ASSERT_TRUE(model.count(field)) << "missing top-level field " << field;
  }
  EXPECT_EQ(model["result"].asString(), "DENY");

  ASSERT_EQ(model["policy_entries"].size(), 1);
  const auto& term = model["policy_entries"][0];
  for (const auto* field :
       {"name",
        "policy_matches",
        "policy_action_entries",
        "term_miss_action"}) {
    ASSERT_TRUE(term.count(field)) << "missing term field " << field;
  }
  EXPECT_EQ(term["term_miss_action"].asString(), "NEXT_TERM");

  // A match and an action inside the term - the two halves the prose says to
  // read a term by.
  ASSERT_EQ(term["policy_matches"].size(), 1);
  const auto& matchEntries = term["policy_matches"][0]["match_entries"];
  ASSERT_EQ(matchEntries.size(), 1);
  EXPECT_EQ(matchEntries[0]["type"].asString(), "COMMUNITY_LIST");
  ASSERT_EQ(term["policy_action_entries"].size(), 2);
  EXPECT_EQ(
      term["policy_action_entries"][0]["type"].asString(), "SET_LOCAL_PREF");

  /*
   * printOutput() is in the routing-protocol-only TU (see the profiler test
   * above), so it cannot be called here and the command's own rendering path
   * is NOT covered - neither the non-empty render nor the empty/lookup-failure
   * early return description() documents. Recorded as a known gap rather than
   * papered over by re-implementing folly::toPrettyJson here, which would only
   * assert the sample against itself.
   */
}

} // namespace facebook::fboss
