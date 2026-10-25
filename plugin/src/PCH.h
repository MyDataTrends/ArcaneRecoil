#pragma once

// CommonLibSSE-NG — RE:: namespace (Skyrim reverse-engineered classes)
// and SKSE:: namespace (SKSE API)
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

// Required for the auto-generated plugin descriptor (uses "..."sv literals)
using namespace std::literals;

// Convenience logging macros
#define LOG_INFO(...) SKSE::log::info(__VA_ARGS__)
#define LOG_WARN(...) SKSE::log::warn(__VA_ARGS__)
#define LOG_ERROR(...) SKSE::log::error(__VA_ARGS__)

// Per-hit diagnostics: off unless the "Debug log" MCM option is on (v1.0 logged and flushed every hit).
namespace Settings { bool DebugLog(); }
#define LOG_DEBUG(...) do { if (Settings::DebugLog()) SKSE::log::info(__VA_ARGS__); } while (0)
