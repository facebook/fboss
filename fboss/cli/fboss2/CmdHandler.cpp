/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/utils/CmdCommonUtils.h"
#include "thrift/lib/cpp/util/EnumUtils.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include <cstring>
#include <future>
#include <iostream>
#include <stdexcept>

template <typename CmdTypeT>
void printTabular(
    CmdTypeT& cmd,
    std::vector<std::shared_future<
        std::tuple<std::string, typename CmdTypeT::RetType, std::string>>>&
        results,
    std::ostream& out,
    std::ostream& err) {
  for (auto& result : results) {
    auto [host, data, errStr] = result.get();
    if (results.size() != 1) {
      out << host << "::" << std::endl << std::string(80, '=') << std::endl;
    }

    if (errStr.empty()) {
      cmd.printOutput(data);
    } else {
      err << errStr << std::endl << std::endl;
    }
  }
}

template <typename CmdTypeT>
void printJson(
    const CmdTypeT& /* cmd */,
    std::vector<std::shared_future<
        std::tuple<std::string, typename CmdTypeT::RetType, std::string>>>&
        results,
    std::ostream& out,
    std::ostream& err) {
  std::map<std::string, typename CmdTypeT::RetType> hostResults;
  for (auto& result : results) {
    auto [host, data, errStr] = result.get();
    if (errStr.empty()) {
      hostResults[host] = data;
    } else {
      err << host << "::" << std::endl << std::string(80, '=') << std::endl;
      err << errStr << std::endl << std::endl;
    }
  }

  out << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
             hostResults)
      << std::endl;
}

template <typename CmdTypeT>
void printAggregate(
    const std::optional<facebook::fboss::CmdGlobalOptions::AggregateOption>&
        parsedAgg,
    std::vector<std::shared_future<
        std::tuple<std::string, typename CmdTypeT::RetType, std::string>>>&
        results,
    const facebook::fboss::ValidAggMapType& validAggMap) {
  if (!parsedAgg->acrossHosts) {
    for (auto& result : results) {
      auto [host, data, errStr] = result.get();
      if (errStr.empty()) {
        std::cout << host << " Aggregation result:: "
                  << facebook::fboss::performAggregation<CmdTypeT>(
                         data, parsedAgg, validAggMap)
                  << std::endl;
      } else {
        std::cerr << host << "::" << std::endl
                  << std::string(80, '=') << std::endl;
        std::cerr << errStr << std::endl << std::endl;
      }
    }
  } else {
    // double because aggregation results are doubles.
    std::vector<double> hostAggResults;
    for (auto& result : results) {
      auto [host, data, errStr] = result.get();
      if (errStr.empty()) {
        hostAggResults.push_back(facebook::fboss::performAggregation<CmdTypeT>(
            data, parsedAgg, validAggMap));
      } else {
        std::cerr << host << "::" << std::endl
                  << std::string(80, '=') << std::endl;
        std::cerr << errStr << std::endl << std::endl;
      }
    }

    double aggregateAcrossHosts;
    int rowNumber = 0;
    const auto& aggOp = parsedAgg->aggOp;
    const auto& aggColumn = parsedAgg->columnName;
    auto it = validAggMap.find(aggColumn);
    for (auto result : hostAggResults) {
      if (rowNumber == 0) {
        if (aggOp != facebook::fboss::AggregateOpEnum::COUNT) {
          aggregateAcrossHosts =
              (it->second)->getInitValue(std::to_string(result), aggOp);
        } else {
          // can't use init value from the countAgg() here, hence separate
          // handling
          aggregateAcrossHosts = result;
        }
        rowNumber += 1;
      } else {
        switch (aggOp) {
          case facebook::fboss::AggregateOpEnum::SUM:
            aggregateAcrossHosts = facebook::fboss::SumAgg<double>().accumulate(
                result, aggregateAcrossHosts);

            break;
          case facebook::fboss::AggregateOpEnum::MIN:
            aggregateAcrossHosts = facebook::fboss::MinAgg<double>().accumulate(
                result, aggregateAcrossHosts);
            break;
          case facebook::fboss::AggregateOpEnum::MAX:
            aggregateAcrossHosts = facebook::fboss::MaxAgg<double>().accumulate(
                result, aggregateAcrossHosts);
            break;
          // when aggregating counts across hosts, we need to sum them!
          case facebook::fboss::AggregateOpEnum::COUNT:
            aggregateAcrossHosts = facebook::fboss::SumAgg<double>().accumulate(
                result, aggregateAcrossHosts);
            break;
          case facebook::fboss::AggregateOpEnum::AVG:
            std::cerr << "Average aggregation not supported yet!" << std::endl;
            break;
        }
      }
    }
    std::cout << "Result of aggregating across all hosts: "
              << aggregateAcrossHosts << std::endl;
  }
}

