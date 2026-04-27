#include "core/Time.h"

namespace helios::core {
float Time::s_DeltaTime = 0.0f;
float Time::s_TotalTime = 0.0f;

void Time::Tick(double deltaSeconds) {
  s_DeltaTime = static_cast<float>(deltaSeconds);
  s_TotalTime += s_DeltaTime;
}

float Time::DeltaTime() { return s_DeltaTime; }

float Time::TotalTime() { return s_TotalTime; }
} // namespace helios::core
