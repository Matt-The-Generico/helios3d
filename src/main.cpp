#include "core/Application.h"

int main() {
  helios::core::Application app;
  if (!app.Init()) {
    return 1;
  }
  app.Run();
  app.Shutdown();
  return 0;
}
