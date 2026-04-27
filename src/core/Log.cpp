#include "core/Log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace helios::core {
void Log::Init() { Message(LogLevel::Info, "Logger initialized"); }

void Log::Shutdown() { Message(LogLevel::Info, "Logger shutdown"); }

void Log::Message(LogLevel level, const std::string& text) {
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif

  const char* prefix = "[INFO]";
  if (level == LogLevel::Warning) {
    prefix = "[WARN]";
  } else if (level == LogLevel::Error) {
    prefix = "[ERR ]";
  }

  std::cout << std::put_time(&tm, "%H:%M:%S") << " " << prefix << " " << text << std::endl;
}
} // namespace helios::core
