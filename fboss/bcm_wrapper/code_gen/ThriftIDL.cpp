/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "ThriftIDL.h"

#include <sstream>
#include <stdexcept>

#include <folly/Format.h>
#include <folly/String.h>

namespace {
// struct_name -> structName
void snakeToCamel(std::string& s) {
  int numSeen = 0;
  bool shouldCapitalize = false;
  for (int i = 0; i < s.size(); ++i) {
    int target = i + numSeen;
    while (target < s.size() && s.at(target) == '_') {
      ++target;
      ++numSeen;
      shouldCapitalize = true;
    }
    if (target < s.size() && numSeen) {
      std::swap(s.at(i), s.at(target));
      if (shouldCapitalize) {
        auto upper = toupper(s.at(i));
        s.at(i) = upper;
        shouldCapitalize = false;
      }
    }
  }
  s.resize(s.size() - numSeen);
}

// fooBar -> FooBar
void capitalize(std::string& s) {
  if (s.empty()) {
    return;
  }
  auto upper = toupper(s.at(0));
  s.at(0) = upper;
}

// These various "stripping" functions are a bit of a kludge. I think I can fix
// this by properly handling typedef declarations rather than relying on a
// naming scheme pattern.
void stripPrefix(std::string& s, const std::string& prefix) {
  std::string::size_type structLoc = s.find(prefix);
  if (structLoc != std::string::npos && structLoc == 0) {
    s.erase(0, prefix.size());
  }
}

// remove leading "struct "
void stripStructPrefix(std::string& s) {
  stripPrefix(s, "struct ");
}

// remove leading "enum "
void stripEnumPrefix(std::string& s) {
  stripPrefix(s, "enum ");
}

bool stripSuffix(std::string& s, const std::string& suffix) {
  std::string::size_type loc = s.find_last_of(suffix);
  if (loc == s.size() - suffix.size()) {
    s.erase(loc, suffix.size());
    return true;
  }
  return false;
}

// remove trailing "_t" or "_s"
void stripStructSuffix(std::string& s) {
  if (stripSuffix(s, "_t")) {
    return;
  }
  stripSuffix(s, "_s");
}

// remove trailing "_t" or "_e"
void stripEnumSuffix(std::string& s) {
  if (stripSuffix(s, "_t")) {
    return;
  }
  stripSuffix(s, "_e");
}

void normalizeEnumName(std::string& s) {
  stripEnumPrefix(s);
  stripEnumSuffix(s);
  snakeToCamel(s);
  capitalize(s);
}

void normalizeStructName(std::string& s) {
  stripStructPrefix(s);
  stripStructSuffix(s);
  snakeToCamel(s);
  capitalize(s);
}
} // namespace

