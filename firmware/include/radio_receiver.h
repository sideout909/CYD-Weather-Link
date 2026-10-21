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
}  // namespace radio_receiver
