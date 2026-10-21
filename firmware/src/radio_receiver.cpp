#include "radio_receiver.h"
#include <rtl_433_ESP.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstring>

namespace radio_receiver {
namespace {
rtl_433_ESP receiver;
constexpr size_t kMessageSize = 1024;
struct Message { char json[kMessageSize]; };
QueueHandle_t queue = nullptr;
char decoder_buffer[kMessageSize];
bool initialized = false;
volatile uint32_t queue_drops = 0;
void decoded(char* json) {
  // Called from the decoder task. Only copy to a bounded queue; no UI, TLS,
  // configuration, or shared weather-model access from this context.
  Message message{};
  const size_t length = strnlen(json,kMessageSize);
  if (length >= kMessageSize) { ++queue_drops; return; }
  memcpy(message.json,json,length+1);
  if (xQueueSend(queue,&message,0)!=pdTRUE) ++queue_drops;
}
}
bool begin() {
  if (!queue) queue=xQueueCreate(4,sizeof(Message));
  if (!queue) return false;
  receiver.setCallback(decoded,decoder_buffer,sizeof(decoder_buffer));
  initialized=receiver.begin(kGdo0,433.92f);
  if (initialized) receiver.enableReceiver();
  return initialized;
}
void loop() { if(initialized)receiver.loop(); }
bool ready() { return initialized; }
bool pop(char* output,size_t capacity) {
  if(!queue || capacity<kMessageSize)return false;
  Message message;
  if(xQueueReceive(queue,&message,0)!=pdTRUE)return false;
  memcpy(output,message.json,kMessageSize);return true;
}
uint32_t dropped() { return queue_drops; }
Diagnostics diagnostics(){
  Diagnostics d;d.ready=initialized;if(!initialized)return d;
  const auto s=receiver.statusSnapshot();d.enabled=s.receiverEnabled;
  d.current_rssi=s.currentRssi;d.average_rssi=s.averageRssi;d.threshold=s.rssiThreshold;
  d.decoded_signals=s.decodedSignals;d.decoded_messages=s.decodedMessages;
  d.zero_decodes=s.zeroDecodedSignals;d.capture_drops=s.droppedCaptureBuffers;
  d.decoder_drops=s.droppedDecoderQueue;return d;
}
}
