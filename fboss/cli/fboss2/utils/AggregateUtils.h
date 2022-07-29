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
using ValidAggMapType = std::unordered_map<
    std::string_view,
    std::shared_ptr<CmdGlobalOptions::BaseAggInfo>>;

template <typename RowType>
void aggregationHelper(
    RowType row,
    const std::optional<facebook::fboss::CmdGlobalOptions::AggregateOption>&
        parsedAggs,
    const ValidAggMapType& validAggMap,
    int rowNumber,
    double& aggAnswer) {
  apache::thrift::for_each_field(
      row, [&](const ThriftField& meta, auto&& field_ref) {
        const auto& aggColumn = parsedAggs->columnName;
        const auto& aggOp = parsedAggs->aggOp;
        // this is the colum being aggregated
        if (*meta.name() == aggColumn) {
          using FieldType = folly::remove_cvref_t<decltype(*field_ref)>;
          if constexpr (std::is_fundamental<FieldType>::value) {
            auto it = validAggMap.find(aggColumn);
            // todo(surabih236): separate handling for string and numeric types
            // to aboid doing extra type conversions.
            const auto& value = std::to_string(*field_ref);
            if (it != validAggMap.end()) {
              if (rowNumber == 0) {
                aggAnswer = (it->second)->getInitValue(value, aggOp);
              } else {
                aggAnswer =
                    (it->second)->performAggregate(value, aggAnswer, aggOp);
              }
            }
          } else if constexpr (std::is_same<FieldType, std::string>::value) {
            auto it = validAggMap.find(aggColumn);
            if (it != validAggMap.end()) {
              if (rowNumber == 0) {
                aggAnswer = (it->second)->getInitValue(*field_ref, aggOp);
              } else {
                aggAnswer =
                    (it->second)
                        ->performAggregate(*field_ref, aggAnswer, aggOp);
              }
            }
          }
        }
      });
}

template <typename CmdTypeT>
double performAggregation(
    typename CmdTypeT::RetType model,
    const std::optional<facebook::fboss::CmdGlobalOptions::AggregateOption>&
        parsedAggs,
    const ValidAggMapType& validAggMap) {
  double aggAnswer = 0;
  if constexpr (
      apache::thrift::is_thrift_struct_v<typename CmdTypeT::RetType> &&
      CmdTypeT::Traits::ALLOW_AGGREGATION) {
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
              for (int rowNumber = 0; rowNumber < innerEntryVector.size();
                   rowNumber++) {
                auto innerEntry = innerEntryVector[rowNumber];
                aggregationHelper(
                    innerEntry, parsedAggs, validAggMap, rowNumber, aggAnswer);
              }
            }
          }
        });
  }
  return aggAnswer;
}
} // namespace facebook::fboss
