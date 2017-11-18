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

#include <string>
#include <memory>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>

#include "ThriftIDL.h"

namespace facebook { namespace fboss {
/*
 * This class implements the visitor function that we run on matching nodes in
 * the clang AST. It also owns the matchers we use to determine whether to run
 * on a node or not. Currently, we visit record (class, union, struct)
 * declarations, enum declarations and function declarations.
 *
 * As we visit those declarations, we populate objects which represent ThrifIDL
 * objects corresponding to those declarations which we can finally use to
 * output thrift corresponding to the processed header file.
 */
class HeaderParser : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  HeaderParser();
  void run(
      const clang::ast_matchers::MatchFinder::MatchResult& result) override;
  // matcher for structs, unions, and classes
  static const clang::ast_matchers::DeclarationMatcher& recordDeclMatcher();
  // matcher for enums
  static const clang::ast_matchers::DeclarationMatcher& enumDeclMatcher();
  // matcher for functions
  static const clang::ast_matchers::DeclarationMatcher& functionDeclMatcher();
  // return the the thrift code we have generated while processing the header
  std::string getThrift() const;
 private:
  // These will store the generated Thrift IDL objects we parse as we go.
  // We need to store because functions will result in methods (the function
  // itself) and structs (the return type) so we have to write out the file
  // at the end rather than as we go.
  std::unique_ptr<ThriftFile> file_;
};
}} // facebook::fboss
