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
#include <boost/variant.hpp>
#include <clang/Tooling/Tooling.h>
#include <optional>

namespace facebook { namespace fboss {

/*
 * Abstract base class for all the various Thrift IDL objects we support.
 * The concrete classes roughly follow the Thrift IDL grammar defined at
 * https://thrift.apache.org/docs/idl
 */
class ThriftIDLObject {
 public:
  virtual ~ThriftIDLObject() {}
  virtual std::string getThrift() const = 0;
  std::string name;
};

/*
 * Represents a thrift type as used in objects which give something
 * a type. Specifically this would be types in struct fields or in
 * method parameters.
 * ie: in a field line like: "0: i32 x", the "i32" portion is a ThriftType.
 *
 * Implementation Note:
 * Since we output list types, I found internally representing things with
 * boost::variant to be convenient. That way a type is either a name, a
 * primitive, or a list of some other valid type. We can support list<list<int>>
 * rather naturally this way.
 * (Lists are needed because all pointer types are treated as list<Pointee>)
 */
class ThriftType : public ThriftIDLObject {
 public:
  explicit ThriftType(const clang::QualType& qt);
  ThriftType();
  std::string getThrift() const override;
  enum class Primitive {
    Bool = 0,
    Byte = 1,
    Int16 = 2,
    Int32 = 3,
    Int64 = 4,
    Float = 5,
    Double = 6,
    Binary = 7,
    String = 8,
  };

  class List;
  using Type = boost::variant<
      std::string, // thrift type name (eg a struct)
      Primitive,
      boost::recursive_wrapper<List>>;

  class List {
   public:
    explicit List(const Type& elementType);
    Type elementType;
  };
  Type type;

 private:
  class ThriftGenVisitor : public boost::static_visitor<std::string> {
   public:
    std::string operator()(const std::string& name) const;
    std::string operator()(const Primitive& primitive) const;
    std::string operator()(const List& list) const;
  };
};

/*
 * Represents an indexed, typed entry in a list of fields. In particular,
 * this is used to represent fields in structs/unions and parameters of
 * methods.
 */
class ThriftField : public ThriftIDLObject {
 public:
  ThriftField(int index, const clang::ValueDecl& field);
  ThriftField(
      const std::string& name,
      int index,
      const clang::QualType& qualifiedType);
  std::string getThrift() const override;
 private:
  int index_;
  ThriftType type_;
};

/*
 * Represents a struct. Can be constructed from a clang AST struct declaration
 * or directly from a clang ast function declaration in the case of generating a
 * struct for the return value of a method.
 */
class ThriftStruct : public ThriftIDLObject {
 public:
  explicit ThriftStruct(const clang::RecordDecl& record);
  explicit ThriftStruct(const clang::FunctionDecl& function);
  std::string getThrift() const override;
  bool hasFields() const;
 private:
  std::vector<std::unique_ptr<ThriftField>> fields_;
};

/*
 * Represents one possible value for an enum
 */
class ThriftEnumerator : public ThriftIDLObject {
 public:
  explicit ThriftEnumerator(const clang::EnumConstantDecl& enumConstant);
  std::string getThrift() const override;
 private:
  int value_;
};

/*
 * Represents an enum. Constructed from enum declarations in the clang AST
 */
class ThriftEnum : public ThriftIDLObject {
 public:
  explicit ThriftEnum(const clang::EnumDecl& en);
  std::string getThrift() const override;
 private:
  std::vector<std::unique_ptr<ThriftEnumerator>> enumerators_;
};

/*
 * Represents a method inside a service. Constructed from function
 * declarations in the clang AST.
 */
class ThriftMethod : public ThriftIDLObject {
 public:
  explicit ThriftMethod(const clang::FunctionDecl& function);
  std::string getThrift() const override;
  static std::string methodName(const std::string& functionName);
  std::string resultTypeName;
 private:
  std::vector<std::unique_ptr<ThriftField>> parameters_;
};

/*
 * Represents a service. Has a list of methods and exposes the ability to
 * extract all of their generated return types so that they can be included
 * in the file.
 */
class ThriftService : public ThriftIDLObject {
 public:
  std::string getThrift() const override;
  void addMethod(std::unique_ptr<ThriftMethod> method);
 private:
  std::vector<std::unique_ptr<ThriftMethod>> methods_;
};

/*
 * Represents the entire thrift file. This is the top level object which
 * will contain all the other IDL objects.
 */
class ThriftFile : public ThriftIDLObject {
 public:
  explicit ThriftFile(const std::string& name);
  std::string getThrift() const override;
  void addEnum(std::unique_ptr<ThriftEnum> en);
  // Adding a method also adds the struct representing the return value
  void addMethod(std::unique_ptr<ThriftMethod> method);
  void addStruct(std::unique_ptr<ThriftStruct> st);
 private:
  std::vector<std::unique_ptr<ThriftEnum>> enums_;
  // While thrift supports multiple services in a file, for now we generate
  // just one, so don't bother with the extra complication
  ThriftService service_;
  std::vector<std::unique_ptr<ThriftStruct>> structs_;
};

}} // facebook::fboss
