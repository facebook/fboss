// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

// HACK: This is referencing a python library's (py-radix)
// C code implementation
extern "C" {
#include "radix.h"
void* PyMem_Malloc(size_t n) {
  return malloc(n);
}
void PyMem_Free(void* p) {
  free(p);
}
}
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <memory>

#include "Utils.h"
#include "common/logging/logging.h"

// Not implemented. Only exists so we can specialize
// for v4 and v6
template <typename IPAddrType>
struct PyRadixRootAdapter;

template <>
struct PyRadixRootAdapter<folly::IPAddressV4> {
  template <typename PyTree>
  static const radix_node_t* getRoot(const PyTree& treePtr) {
    return treePtr->head_ipv4;
  }
};

template <>
struct PyRadixRootAdapter<folly::IPAddressV6> {
  template <typename PyTree>
  static const radix_node_t* getRoot(const PyTree& treePtr) {
    return treePtr->head_ipv6;
  }
};

/*
 * Quick and dirty PyRadixWrapper.
 * Sole purpose of this is to enable us to test by comparing against a widely
 * used radix tree implementation
 */
template <typename IPAddrType, typename T>
class PyRadixWrapper {
 public:
  explicit PyRadixWrapper() : tree_(New_Radix()) {}

  bool insert(const IPAddrType& ip, uint8_t mask, const T& value) {
    auto data = new std::shared_ptr<T>(new T(value));
    auto radixPfx = makePrefix(ip, mask);
    if (!radixPfx) {
      VLOG(0) << "Unable to make prefix/find search tree for prefix "
              << ip.str() << "/" << mask;
      return false;
    }
    // Look up radix node, fail if we find a non-empty one.
    auto radixNode = radix_search_exact(tree_.get(), radixPfx.get());
    if (radixNode) {
      if (radixNode->data) {
        VLOG(0) << "Radix node data exists for " << ip.str() << "/" << mask
                << ", no insertion performed.";
        return false;
      }
    }
    if (!radixNode) {
      VLOG(0) << "Attempting to create radix node.";
      radixNode = radix_lookup(tree_.get(), radixPfx.get());
      if (!radixNode) {
        LOG(INFO) << "Unable to allocate radix node.";
        return false;
      }
    }
    // Store data.
    radixNode->data = data;
    return true;
  }

  bool erase(const IPAddrType& ip, int prefix) {
    auto radixNode = findByPrefixNode(ip, prefix);
    if (radixNode == nullptr) {
      return false;
    }
    auto ptr = static_cast<std::shared_ptr<T>*>(radixNode->data);
    if (ptr) {
      delete ptr;
      radixNode->data = nullptr;
    }
    radix_remove(tree_.get(), radixNode);
    return 1;
  }

  std::shared_ptr<T> longestMatch(const IPAddrType& addr, int prefix) {
    auto rawData = findClosestRaw(addr, prefix);
    return rawData ? *(static_cast<std::shared_ptr<T>*>(rawData)) : nullptr;
  }

  std::shared_ptr<T> exactMatch(const IPAddrType& ip, int prefix) {
    auto rawData = findByPrefixRaw(ip, prefix);
    return rawData ? *(static_cast<std::shared_ptr<T>*>(rawData)) : nullptr;
  }

  const radix_node_t* root() const {
    return PyRadixRootAdapter<IPAddrType>::getRoot(tree_);
  }

 private:
  // Deleter for radix tree.
  class RadixDelete {
   public:
    static void node_delete_callback(radix_node_t* node, void* /*arg*/) {
      auto ptr = static_cast<std::shared_ptr<T>*>(node->data);
      delete ptr;
      node->data = nullptr;
    }
    void operator()(radix_tree_t* tree) {
      Destroy_Radix(tree, node_delete_callback, nullptr);
    }
  };

  class RadixPrefixDelete {
   public:
    void operator()(prefix_t* pfx) {
      Deref_Prefix(pfx);
    }
  };

  void* findByPrefixRaw(const IPAddrType& ip, int prefix) {
    auto radixNode = findByPrefixNode(ip, prefix);
    if (!radixNode || !radixNode->data) {
      VLOG(0) << "No data found for " << ip.str() << "/" << prefix;
      return nullptr;
    }
    return radixNode->data;
  }

  void* findClosestRaw(const IPAddrType& ip, int /*prefix*/) {
    auto radixPfx = makePrefix(ip, ip.bitCount());
    auto radixNode = radix_search_best(tree_.get(), radixPfx.get());
    if (!radixNode || !radixNode->data) {
      VLOG(0) << "No closest match data found for " << ip.str();
      return nullptr;
    }
    return radixNode->data;
  }

  std::unique_ptr<prefix_t, RadixPrefixDelete> makePrefix(
      const IPAddrType& ip,
      int prefix) {
    std::unique_ptr<prefix_t, RadixPrefixDelete> radixPfx;
    radixPfx.reset(prefix_from_blob(
        ip.toByteArray().data(), // bytes
        IPAddrType::byteCount(),
        prefix));
    return std::move(radixPfx);
  }

  radix_node_t* findByPrefixNode(const IPAddrType& ip, int prefix) {
    auto radixPfx = makePrefix(ip, prefix);
    return radix_search_exact(tree_.get(), radixPfx.get());
  }

  std::unique_ptr<radix_tree_t, RadixDelete> tree_;
};
