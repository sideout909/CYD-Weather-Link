#pragma once
#include <Arduino.h>

namespace radio_receiver {
// Only the six documented external pins. Display/touch/native USB are separate.
constexpr int kSck = 10, kMiso = 11, kMosi = 12, kCs = 13, kGdo0 = 14, kGdo2 = 21;
bool begin();
void loop();
bool ready();
bool pop(char* output, size_t capacity);
uint32_t dropped();
struct Diagnostics {
  bool ready=false,enabled=false;
  int current_rssi=0,average_rssi=0,threshold=0,margin=0;
  uint32_t decoded_signals=0,decoded_messages=0,zero_decodes=0;
  uint32_t capture_drops=0,decoder_drops=0;
};
Diagnostics diagnostics();
bool setCaptureMargin(unsigned db);
}  // namespace radio_receiver
