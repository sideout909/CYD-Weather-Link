#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace factory_reset {
inline constexpr const char* kConfirmation="RESET WX LINK";
inline constexpr std::array<const char*,9> kFiles={
  "/weather.bin","/weather.bin.tmp","/archive-v1.bin","/archive-v1.tmp",
  "/nws.json","/nws.tmp","/checkpoint-selftest.bin","/checkpoint-selftest.bin.tmp","/archive-selftest.bin"
};
inline bool confirmed(const char* phrase){return phrase&&!std::strcmp(phrase,kConfirmation);}
inline bool held(uint64_t now,uint64_t started,bool eligible){
  return eligible&&now>=started&&now-started>=10000;
}
enum class Recovery { NotRequested, Complete, Failed };
// Run only at startup, before workers/history open files. A durable marker is
// cleared last, so a partial reset retries rather than loading surviving secrets.
template<class Backend> Recovery recover(Backend& backend){
  bool pending=false;
  if(!backend.pending(pending))return Recovery::Failed;
  if(!pending)return Recovery::NotRequested;
  if(!backend.mount())return Recovery::Failed;
  for(const char* path:kFiles)if(!backend.removeFile(path))return Recovery::Failed;
  if(!backend.clearSettings())return Recovery::Failed;
  if(!backend.complete())return Recovery::Failed;
  return Recovery::Complete;
}
bool arm();
Recovery resume();
}