namespace facebook::fboss {

static bool hasRun = false;

template <typename CmdTypeT, typename CmdTypeTraits>
void CmdHandler<CmdTypeT, CmdTypeTraits>::runHelper() {
  // Parsing library invokes every chained command handler, but we only need
  // the 'leaf' command handler to be invoked. Thus, after the first (leaf)
  // command handler is invoked, simply return.
  // TODO: explore if the parsing library provides a better way to implement
  // this.
  if (hasRun) {
    return;
  }

  hasRun = true;
  auto extraOptionsEC =
      CmdGlobalOptions::getInstance()->validateNonFilterOptions();
  if (extraOptionsEC != cli::CliOptionResult::EOK) {
    throw std::invalid_argument(folly::to<std::string>(
        "Error in filter parsing: ",
        apache::thrift::util::enumNameSafe(extraOptionsEC)));
  }

  /* If there are errors during filter parsing, we do not exit in the getFilters
  method of CmdGlobalOptions.cpp. Instead, we get the parsing error code and
  exit here. This is to avoid having living references during the
  destrowInstances time!
  */
  auto filterParsingEC = cli::CliOptionResult::EOK;
  auto parsedFilters =
      CmdGlobalOptions::getInstance()->getFilters(filterParsingEC);
  if (filterParsingEC != cli::CliOptionResult::EOK) {
    throw std::invalid_argument(folly::to<std::string>(
        "Error in filter parsing: ",
        apache::thrift::util::enumNameSafe(filterParsingEC)));
  }

  ValidFilterMapType validFilters = {};
  if (!parsedFilters.empty()) {
    validFilters = getValidFilters();
    const auto& errorCode =
        CmdGlobalOptions::getInstance()->isValid(validFilters, parsedFilters);
    if (!(errorCode == cli::CliOptionResult::EOK)) {
      throw std::invalid_argument(folly::to<std::string>(
          "Error in filter parsing: ",
          apache::thrift::util::enumNameSafe(errorCode)));
    }
  }

  auto aggParsingEC = cli::CliOptionResult::EOK;
  auto parsedAggregationInput =
      CmdGlobalOptions::getInstance()->parseAggregate(aggParsingEC);

  if (aggParsingEC != cli::CliOptionResult::EOK) {
    throw std::invalid_argument(folly::to<std::string>(
        "Error in aggregate parsing: ",
        apache::thrift::util::enumNameSafe(aggParsingEC)));
  }

  ValidAggMapType validAggs = {};
  if (parsedAggregationInput.has_value()) {
    validAggs = getValidAggs();
    const auto& errorCode = CmdGlobalOptions::getInstance()->isValidAggregate(
        validAggs, parsedAggregationInput);
    if (errorCode != cli::CliOptionResult::EOK) {
      throw std::invalid_argument(folly::to<std::string>(
          "Error in Aggregate validation: ",
          apache::thrift::util::enumNameSafe(errorCode)));
    }
  }

  auto hosts = getHosts();

  std::vector<std::shared_future<std::tuple<std::string, RetType, std::string>>>
      futureList;

  for (const auto& host : hosts) {
    futureList.push_back(std::async(
                             std::launch::async,
                             &CmdHandler::asyncHandler,
                             this,
                             host,
                             parsedFilters,
                             validFilters)
                             .share());
  }

  if (!parsedAggregationInput.has_value()) {
    if (CmdGlobalOptions::getInstance()->getFmt().isJson()) {
      printJson(impl(), futureList, std::cout, std::cerr);
    } else {
      printTabular(impl(), futureList, std::cout, std::cerr);
    }
  } else {
    printAggregate<CmdTypeT>(parsedAggregationInput, futureList, validAggs);
  }

  for (auto& fut : futureList) {
    auto [host, data, errStr] = fut.get();
    // exit with failure if any of the calls failed
    if (!errStr.empty()) {
      throw std::runtime_error("Error in command execution");
    }
  }
}

template <typename CmdTypeT, typename CmdTypeTraits>
void CmdHandler<CmdTypeT, CmdTypeTraits>::run() {
  utils::setLogLevel(CmdGlobalOptions::getInstance()->getLogLevel());

  // setup a stop_watch for total time to run command
  folly::stop_watch<> watch;
  utils::CmdLogInfo cmdLogInfo;

  try {
    // The MACROS are executed in reversed order
    // In order to have SCOPE_EXIT be the last one to get executed,
    // it must come before SCOPE_FAIL and SCOPE_SUCCESS
    SCOPE_EXIT {
      cmdLogInfo.CmdName = folly::demangle(typeid(this)).toStdString();
      cmdLogInfo.Arguments = CmdArgsLists::getInstance()->getArgStr();
      cmdLogInfo.Duration = utils::getDurationStr(watch);
      cmdLogInfo.UserInfo = utils::getUserInfo();
      utils::logUsage(cmdLogInfo);
    };

    SCOPE_FAIL {
      cmdLogInfo.ExitStatus = "FAILURE";
    };

    SCOPE_SUCCESS {
      cmdLogInfo.ExitStatus = "SUCCESS";
    };

    runHelper();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }
}

/* Logic: We consider a thrift struct to be filterable only if it is of type
  list and has a single element in it that is another thrift struct.
  The idea is that the outer thrift struct (which corresponds to the RetType()
  in the cmd traits) should contain a list of the actual displayable thrift
  entry. For instance, the ShowPortModel struct has only one elelent that is a
  list of PortEntry struct. This makes it filterable.
  */
template <typename CmdTypeT, typename CmdTypeTraits>
bool CmdHandler<CmdTypeT, CmdTypeTraits>::isFilterable() {
  int numFields = 0;
  bool filterable = true;
  if constexpr (
      apache::thrift::is_thrift_struct_v<RetType> &&
      CmdTypeT::Traits::ALLOW_FILTERING == true) {
    apache::thrift::for_each_field(
        RetType(), [&](const ThriftField& meta, auto&& /*field_ref*/) {
          const auto& fieldType = *meta.type();
          const auto& thriftBaseType = fieldType.getType();

          if (thriftBaseType != ThriftType::Type::t_list) {
            filterable = false;
          }
          numFields += 1;
        });
  }

  if (numFields != 1) {
    filterable = false;
  }
  return filterable;
}

/* Logic (similar to isFilterable): We consider a thrift struct to be
  amenable to aggregation only if it is of type list and has a single element
  in it that is another thrift struct.
  The idea is that the outer thrift struct (which corresponds to the RetType()
  in the cmd traits) should contain a list of the actual displayable thrift
  entry. For instance, the ShowPortModel struct has only one elelent that is a
  list of PortEntry struct. This makes it aggregatable.
  */
template <typename CmdTypeT, typename CmdTypeTraits>
bool CmdHandler<CmdTypeT, CmdTypeTraits>::isAggregatable() {
  int numFields = 0;
  bool aggregatable = true;
  if constexpr (
      apache::thrift::is_thrift_struct_v<RetType> &&
      CmdTypeT::Traits::ALLOW_AGGREGATION) {
    apache::thrift::for_each_field(
        RetType(), [&](const ThriftField& meta, auto&& /*field_ref*/) {
          const auto& fieldType = *meta.type();
          const auto& thriftBaseType = fieldType.getType();

          if (thriftBaseType != ThriftType::Type::t_list) {
            aggregatable = false;
          }
          numFields += 1;
        });
  }

  if (numFields != 1) {
    aggregatable = false;
  }
  return aggregatable;
}

template <class T>
using get_value_type_t = typename T::value_type;

/* Logic: We first check if this thrift struct is filterable at all. If it is,
 we constuct a filter map that contains filterable fields. For now, only
 primitive fields will be added to the filter map and considered filterable.
 Also, here we are traversing the nested thrift struct using the visitation api.
 for that, we start with the RetType() object and get to the fields of the inner
 struct using reflection. This is a much cleaner approach than the one that
 redirects through each command's handler to get the inner struct.
 */
template <typename CmdTypeT, typename CmdTypeTraits>
const ValidFilterMapType
CmdHandler<CmdTypeT, CmdTypeTraits>::getValidFilters() {
  if (!CmdHandler<CmdTypeT, CmdTypeTraits>::isFilterable()) {
    return {};
  }
  ValidFilterMapType filterMap;

  if constexpr (
      apache::thrift::is_thrift_struct_v<RetType> &&
      CmdTypeT::Traits::ALLOW_FILTERING == true) {
    apache::thrift::for_each_field(
        RetType(),
        [&filterMap, this](
            const ThriftField& /*outer_meta*/, auto&& outer_field_ref) {
          if constexpr (apache::thrift::is_thrift_struct_v<folly::detected_or_t<
                            void,
                            get_value_type_t,
                            folly::remove_cvref_t<
                                decltype(*outer_field_ref)>>>) {
            using NestedType = get_value_type_t<
                folly::remove_cvref_t<decltype(*outer_field_ref)>>;
            if constexpr (apache::thrift::is_thrift_struct_v<NestedType>) {
              apache::thrift::for_each_field(
                  NestedType(),
                  [&, this](const ThriftField& meta, auto&& /*field_ref*/) {
                    const auto& fieldType = *meta.type();
                    const auto& thriftBaseType = fieldType.getType();

                    // we are only supporting filtering on primitive typed keys
                    // for now. This will be expanded as we proceed.
                    if (thriftBaseType == ThriftType::Type::t_primitive) {
                      const auto& thriftType = fieldType.get_t_primitive();
                      switch (thriftType) {
                        case ThriftPrimitiveType::THRIFT_STRING_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<std::string>>(
                              *meta.name(),
                              impl().getAcceptedFilterValues()[*meta.name()]);
                          break;

                        case ThriftPrimitiveType::THRIFT_I16_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<int16_t>>(
                              *meta.name());
                          break;

                        case ThriftPrimitiveType::THRIFT_I32_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<int32_t>>(
                              *meta.name());
                          break;

                        case ThriftPrimitiveType::THRIFT_I64_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<int64_t>>(
                              *meta.name());
                          break;

                        case ThriftPrimitiveType::THRIFT_BYTE_TYPE:
                          filterMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::TypeVerifier<std::byte>>(
                              *meta.name());
                          break;

                        default:;
                      }
                    }
                  });
            }
          }
        });
  }
  return filterMap;
}

/* Logic: We first check if this thrift struct is aggregatable at all. If it is,
 we constuct an aggregate map that contains aggregatable fields. Only numeric
 and string fields are aggregatable for now. The map maps to the AggInfo object
 that contains information about the operations that are permitted for this
 column. This information is subsequenlty used to perform aggregate validation.
 Also, here we are traversing the nested thrift struct using the visitation api.
 for that, we start with the RetType() object and get to the fields of the inner
 struct using reflection.
 */
template <typename CmdTypeT, typename CmdTypeTraits>
const ValidAggMapType CmdHandler<CmdTypeT, CmdTypeTraits>::getValidAggs() {
  if (!CmdHandler<CmdTypeT, CmdTypeTraits>::isAggregatable()) {
    return {};
  }
  ValidAggMapType aggMap;

  if constexpr (
      apache::thrift::is_thrift_struct_v<RetType> &&
      CmdTypeT::Traits::ALLOW_AGGREGATION) {
    apache::thrift::for_each_field(
        RetType(),
        [&aggMap, this](
            const ThriftField& /*outer_meta*/, auto&& outer_field_ref) {
          if constexpr (apache::thrift::is_thrift_struct_v<folly::detected_or_t<
                            void,
                            get_value_type_t,
                            folly::remove_cvref_t<
                                decltype(*outer_field_ref)>>>) {
            using NestedType = get_value_type_t<
                folly::remove_cvref_t<decltype(*outer_field_ref)>>;
            if constexpr (apache::thrift::is_thrift_struct_v<NestedType>) {
              apache::thrift::for_each_field(
                  NestedType(),
                  [&, this](const ThriftField& meta, auto&& /*field_ref*/) {
                    const auto& fieldType = *meta.type();
                    const auto& thriftBaseType = fieldType.getType();

                    // Aggregation only supported on numerical keys
                    if (thriftBaseType == ThriftType::Type::t_primitive) {
                      const auto& thriftType = fieldType.get_t_primitive();
                      switch (thriftType) {
                        case ThriftPrimitiveType::THRIFT_I16_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<int16_t>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_I32_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<int32_t>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_I64_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<int64_t>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_FLOAT_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<float>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_DOUBLE_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<double>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::SUM,
                                  AggregateOpEnum::AVG,
                                  AggregateOpEnum::MIN,
                                  AggregateOpEnum::MAX,
                                  AggregateOpEnum::COUNT});
                          break;

                        case ThriftPrimitiveType::THRIFT_STRING_TYPE:
                          aggMap[*meta.name()] = std::make_shared<
                              CmdGlobalOptions::AggInfo<std::string>>(
                              *meta.name(),
                              std::vector<AggregateOpEnum>{
                                  AggregateOpEnum::COUNT});

                        default:;
                      }
                    }
                  });
            }
          }
        });
  }
  return aggMap;
}

} // namespace facebook::fboss
