#pragma once

namespace helios::core {
class Time {
public:
  static void Tick(double deltaSeconds);
  static float DeltaTime();
  static float TotalTime();

private:
  static float s_DeltaTime;
  static float s_TotalTime;
};
} // namespace helios::core
