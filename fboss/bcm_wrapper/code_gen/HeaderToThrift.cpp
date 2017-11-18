/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "HeaderParser.h"

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

static llvm::cl::extrahelp commonHelp(
    clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::OptionCategory headerToThriftCategory(
    "HeaderToThrift options");

int main(int argc, char **argv) {
  clang::tooling::CommonOptionsParser optionsParser(
      argc, const_cast<const char**>(argv), headerToThriftCategory);
  clang::tooling::ClangTool tool(
      optionsParser.getCompilations(), optionsParser.getSourcePathList());
  facebook::fboss::HeaderParser hp;
  clang::ast_matchers::MatchFinder mf;
  mf.addMatcher(facebook::fboss::HeaderParser::recordDeclMatcher(), &hp);
  mf.addMatcher(facebook::fboss::HeaderParser::enumDeclMatcher(), &hp);
  mf.addMatcher(facebook::fboss::HeaderParser::functionDeclMatcher(), &hp);
  tool.run(clang::tooling::newFrontendActionFactory(&mf).get());
  llvm::outs() << hp.getThrift() << "\n";
}
