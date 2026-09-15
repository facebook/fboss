// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/visitors/ExtendedPathMatcher.h"

#include <gtest/gtest.h>

namespace facebook::fboss::thrift_cow {
namespace {

using fsdb::OperPathElem;

OperPathElem rawElem(std::string token) {
  OperPathElem elem;
  elem.set_raw(std::move(token));
  return elem;
}

OperPathElem regexElem(std::string regexStr) {
  OperPathElem elem;
  elem.set_regex(std::move(regexStr));
  return elem;
}

OperPathElem anyElem() {
  OperPathElem elem;
  elem.set_any(true);
  return elem;
}

} // namespace

TEST(ExtendedPathMatcherTest, anyMatchesEverything) {
  auto elem = anyElem();
  EXPECT_TRUE(matchesStrToken("anything", elem));
  EXPECT_TRUE(matchesStrToken("", elem));
  EXPECT_TRUE(matchesStrToken("123", elem));
}

TEST(ExtendedPathMatcherTest, rawMatchesExactlyOnly) {
  auto elem = rawElem("eth0");
  EXPECT_TRUE(matchesStrToken("eth0", elem));
  EXPECT_FALSE(matchesStrToken("eth1", elem));
  EXPECT_FALSE(matchesStrToken("eth0 ", elem));
  EXPECT_FALSE(matchesStrToken("", elem));
}

TEST(ExtendedPathMatcherTest, regexUsesFullMatch) {
  auto elem = regexElem("port[0-9]+");
  EXPECT_TRUE(matchesStrToken("port42", elem));
  // FullMatch: a leading/trailing extra char must fail
  EXPECT_FALSE(matchesStrToken("Xport42", elem));
  EXPECT_FALSE(matchesStrToken("port42X", elem));
  EXPECT_FALSE(matchesStrToken("port", elem));
}

TEST(ExtendedPathMatcherTest, numericTokenStringParity) {
  // Tokens reaching the matcher are already stringified; raw/regex compare
  // against the string form, matching how the visitor stringifies int/enum
  // keys before calling matchesStrToken.
  EXPECT_TRUE(matchesStrToken("3", rawElem("3")));
  EXPECT_FALSE(matchesStrToken("3", rawElem("4")));
  EXPECT_TRUE(matchesStrToken("3", regexElem("[0-9]+")));
}

TEST(ExtendedPathMatcherTest, emptyElemNeverMatches) {
  // An ill-formed (unset union) elem should not match any token.
  OperPathElem empty;
  EXPECT_FALSE(matchesStrToken("foo", empty));
}

TEST(ExtendedPathMatcherTest, precompiledRegexUsesFullMatch) {
  const auto elem = regexElem("port[0-9]+");
  const re2::RE2 regex(*elem.regex());
  EXPECT_TRUE(matchesStrToken("port42", elem, &regex));
  EXPECT_FALSE(matchesStrToken("Xport42", elem, &regex));
  EXPECT_FALSE(matchesStrToken("port42X", elem, &regex));
}

TEST(ExtendedPathMatcherTest, invalidPrecompiledRegexNeverMatches) {
  const auto elem = regexElem("[");
  const re2::RE2 regex(*elem.regex());
  EXPECT_FALSE(regex.ok());
  EXPECT_FALSE(matchesStrToken("anything", elem, &regex));
}

TEST(ExtendedPathMatcherTest, wildcardPredicates) {
  OperPathElem empty;
  EXPECT_FALSE(isWildcardElem(empty));
  EXPECT_FALSE(isWildcardElem(rawElem("raw")));
  EXPECT_TRUE(isWildcardElem(anyElem()));
  EXPECT_TRUE(isWildcardElem(regexElem(".*")));

  EXPECT_FALSE(hasWildcard({}));
  EXPECT_FALSE(hasWildcard({empty}));
  EXPECT_FALSE(hasWildcard({rawElem("a"), empty}));
  EXPECT_FALSE(hasWildcard({rawElem("a"), rawElem("b")}));
  EXPECT_TRUE(hasWildcard({rawElem("a"), anyElem()}));
  EXPECT_TRUE(hasWildcard({rawElem("a"), regexElem("b.*")}));
}

} // namespace facebook::fboss::thrift_cow
