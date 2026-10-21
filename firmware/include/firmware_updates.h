#pragma once
#include "services.h"
namespace firmware_updates {
struct State {
  String message="Updates off",available_version;
  bool busy=false,available=false,boot_pending=false,restart_pending=false;
  uint8_t progress=0;
  time_t last_checked=0;
};
void begin();
void startWorker();
State state();
bool requestCheck();
bool requestInstall();
void tick(const services::Settings& settings);
void confirmBoot(bool healthy);
bool restartPending();
}
