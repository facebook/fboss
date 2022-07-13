// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <thrift/lib/cpp2/visitation/for_each.h>
#include <thrift/lib/cpp2/visitation/metadata.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types.h>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"

namespace facebook::fboss {

template <class T>
using get_value_type_t = typename T::value_type;
using ThriftField = apache::thrift::metadata::ThriftField;
using ThriftType = apache::thrift::metadata::ThriftType;
using ValidFilterMapType = std::unordered_map<
    std::string_view,
    std::shared_ptr<CmdGlobalOptions::BaseTypeVerifier>>;

template <typename RowType>
bool satisfiesFilterTerm(
    const RowType& row,
    const CmdGlobalOptions::FilterTerm& filterTerm,
    const ValidFilterMapType& validFilterMap) {
  bool satisfies = true;
  // todo(surabhi236): implement logic to check if row satisfies the filter
  // condition
  return satisfies;
}

// returns true if all predicates in the given intersection list are satisfied
template <typename RowType>
bool satisfiesAllPredicates(
    const RowType& row,
    const CmdGlobalOptions::IntersectionList& intersectList,
    const ValidFilterMapType& validFilterMap) {
  for (const auto& filterTerm : intersectList) {
    if (!satisfiesFilterTerm(row, filterTerm, validFilterMap)) {
      return false;
    }
  }
  return true;
}

// returns true if atleast one intersection list is satisfied in the given union
// list
template <typename RowType>
bool satisfiesFilterCondition(
    const RowType& row,
    const CmdGlobalOptions::UnionList& unionList,
    const ValidFilterMapType& validFilterMap) {
  for (const auto& intersectList : unionList) {
    if (satisfiesAllPredicates(row, intersectList, validFilterMap)) {
      return true;
    }
  }
  return false;
}

/* logic: We use the result model obtained from queryClient and perform thrift
reflection on the model fields to obtain the field_refs (which correspond to the
result values). Here, the model would be a struct containing a single list of
structs(this check is statically done). Once we obtain the inner list, we check
if each row in the list satisifes the input filter condition. The rows that do
not satisfy are deleted. The resultant model (otatined after filtering out bad
rows) is returned at the end.
*/
template <typename CmdTypeT>
typename CmdTypeT::RetType filterOutput(
    typename CmdTypeT::RetType model,
    const facebook::fboss::CmdGlobalOptions::UnionList& parsedFilters,
    const ValidFilterMapType& validFilterMap) {
  if constexpr (apache::thrift::is_thrift_struct_v<
                    typename CmdTypeT::RetType>) {
    apache::thrift::for_each_field(
        model, [&](const ThriftField& /*outer_meta*/, auto&& outer_field_ref) {
          if constexpr (apache::thrift::is_thrift_struct_v<folly::detected_or_t<
                            void,
                            get_value_type_t,
                            folly::remove_cvref_t<
                                decltype(*outer_field_ref)>>>) {
            using NestedType = get_value_type_t<
                folly::remove_cvref_t<decltype(*outer_field_ref)>>;
            if constexpr (apache::thrift::is_thrift_struct_v<NestedType>) {
              auto innerEntryVector = *outer_field_ref;
              auto removed = std::remove_if(
                  innerEntryVector.begin(),
                  innerEntryVector.end(),
                  [&](auto innerEntry) {
                    return !satisfiesFilterCondition(
                        innerEntry, parsedFilters, validFilterMap);
                  });
              innerEntryVector.erase(removed, innerEntryVector.end());
              *outer_field_ref = innerEntryVector;
            }
          }
        });
  }
  return model;
}
} // namespace facebook::fboss
