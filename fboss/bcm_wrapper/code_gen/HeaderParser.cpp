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

#include <stdexcept>

namespace facebook { namespace fboss {

const clang::ast_matchers::DeclarationMatcher&
HeaderParser::recordDeclMatcher() {
  static const clang::ast_matchers::DeclarationMatcher matcher =
      clang::ast_matchers::recordDecl().bind("rd");
  return matcher;
}

const clang::ast_matchers::DeclarationMatcher& HeaderParser::enumDeclMatcher() {
  static const clang::ast_matchers::DeclarationMatcher matcher =
      clang::ast_matchers::enumDecl().bind("ed");
  return matcher;
}

const clang::ast_matchers::DeclarationMatcher&
HeaderParser::functionDeclMatcher() {
  static const clang::ast_matchers::DeclarationMatcher matcher =
      clang::ast_matchers::functionDecl().bind("fd");
  return matcher;
}

HeaderParser::HeaderParser()
    : file_(std::make_unique<ThriftFile>("BcmWrapper")) {}

void HeaderParser::run(
    const clang::ast_matchers::MatchFinder::MatchResult& result) {
  // Parse structs, unions, and class declarations
  const clang::RecordDecl* rd = result.Nodes.getNodeAs<clang::RecordDecl>("rd");
  if (rd) {
    try {
      file_->addStruct(std::make_unique<ThriftStruct>(*rd));
    } catch (const std::exception& e) {
      // TODO(borisb): this case should generate thrift comments calling for
      // manual code that needs to be written.
    }
    return;
  }
  // Parse enum declarations
  const clang::EnumDecl* ed = result.Nodes.getNodeAs<clang::EnumDecl>("ed");
  if (ed) {
    file_->addEnum(std::make_unique<ThriftEnum>(*ed));
    return;
  }
  // Parse function declarations
  const clang::FunctionDecl* fd =
      result.Nodes.getNodeAs<clang::FunctionDecl>("fd");
  if (fd) {
    try {
      auto tm = std::make_unique<ThriftMethod>(*fd);
      auto ts = std::make_unique<ThriftStruct>(*fd);
      if (ts->hasFields()) {
        tm->resultTypeName = ts->name;
        file_->addStruct(std::move(ts));
      }
      file_->addMethod(std::move(tm));
    } catch (const std::exception& e) {
      // TODO(borisb): this case should generate thrift comments calling for
      // manual code that needs to be written.
    }
    return;
  }
}

std::string HeaderParser::getThrift() const {
  return file_->getThrift();
};

}} // facebook::fboss