namespace facebook { namespace fboss {

ThriftType::ThriftType(const clang::QualType& qt) {
  if (qt->isStructureType()) {
    std::string t = qt.getAsString();
    normalizeStructName(t);
    type = t;
  } else if (qt->isEnumeralType()) {
    std::string t = qt.getAsString();
    normalizeEnumName(t);
    type = t;
  } else if (qt->isPointerType()) {
    ThriftType elementType(qt->getPointeeType());
    type = List(elementType.type);
  } else if (qt->isIntegerType()) {
    std::string t = qt.getAsString();
    if (t == "bool") {
      type = ThriftType::Primitive::Bool;
    }
    else if (t == "int") {
      type = ThriftType::Primitive::Int32;
    }
  } else if (qt->isVoidType()) {
    type = "void";
  } else {
    type = "UNMATCHED_TYPE";
  }
  name = boost::apply_visitor(ThriftType::ThriftGenVisitor(), type);
}

ThriftType::ThriftType() {
  type = "void";
  name = boost::apply_visitor(ThriftType::ThriftGenVisitor(), type);
}

std::string ThriftType::getThrift() const {
  return name;
}

ThriftType::List::List(const ThriftType::Type& elementType) {
  this->elementType = elementType;
}

std::string ThriftType::ThriftGenVisitor::operator()(
    const std::string& name) const {
  return name;
}

std::string ThriftType::ThriftGenVisitor::operator()(
    const ThriftType::Primitive& primitive) const {
  switch (primitive) {
    case ThriftType::Primitive::Bool:
      return "bool";
    case ThriftType::Primitive::Byte:
      return "byte";
    case ThriftType::Primitive::Int16:
      return "i16";
    case ThriftType::Primitive::Int32:
      return "i32";
    case ThriftType::Primitive::Int64:
      return "i64";
    case ThriftType::Primitive::Float:
      return "float";
    case ThriftType::Primitive::Double:
      return "double";
    case ThriftType::Primitive::Binary:
      return "binary";
    case ThriftType::Primitive::String:
      return "string";
    default:
      return "unknown primitive";
  }
}

std::string ThriftType::ThriftGenVisitor::operator()(
    const ThriftType::List& list) const {
  return folly::sformat(
      "list<{}>",
      boost::apply_visitor(ThriftType::ThriftGenVisitor(), list.elementType));
}

ThriftEnumerator::ThriftEnumerator(
    const clang::EnumConstantDecl& enumConstant) {
  name = enumConstant.getName().str();
  value_ = enumConstant.getInitVal().getExtValue();
}

std::string ThriftEnumerator::getThrift() const {
  return folly::sformat("{} = {}", name, value_);
}

ThriftEnum::ThriftEnum(const clang::EnumDecl& en) {
  name = en.getName().str();
  normalizeEnumName(name);
  for (const clang::EnumConstantDecl* enumConstant : en.enumerators()) {
    enumerators_.emplace_back(
        std::make_unique<ThriftEnumerator>(*enumConstant));
  }
}

std::string ThriftEnum::getThrift() const {
  std::stringstream ss;
  ss << "enum " << name << "\n{\n";
  for (const auto& enumerator : enumerators_) {
    ss << "    " << enumerator->getThrift() << ",\n";
  }
  ss << "}";
  return ss.str();
}

ThriftField::ThriftField(int index, const clang::ValueDecl& decl)
    : ThriftField(decl.getName().str(), index, decl.getType()) {}

ThriftField::ThriftField(
    const std::string& name,
    int index,
    const clang::QualType& qualifiedType)
    : index_(index) {
  if (qualifiedType->isFunctionPointerType()) {
    throw std::runtime_error("can't make a thrift field with fn ptr");
  }
  this->name = name;
  type_ = ThriftType(qualifiedType);
}

std::string ThriftField::getThrift() const {
  return folly::sformat("{}: {} {}", index_, type_.getThrift(), name);
}

ThriftStruct::ThriftStruct(const clang::RecordDecl& record) {
  name = record.getName().str();
  normalizeStructName(name);
  int index = 0;
  for (const clang::FieldDecl* field : record.fields()) {
    fields_.emplace_back(std::make_unique<ThriftField>(index, *field));
    ++index;
  }
}

ThriftStruct::ThriftStruct(const clang::FunctionDecl& function) {
  name = folly::sformat(
      "{}Result", ThriftMethod::methodName(function.getName().str()));
  normalizeStructName(name);
  int index = 0;
  auto rt = function.getReturnType();
  if (!rt->isVoidType()) {
    fields_.emplace_back(std::make_unique<ThriftField>("retVal", index, rt));
    ++index;
  }
  for (const clang::ParmVarDecl* param : function.parameters()) {
    if (param->getType()->isPointerType()) {
      fields_.emplace_back(std::make_unique<ThriftField>(
          param->getName().str(), index, param->getType()));
      ++index;
    }
  }
}

std::string ThriftStruct::getThrift() const {
  std::stringstream ss;
  ss << "struct " << name << "\n{\n";
  for (const auto& field : fields_) {
    ss << "    " << field->getThrift() << "\n";
  }
  ss << "}";
  return ss.str();
}

bool ThriftStruct::hasFields() const {
  return not fields_.empty();
}

ThriftMethod::ThriftMethod(const clang::FunctionDecl& function) {
  name = methodName(function.getName().str());
  resultTypeName = "void";
  int index = 0;
  for (const clang::ParmVarDecl* param : function.parameters()) {
    parameters_.emplace_back(std::make_unique<ThriftField>(index, *param));
    ++index;
  }
}

std::string ThriftMethod::methodName(const std::string& functionName) {
  std::string s = functionName;
  snakeToCamel(s);
  return s;
}

std::string ThriftMethod::getThrift() const {
  std::stringstream ss;
  ss << resultTypeName << " ";
  ss << name << "(";
  int i = 0;
  for (const auto& param : parameters_) {
    ss << param->getThrift();
    if (++i < parameters_.size()) {
      ss << ", ";
    }
  }
  ss << ")";
  return ss.str();
}

void ThriftService::addMethod(std::unique_ptr<ThriftMethod> method) {
  methods_.push_back(std::move(method));
}

std::string ThriftService::getThrift() const {
  std::stringstream ss;
  ss << "service " << name << "\n{\n";
  for (const auto& method : methods_) {
    ss << "    " << method->getThrift() << "\n";
  }
  ss << "}";
  return ss.str();
}

ThriftFile::ThriftFile(const std::string& name) {
  this->name = name;
  service_.name = folly::sformat("{}Service", name);
}

void ThriftFile::addEnum(std::unique_ptr<ThriftEnum> en) {
  enums_.push_back(std::move(en));
}

void ThriftFile::addMethod(std::unique_ptr<ThriftMethod> method) {
  service_.addMethod(std::move(method));
}

void ThriftFile::addStruct(std::unique_ptr<ThriftStruct> st) {
  structs_.push_back(std::move(st));
}

std::string ThriftFile::getThrift() const {
  std::stringstream ss;
  for (const auto& en : enums_) {
    ss << en->getThrift() << "\n";
  }
  for (const auto& st : structs_) {
    ss << st->getThrift() << "\n";
  }
  ss << service_.getThrift() << "\n";
  return ss.str();
}

}} // facebook::fboss
