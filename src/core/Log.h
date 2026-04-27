#pragma once

#include <string>

namespace helios::core {
enum class LogLevel {
  Info,
  Warning,
  Error
};

class Log {
public:
  static void Init();
  static void Shutdown();
  static void Message(LogLevel level, const std::string& text);
};
} // namespace helios::core
