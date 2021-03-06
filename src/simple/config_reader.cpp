/* Copyright (c) 2017-2018, Hans Erik Thrane */

#include "simple/config_reader.h"

#include <ucl++.h>
#include <cctz/time_zone.h>
#include <roq/logging.h>

#include <chrono>
#include <fstream>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "common/config_variables.h"

namespace examples {
namespace simple {

namespace {
const auto NaN = std::numeric_limits<double>::quiet_NaN();
// Read and parse config file.
// Optionally expand variables using another config file.
static ucl::Ucl read_config_file(const std::string& config_file,
    const std::string& config_variables) {
  auto variables = common::ConfigVariables::read(config_variables);
  LOG(INFO) << "Reading config file \"" << config_file << "\"";
  std::ifstream file(config_file);
  LOG_IF(FATAL, file.fail()) << "Unable to read \"" << config_file << "\"";
  LOG(INFO) << "Parsing config file";
  std::string data((std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());
  std::string error;
  auto result = ucl::Ucl::parse(data, variables, error);
  LOG_IF(FATAL, !error.empty()) << error;
  return result;
}
// Create accounts
static std::map<std::string, common::Config::Instrument::Account>
create_accounts(
    const ucl::Ucl& setting) {
  std::map<std::string, common::Config::Instrument::Account> result;
  for (auto& iter : setting) {
    auto key = iter.key();
    LOG_IF(FATAL, key.empty()) << "Expected a dictionary";
    auto account = common::Config::Instrument::Account {
      .long_limit = iter.lookup("long_limit").number_value(),
      .short_limit = iter.lookup("short_limit").number_value(),
      .long_start_of_day = NaN,
      .short_start_of_day = NaN,
    };
    result.emplace(key, std::move(account));
  }
  return result;
}
// Create base instrument.
static common::Config::Instrument create_base_instrument(
    const ucl::Ucl& setting) {
  return common::Config::Instrument {
    .exchange   = setting.lookup("exchange").string_value(),
    .symbol     = setting.lookup("symbol").string_value(),
    .tick_size  = setting.lookup("tick_size").number_value(),
    .multiplier = setting.lookup("multiplier").number_value(),
    .net_limit  = setting.lookup("net_limit").number_value(),
    .accounts   = create_accounts(setting.lookup("accounts")),
  };
}
// Create base config.
static common::Config create_base_config(const ucl::Ucl& setting) {
  common::Config result;
  LOG_IF(FATAL, setting.type() != UCL_ARRAY) << "Instruments must be an array";
  for (auto i = 0; i < setting.size(); ++i)
    result.instruments.emplace_back(
        create_base_instrument(setting.at(i)));
  return result;
}
// Create timer schedule.
static std::vector<std::string> create_schedule(const ucl::Ucl& setting) {
  std::vector<std::string> result;
  LOG_IF(FATAL, setting.type() != UCL_ARRAY) << "Schedule must be an array";
  for (auto i = 0; i < setting.size(); ++i)
    result.push_back(setting.at(i).string_value());
  return result;
}
// Create config object from parsed config file.
static Config create_config(const ucl::Ucl& setting) {
  Config result {
    .config    = create_base_config(setting.lookup("instruments")),
    .weighted  = setting.lookup("weighted").bool_value(),
    .threshold = setting.lookup("threshold").number_value(),
    .quantity  = static_cast<double>(setting.lookup("quantity").int_value()),
    .time_zone = setting.lookup("time_zone").string_value(),
    .schedule =  create_schedule(setting.lookup("schedule")),
  };
  LOG(INFO) << "config=" << result;
  return result;
}
}  // namespace

Config ConfigReader::read(const std::string& config_file,
    const std::string& config_variables) {
  try {
    auto config = read_config_file(config_file, config_variables);
    auto root = config["strategy"];
    LOG_IF(FATAL, root.type() == UCL_NULL) <<
      "Root (strategy) was not found in the config file";
    return create_config(root);
  } catch (std::exception& e) {
    LOG(FATAL) << e.what();
    std::abort();  // FIXME(thraneh): avoid warning until LOG(FATAL) has been fixed
  }
}

}  // namespace simple
}  // namespace examples
