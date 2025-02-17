////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#include "h2/utils/Logger.hpp"

#include "h2_config.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "logger_internals.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#if H2_HAS_MPI
#define H2_LOGGER_HAS_MPI
#include <mpi.h>
#endif

#if __has_include(<unistd.h>)
#define H2_LOGGER_HAS_UNISTD_H
#include <unistd.h>
#endif

namespace
{
char const* get_first_env(std::initializer_list<char const*> names)
{
  for (auto const& name : names)
  {
    if (char const* ptr = std::getenv(name))
      return ptr;
  }
  return nullptr;
}

class MPIRankFlag : public spdlog::custom_flag_formatter
{
public:
  void format(spdlog::details::log_msg const&,
              std::tm const&,
              spdlog::memory_buf_t& dest) override
  {
    static std::string rank = get_rank_str();
    dest.append(rank.data(), rank.data() + rank.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override
  {
    return spdlog::details::make_unique<MPIRankFlag>();
  }

  static std::string get_rank_str()
  {
    int rank = get_rank_mpi();
    if (rank < 0)
      rank = get_rank_env();
    return (rank >= 0 ? std::to_string(rank) : std::string("?"));
  }

  static int get_rank_mpi()
  {
#ifdef H2_LOGGER_HAS_MPI
    int is_init = 0;
    MPI_Initialized(&is_init);
    if (is_init)
    {
      int rank = 0;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      return rank;
    }
#endif  // H2_LOGGER_HAS_MPI
    return -1;
  }

  static int get_rank_env()
  {
    char const* env = get_first_env({"FLUX_TASK_RANK",
                                     "SLURM_PROCID",
                                     "PMI_RANK",
                                     "MPIRUN_RANK",
                                     "OMPI_COMM_WORLD_RANK",
                                     "MV2_COMM_WORLD_RANK"});
    return env ? std::atoi(env) : -1;
  }
};  // class MPIRankFlag

class MPISizeFlag : public spdlog::custom_flag_formatter
{
public:
  void format(spdlog::details::log_msg const&,
              std::tm const&,
              spdlog::memory_buf_t& dest) override
  {
    static std::string size = get_size_str();
    dest.append(size.data(), size.data() + size.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override
  {
    return spdlog::details::make_unique<MPISizeFlag>();
  }

  static std::string get_size_str()
  {
    int size = get_size_mpi();
    if (size < 0)
      size = get_size_env();
    return (size >= 0 ? std::to_string(size) : std::string("?"));
  }

  static int get_size_mpi()
  {
#ifdef H2_LOGGER_HAS_MPI
    int is_init = 0;
    MPI_Initialized(&is_init);
    if (is_init)
    {
      int size = 0;
      MPI_Comm_size(MPI_COMM_WORLD, &size);
      return size;
    }
#endif  // H2_LOGGER_HAS_MPI
    return -1;
  }

  static int get_size_env()
  {
    char const* env = get_first_env({"FLUX_JOB_SIZE",
                                     "SLURM_NTASKS",
                                     "PMI_SIZE",
                                     "MPIRUN_NTASKS",
                                     "OMPI_COMM_WORLD_SIZE",
                                     "MV2_COMM_WORLD_SIZE"});
    return env ? std::atoi(env) : -1;
  }
};  // class MPISizeFlag

class HostnameFlag final : public spdlog::custom_flag_formatter
{
public:
  void format(spdlog::details::log_msg const&,
              std::tm const&,
              spdlog::memory_buf_t& dest) override
  {
    static auto const hostname = get_hostname();
    dest.append(hostname.data(), hostname.data() + hostname.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override
  {
    return spdlog::details::make_unique<HostnameFlag>();
  }

#ifdef H2_LOGGER_HAS_UNISTD_H
  static std::string get_hostname()
  {
    char buf[1024];
    if (gethostname(buf, 1024) != 0)
      throw std::runtime_error("gethostname failed.");
    auto end = std::find(buf, buf + 1024, '\0');
    return std::string{buf, end};
  }
#else
  static std::string const& get_hostname()
  {
    static std::string const hostname = "<unknown>";
    return hostname;
  }
#endif  // H2_LOGGER_HAS_UNISTD_H

};  // class HostnameFlag

}  // namespace

using LevelMapType = std::unordered_map<std::string, h2::Logger::LogLevelType>;
using MaskMapType = std::unordered_map<std::string, unsigned char>;

::spdlog::sink_ptr h2_internal::make_file_sink(std::string const& sinkname)
{
  if (sinkname == "stdout")
    return std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  if (sinkname == "stderr")
    return std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
  return std::make_shared<spdlog::sinks::basic_file_sink_mt>(sinkname);
}

::spdlog::sink_ptr h2_internal::get_file_sink(std::string const& sinkname)
{
  static std::unordered_map<std::string, ::spdlog::sink_ptr> sink_map_;

  auto& sink = sink_map_[sinkname];
  if (!sink)
    sink = make_file_sink(sinkname);
  return sink;
}

std::unique_ptr<::spdlog::pattern_formatter>
h2_internal::make_h2_formatter(std::string const& pattern_prefix)
{
  auto formatter = std::make_unique<spdlog::pattern_formatter>();
  formatter->add_flag<HostnameFlag>('h');
  formatter->add_flag<MPISizeFlag>('W');
  formatter->add_flag<MPIRankFlag>('w');
  formatter->set_pattern(pattern_prefix + "%v");

  return formatter;
}

std::shared_ptr<::spdlog::logger>
h2_internal::make_logger(std::string name,
                         std::string const& sink_name,
                         std::string const& pattern_prefix)
{
  auto logger = std::make_shared<::spdlog::logger>(std::move(name),
                                                   get_file_sink(sink_name));
  logger->set_formatter(make_h2_formatter(pattern_prefix));
  ::spdlog::register_logger(logger);
  logger->set_level(::spdlog::level::trace);
  return logger;
}

// convert to uppercase
std::string& h2_internal::to_upper(std::string& str)
{
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::toupper(c);
  });

  return str;
}

h2::Logger::LogLevelType
h2_internal::get_log_level_type(std::string const& level)
{
  char const* const s = level.data();
  size_t const l = level.length();

  switch (s[0])
  {
  case 'T':
    if (strncmp(s, "TRACE", l) == 0)
      return h2::Logger::LogLevelType::TRACE;
    break;
  case 'D':
    if (strncmp(s, "DEBUG", l) == 0)
      return h2::Logger::LogLevelType::DEBUG;
    break;
  case 'I':
    if (strncmp(s, "INFO", l) == 0)
      return h2::Logger::LogLevelType::INFO;
    break;
  case 'W':
    if (strncmp(s, "WARNING", l) == 0)
      return h2::Logger::LogLevelType::WARN;
    break;
  case 'E':
    if (strncmp(s, "ERROR", l) == 0)
      return h2::Logger::LogLevelType::ERROR;
    break;
  case 'C':
    if (strncmp(s, "CRITICAL", l) == 0)
      return h2::Logger::LogLevelType::CRITICAL;
    break;
  case 'O':
    if (strncmp(s, "OFF", l) == 0)
      return h2::Logger::LogLevelType::OFF;
    break;
  default: break;
  }
  throw std::runtime_error("Invalid log level: " + level);
  return h2::Logger::LogLevelType::OFF;
}

std::string
h2_internal::get_log_level_string(h2::Logger::LogLevelType const& level)
{
  switch (level)
  {
  case h2::Logger::LogLevelType::TRACE: return "TRACE";
  case h2::Logger::LogLevelType::DEBUG: return "DEBUG";
  case h2::Logger::LogLevelType::INFO: return "INFO";
  case h2::Logger::LogLevelType::WARN: return "WARN";
  case h2::Logger::LogLevelType::ERROR: return "ERROR";
  case h2::Logger::LogLevelType::CRITICAL: return "CRITICAL";
  case h2::Logger::LogLevelType::OFF: return "OFF";
  default: throw std::runtime_error("Invalid log level");
  }
}

// trim spaces
std::string& h2_internal::trim(std::string& str)
{
  char const* spaces = " \n\r\t";
  str.erase(str.find_last_not_of(spaces) + 1);
  str.erase(0, str.find_first_not_of(spaces));
  return str;
}

unsigned char h2_internal::extract_mask(std::string levels)
{
  unsigned char mask = 0x0;

  std::string token;
  std::istringstream token_stream(levels);
  while (std::getline(token_stream, token, '|'))
  {
    if (token.empty())
      continue;

    mask |= get_log_level_type(trim(to_upper(token)));
  }

  return mask;
}

h2::Logger::LogLevelType h2_internal::extract_level(std::string level)
{
  return get_log_level_type(trim(to_upper(level)));
}

std::pair<std::string, std::string>
h2_internal::extract_key_and_val(char delim, std::string const& str)
{
  auto const n = str.find(delim);
  if (n == std::string::npos)
    return std::make_pair("", str);
  auto key = str.substr(0, n);
  auto const val = str.substr(n + 1);

  return std::make_pair(trim(key), val);
}

MaskMapType h2_internal::get_keys_and_masks(std::string const& str)
{
  std::string token;
  std::istringstream token_stream(str);
  MaskMapType km{};
  while (std::getline(token_stream, token, ','))
  {
    if (token.empty())
      continue;

    auto kv = extract_key_and_val('=', token);

    km[kv.first] = extract_mask(kv.second);
  }
  return km;
}

LevelMapType h2_internal::get_keys_and_levels(std::string const& str)
{
  std::string token;
  std::istringstream token_stream(str);
  LevelMapType kl{};
  while (std::getline(token_stream, token, ','))
  {
    if (token.empty())
      continue;

    auto kv = extract_key_and_val('=', token);

    kl[kv.first] = extract_level(kv.second);
  }

  return kl;
}

namespace h2
{

Logger::Logger(std::string name,
               std::string const& sink_name,
               std::string const& pattern_prefix)
  : m_logger{
      h2_internal::make_logger(std::move(name), sink_name, pattern_prefix)}
{}

void Logger::set_log_level(LogLevelType level)
{
  unsigned char mask = 0x0;

  switch (level)
  {
  case LogLevelType::TRACE: mask |= LogLevelType::TRACE; [[fallthrough]];
  case LogLevelType::DEBUG: mask |= LogLevelType::DEBUG; [[fallthrough]];
  case LogLevelType::INFO: mask |= LogLevelType::INFO; [[fallthrough]];
  case LogLevelType::WARN: mask |= LogLevelType::WARN; [[fallthrough]];
  case LogLevelType::ERROR: mask |= LogLevelType::ERROR; [[fallthrough]];
  case LogLevelType::CRITICAL: mask |= LogLevelType::CRITICAL; break;
  default: mask = LogLevelType::OFF;
  }

  set_mask(mask);
}

void Logger::set_mask(unsigned char mask)
{
  m_mask = mask;
}

bool Logger::should_log(LogLevelType level) const noexcept
{
  return ((m_mask & level) == level);
}

void setup_levels(std::vector<Logger*>& loggers,
                  char const* const level_env_var,
                  h2::Logger::LogLevelType default_level)
{
  char const* const var = std::getenv(level_env_var);
  auto level_kv =
    (var ? h2_internal::get_keys_and_levels(var) : LevelMapType{});

  if (level_kv.count(""))
  {
    default_level = level_kv.at("");
    level_kv.erase("");
  }

  for (auto& l : loggers)
  {
    auto const name = l->name();
    l->set_log_level(level_kv.count(name) ? level_kv.at(name) : default_level);
    level_kv.erase(name);
  }
  if (level_kv.size())
  {
    std::string err = "Unknown loggers: ";
    for (auto const& [k, _] : level_kv)
      err += k + " ";
    throw std::runtime_error(err);
  }
}

void setup_masks(std::vector<Logger*>& loggers,
                 char const* const mask_env_var,
                 unsigned char default_mask)
{
  char const* const var = std::getenv(mask_env_var);
  auto mask_kv = (var ? h2_internal::get_keys_and_masks(var) : MaskMapType{});

  if (mask_kv.count(""))
  {
    default_mask = mask_kv.at("");
    mask_kv.erase("");
  }

  for (auto& l : loggers)
  {
    auto const name = l->name();
    l->set_mask(mask_kv.count(name) ? mask_kv.at(name) : default_mask);
    mask_kv.erase(name);
  }

  if (mask_kv.size())
  {
    std::string err = "Unknown loggers: ";
    for (auto const& [k, _] : mask_kv)
      err += k + " ";
    throw std::runtime_error(err);
  }
}

spdlog::level::level_enum to_spdlog_level(Logger::LogLevelType level)
{
  switch (level)
  {
  case Logger::LogLevelType::TRACE: return spdlog::level::level_enum::trace;
  case Logger::LogLevelType::DEBUG: return spdlog::level::level_enum::debug;
  case Logger::LogLevelType::INFO: return spdlog::level::level_enum::info;
  case Logger::LogLevelType::WARN: return spdlog::level::level_enum::warn;
  case Logger::LogLevelType::ERROR: return spdlog::level::level_enum::err;
  case Logger::LogLevelType::CRITICAL:
    return spdlog::level::level_enum::critical;
  default: return spdlog::level::off;
  }
}
}  // namespace h2
