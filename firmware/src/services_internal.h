#pragma once
#include "services.h"
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
namespace services::detail {
extern Settings config;
extern State current;
extern SemaphoreHandle_t state_mutex;
extern QueueHandle_t observations;
extern Observation latest;
extern std::atomic<bool> http_serving;
void startWorker();
void writeState(const State& next);
}
