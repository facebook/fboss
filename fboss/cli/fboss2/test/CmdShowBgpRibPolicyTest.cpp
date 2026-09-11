/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

// The rib-policy commands live under commands/show/facebook and are not part
// of the OSS build.
#ifndef IS_OSS

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

#include "fboss/cli/fboss2/commands/show/facebook/bgp/ribpolicy/CmdShowBgpRibPolicyCps.h"
#include "fboss/cli/fboss2/commands/show/facebook/bgp/ribpolicy/CmdShowBgpRibPolicyCrf.h"
#include "fboss/cli/fboss2/commands/show/facebook/bgp/ribpolicy/CmdShowBgpRibPolicyCte.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdShowBgpRibPolicyTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowBgpRibPolicyTestFixture, cpsWikiDocHooks) {
  EXPECT_FALSE(CmdShowBgpRibPolicyCpsTraits::description().empty());
  EXPECT_FALSE(CmdShowBgpRibPolicyCps::sampleModel().statements()->empty());

  std::stringstream ss;
  CmdShowBgpRibPolicyCps().printOutput(
      CmdShowBgpRibPolicyCps::sampleModel(), ss);
  const std::string output = ss.str();

  EXPECT_THAT(output, HasSubstr("path selection policy (version 1732574075)"));
  EXPECT_THAT(output, HasSubstr("route/prefix matcher"));
  // Ordered criteria are the part of the render the description explains, so
  // the example has to show more than one.
  EXPECT_THAT(output, HasSubstr("criteria 1"));
  EXPECT_THAT(output, HasSubstr("criteria 2"));
}

TEST_F(CmdShowBgpRibPolicyTestFixture, crfWikiDocHooks) {
  EXPECT_FALSE(CmdShowBgpRibPolicyCrfTraits::description().empty());
  EXPECT_TRUE(
      CmdShowBgpRibPolicyCrf::sampleModel().golden_prefix_policy().has_value());

  std::stringstream ss;
  CmdShowBgpRibPolicyCrf().printOutput(
      CmdShowBgpRibPolicyCrf::sampleModel(), ss);
  const std::string output = ss.str();

  EXPECT_THAT(output, HasSubstr("route filter policy (version 1784328957)"));
  EXPECT_THAT(output, HasSubstr("golden prefix"));
  EXPECT_THAT(output, HasSubstr("0.0.0.0/0"));
  EXPECT_THAT(output, HasSubstr("limit: 1"));
}

// An empty route-attribute policy is the documented steady state, so the
// sample is deliberately empty and must still render its header.
TEST_F(CmdShowBgpRibPolicyTestFixture, cteWikiDocHooks) {
  EXPECT_FALSE(CmdShowBgpRibPolicyCteTraits::description().empty());
  EXPECT_TRUE(CmdShowBgpRibPolicyCte::sampleModel().statements()->empty());

  std::stringstream ss;
  CmdShowBgpRibPolicyCte().printOutput(
      CmdShowBgpRibPolicyCte::sampleModel(), ss);
  EXPECT_THAT(ss.str(), HasSubstr("route attribute policy"));
}

} // namespace facebook::fboss

#endif // IS_OSS
